#include "dbw.h"
#include "dbg.h"
#include <bstring/bstring/bstrlib.h>
#include <libpq-fe.h>
#include <rvec.h>
#include <sqlite3.h>
#include <stdlib.h>

// statics and helpers
static struct tagbstring __integer = bsStatic("integer");

// forward declarations
static DBWResult *DBWResult_create(int t);
static int DBWResult_destroy(DBWResult *res);

// ******************************
// * SQLite
// ******************************

static DBWHandler *sqlite3_connect(const bstring filename, int *err) {
    DBWHandler *h = NULL;
    sqlite3 *db = NULL;
    char *err_str = NULL;
    int rc = 0;
    int is_loading_enabled = 0;

    h = calloc(1, sizeof(DBWHandler));
    CHECK_MEM(h);
    CHECK(filename != NULL && bdata(filename) != NULL, "Null filename");

    rc = sqlite3_open(bdata(filename), &db);
    CHECK(rc == 0, "Couldn't open sqlite database: %s", sqlite3_errmsg(db));

    rc = sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1,
                           &is_loading_enabled);
    CHECK(rc == 0, "Couldn't allow sqlite to load extensions: %s",
          sqlite3_errmsg(db));
    CHECK(is_loading_enabled == 1, "Couldn't allow sqlite to load extensions");

    rc = sqlite3_load_extension(db, "./extension-functions", NULL, &err_str);
    CHECK(rc == 0, "Couldn't load extensions: %s", err_str);

    h->conn = db;
    h->DBWDBType = DBW_SQLITE3;
    return h;
error:
    if (h != NULL) {
        free(h);
    }
    if (db != NULL) {
        sqlite3_close(db);
    }
    if (err != NULL) {
        *err = DBW_ERR;
    }
    if (err_str != NULL) {
        sqlite3_free(err_str);
    }
    return NULL;
}

static int sqllite3_get_types_cb(DBWResult *res, int n, char **cls,
                                 char **cns) {
    bstring f = NULL;
    int err = 0;
    CHECK(n > 2, "Callback failed");

    struct tagbstring ct;
    struct tagbstring _type;
    f = bfromcstr(cls[1]);
    rv_push(res->head_vec, f, &err);
    f = NULL;
    CHECK(err == 0, "SQLite: callback failed");
    btfromcstr(ct, cls[2] == NULL ? "" : cls[2]);
    if (_type = (struct tagbstring)bsStatic("INTEGER"), !bstrcmp(&ct, &_type)) {
        rv_push(res->types_vec, DBW_INTEGER, NULL);
    } else {
        rv_push(res->types_vec, DBW_UNKN, NULL);
    }
    CHECK(err == 0, "SQLite: callback failed");

    return 0;

error:
    if (f != NULL) {
        bdestroy(f);
    }
    return -1;
}

// Warning -- this funciton is SQL injections vulnarable
static DBWResult *sqlite3_get_table_types(DBWHandler *h, const bstring table,
                                          int *err) {
    bstring query = NULL;
    int rc = 0;
    char *zErrMsg = NULL;
    DBWResult *ret = NULL;
    PGresult *res = NULL;

    CHECK(h != NULL, "Null handler");
    CHECK(table != NULL && bdata(table) != NULL, "Null table");

    ret = DBWResult_create(DBW_TYPES);
    CHECK_MEM(ret);

    query = bformat("PRAGMA TABLE_INFO(%s)", bdata(table));
    CHECK(query != NULL, "Couldn't prepare query");

    rc = sqlite3_exec(
        h->conn, bdata(query),
        (int (*)(void *, int, char **, char **))sqllite3_get_types_cb, ret,
        &zErrMsg);
    CHECK(rc == 0, "SQLite: couldn't execute query: %s", zErrMsg);

    if (res != NULL) {
        PQclear(res);
    }
    return ret;
error:
    if (ret != NULL) {
        free(ret);
    }
    if (res != NULL) {
        PQclear(res);
    }
    if (err != NULL) {
        *err = DBW_ERR;
    }
    return NULL;
}

static int sqllite3_query_cb(DBWResult *res, int n, char **cls, char **cns) {
    bstring f = NULL;
    int err = 0;

    // We assume that first callback invocation fills up header
    if (rv_len(res->head_vec) == 0) {
        for (int i = 0; i < n; i++) {
            f = bfromcstr(cns[i]);
            rv_push(res->head_vec, f, &err);
            f = NULL;
            CHECK(err == 0, "SQLite: callback failed");
        }
    }
    for (int i = 0; i < n; i++) {
        f = bfromcstr(cls[i] == NULL ? "" : cls[i]);
        rv_push(res->res_vec, f, &err);
        f = NULL;
        CHECK(err == 0, "SQLite: callback failed");
    }

    return 0;

error:
    if (f != NULL) {
        bdestroy(f);
    }
    return -1;
}

