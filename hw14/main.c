#include <bstring/bstring/bstrlib.h>
#include <ctype.h>
#include <dbg.h>
#include <dbw.h>
#include <libpq-fe.h>
#include <rvec.h>
#include <stdio.h>

#define USAGE "Usage: %s db_type db_url table column\n"

static int has_forbidden_simbols(const bstring s) {
    CHECK(s != NULL && bdata(s) != NULL, "Null string");
    for (int i = 0; i < (blength(s)); i++) {
        if (!isalnum(bdata(s)[i]) && bdata(s)[i] != '_' && bdata(s)[i] != '-' &&
            bdata(s)[i] != '.') {
            return 1;
        }
    }
    return 0;
error:
    return -1;
}

int main(int argc, char *argv[]) {
    int rc = 0;
    int err = 0;
    int db_type = 0;
    if (argc != 5) {
        goto usage;
    }

    struct tagbstring db_type_str;
    btfromcstr(db_type_str, argv[1]);

    struct tagbstring url;
    btfromcstr(url, argv[2]);

    struct tagbstring table;
    btfromcstr(table, argv[3]);
    CHECK(!has_forbidden_simbols(&table), "Table name has forbidden simbols");

    struct tagbstring column;
    btfromcstr(column, argv[4]);
    CHECK(!has_forbidden_simbols(&column), "Column name has forbidden simbols");
    struct tagbstring tmp;

    if (tmp = (struct tagbstring)bsStatic("sqlite3"),
        !bstrcmp(&db_type_str, &tmp)) {
        db_type = DBW_SQLITE3;
    } else if (tmp = (struct tagbstring)bsStatic("postgresql"),
               !bstrcmp(&db_type_str, &tmp)) {
        db_type = DBW_POSTGRESQL;
    } else {
        LOG_ERR("Unsupported database type.");
        LOG_ERR("Choose of sqlite3 or postgresql.");
        goto error;
    }

    DBWHandler *h = DBWconnect(db_type, &url, &err);
    CHECK(h != NULL, "Couldn't connect to database");
    CHECK(err == 0, "Database connetcion error");

    DBWResult *res = get_table_types(h, &table, NULL);
    CHECK_MEM(res);
    char ok = 0;
    for (; rv_len(res->head_vec) > 0;) {
        bstring f = rv_pop(res->head_vec, NULL);
        LOG_DEBUG("%s", bdata(f));
        int type = rv_pop(res->types_vec, NULL);
        if (!bstrcmp(f, &column)) {
            if (type == DBW_INTEGER) {
                ok = 1;
                break;
            }
            bdestroy(f);
        }
    }
    CHECK(ok, "Couldn't find column with appropriate type");

    char *stdev_func_str = db_type == DBW_SQLITE3 ? "stdev" : "stddev";

    bstring q = bformat("SELECT max(t.c), min(t.c), avg(t.c), sum(t.c), "
                        "%s(t.c) from (select %s as c from %s) as t"
                        " limit 1",
                        stdev_func_str, bdata(&column), bdata(&table));

    res = query(h, q, NULL);
    DBWprint(res);

    rc = 0;
    goto exit;

error:
    rc = 1;
exit:
    return rc;
usage:
    printf(USAGE, argv[0]);
    goto error;
}