static DBWResult *sqlite3_query(DBWHandler *h, const bstring query, int *err) {
    int rc = 0;
    DBWResult *ret = NULL;
    char *zErrMsg = NULL;

    CHECK(h != NULL, "Null handler");
    CHECK(h->conn != NULL, "Null db connection");
    CHECK(query != NULL && bdata(query) != NULL, "Null query");

    ret = DBWResult_create(DBW_TUPLES);
    CHECK_MEM(ret);

    rc = sqlite3_exec(h->conn, bdata(query),
                      (int (*)(void *, int, char **, char **))sqllite3_query_cb,
                      ret, &zErrMsg);
    CHECK(rc == 0, "SQLite: couldn't execute query: %s", zErrMsg);

    return ret;
error:
    if (ret != NULL) {
        DBWResult_destroy(ret);
    }
    if (zErrMsg != NULL) {
        sqlite3_free(zErrMsg);
    }
    return NULL;
}
// ******************************
// * PostgreSQL
// ******************************

const char *const pg_table_types_query = "SELECT \
    column_name as c, \
    data_type as t \
FROM \
    information_schema.columns \
WHERE \
    table_schema = $1::text and table_name = $2::text;";

typedef rvec_t(bstring) rvec_bstring_t;

static DBWHandler *pg_connect(const bstring url, int *err) {
    DBWHandler *h = NULL;
    PGconn *conn = NULL;
    PGresult *res = NULL;

    h = calloc(1, sizeof(DBWHandler));
    CHECK_MEM(h);
    CHECK(url != NULL && bdata(url) != NULL, "Null url");

    conn = PQconnectdb(bdata(url));
    CHECK(PQstatus(conn) == CONNECTION_OK, "%s", PQerrorMessage(conn));

    res =
        PQexec(conn, "SELECT pg_catalog.set_config('search_path', '', false)");
    CHECK(PQresultStatus(res) == PGRES_TUPLES_OK,
          "Failed to SET search_path, error was %s", PQerrorMessage(conn));

    PQclear(res);
    res = NULL;
    h->conn = conn;
    h->DBWDBType = DBW_POSTGRESQL;
    return h;

error:
    if (h != NULL) {
        free(h);
    }
    if (res != NULL) {
        PQclear(res);
    }
    if (conn != NULL) {
        PQfinish(conn);
    }
    return NULL;
}

static DBWResult *pg_get_table_types(DBWHandler *h, const bstring table,
                                     int *err) {
    int n_fields = 0;
    int n_tuples = 0;
    DBWResult *ret = NULL;
    PGresult *res = NULL;
    bstring cn = NULL;
    struct bstrList *l = NULL;
    struct tagbstring ct;
    CHECK(h != NULL, "Null handler");
    CHECK(table != NULL, "Null query");
    ret = DBWResult_create(DBW_TYPES);
    CHECK_MEM(ret);

    l = bsplit(table, '.');
    CHECK(l->qty > 0 && l->qty < 3, "Couldn't split tablename");

    const char *params[2] = {0};
    if (l->qty == 1) {
        params[0] = bdata(l->entry[0]);
        res = PQexecParams(h->conn, pg_table_types_query, 1, NULL, params, NULL,
                           NULL, 0);
    } else {
        params[0] = bdata(l->entry[0]);
        params[1] = bdata(l->entry[1]);
        res = PQexecParams(h->conn, pg_table_types_query, 2, NULL, params, NULL,
                           NULL, 0);
    }
    CHECK(PQresultStatus(res) == PGRES_TUPLES_OK,
          "Failed to get table types, error was %s", PQerrorMessage(h->conn));
    n_fields = PQnfields(res);
    if (n_fields != 2) {
        LOG_ERR("Malformed result");
        if (err != NULL) {
            *err = DBW_ERR;
        }
        goto error;
    }
    n_tuples = PQntuples(res);
    for (int i = 0; i < n_tuples; i++) {
        cn = bfromcstr(PQgetvalue(res, i, 0));
        btfromcstr(ct, PQgetvalue(res, i, 1));
        rv_push(ret->head_vec, cn, NULL);
        if (!bstrcmp(&ct, &__integer)) {
            rv_push(ret->types_vec, DBW_INTEGER, NULL);
        } else {
            rv_push(ret->types_vec, DBW_UNKN, NULL);
        }
    }

    goto exit;
error:
    if (ret != NULL) {
        free(ret);
    }
    ret = NULL;
exit:
    if (l != NULL) {
        bstrListDestroy(l);
    }
    if (res != NULL) {
        PQclear(res);
    }
    return ret;
}

static DBWResult *pg_query(DBWHandler *h, const bstring query, int *err) {
    int n_fields = 0;
    int n_tuples = 0;
    DBWResult *ret = NULL;
    PGresult *res = NULL;
    bstring f = NULL;
    CHECK(h != NULL, "Null handler");
    CHECK(h->conn != NULL, "Null db connection");
    CHECK(query != NULL && bdata(query) != NULL, "Null query");
    ret = DBWResult_create(DBW_TUPLES);
    CHECK_MEM(ret);
    res = PQexec(h->conn, bdata(query));
    CHECK(PQresultStatus(res) == PGRES_TUPLES_OK,
          "Failed to execute query, error was %s", PQerrorMessage(h->conn));
    n_fields = PQnfields(res);
    for (int i = 0; i < n_fields; i++) {
        f = bfromcstr(PQfname(res, i));
        rv_push(ret->head_vec, f, NULL);
    }
    n_tuples = PQntuples(res);
    for (int i = 0; i < n_tuples; i++) {
        for (int j = 0; j < n_fields; j++) {
            f = bfromcstr(PQgetvalue(res, i, j));
            rv_push(ret->res_vec, f, NULL);
        }
    }

    if (res != NULL) {
        PQclear(res);
    }
    return ret;
error:
    if (ret != NULL) {
        DBWResult_destroy(ret);
    }
    if (res != NULL) {
        PQclear(res);
    }
    return NULL;
}

// ******************************
// * Generic interface
// ******************************

int DBWprint(DBWResult *res) {
    CHECK(res != NULL, "Null result.");
    size_t n_fields = (res->head_vec).n;

    if (n_fields < 1 || rv_len(res->res_vec) < 1) {
        return 0;
    }
    for (size_t j = 0; j < n_fields; j++) {
        bstring f = rv_pop(res->head_vec, NULL);
        printf("%s ", bdata(f));
    }
    printf("\n");
    for (; rv_len(res->res_vec) > 0;) {
        for (size_t j = 0; j < n_fields; j++) {
            bstring f = rv_pop(res->res_vec, NULL);
            printf("%s ", bdata(f));
        }
        printf("\n");
    }
error:
    return -1;
}

static int DBWResult_destroy(DBWResult *res) {
    CHECK(res != NULL, "Null result");
    rv_destroy(res->head_vec);
    if (res->res_type == DBW_TYPES) {
        rv_destroy(res->types_vec);
    } else {
        rv_destroy(res->head_vec);
    }

    return 0;
error:
    return -1;
}

static DBWResult *DBWResult_create(int t) {
    DBWResult *ret = calloc(1, sizeof(DBWResult));
    CHECK_MEM(ret);
    rv_init(ret->head_vec);
    if (t == DBW_TYPES) {
        rv_init(ret->types_vec);
    } else {
        rv_init(ret->res_vec);
    }
    ret->res_type = t;

    return ret;
error:
    if (ret != NULL) {
        free(ret);
    }
    return NULL;
}

DBWHandler *DBWconnect(int DBWDBType, const bstring url, int *err) {
    if (DBWDBType == DBW_POSTGRESQL)
        return pg_connect(url, err);
    if (DBWDBType == DBW_SQLITE3)
        return sqlite3_connect(url, err);
    if (err != NULL) {
        *err = DBW_ERR_UNKN_DB;
    }
error:
    return NULL;
}

DBWResult *get_table_types(DBWHandler *h, const bstring table, int *err) {
    CHECK(h != NULL, "Null handler.");
    if (h->DBWDBType == DBW_POSTGRESQL)
        return pg_get_table_types(h, table, err);
    if (h->DBWDBType == DBW_SQLITE3)
        return sqlite3_get_table_types(h, table, err);
    if (err != NULL) {
        *err = DBW_ERR_UNKN_DB;
    }

error:
    return NULL;
}

DBWResult *query(DBWHandler *h, const bstring query, int *err) {
    CHECK(h != NULL, "Null handler.");
    if (h->DBWDBType == DBW_POSTGRESQL)
        return pg_query(h, query, err);
    if (h->DBWDBType == DBW_SQLITE3)
        return sqlite3_query(h, query, err);
    if (err != NULL) {
        *err = DBW_ERR_UNKN_DB;
    }

error:
    return NULL;
}
