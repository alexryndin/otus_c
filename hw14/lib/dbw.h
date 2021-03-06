#include <bstring/bstring/bstrlib.h>
#include <rvec.h>

typedef enum DBWType {
    DBW_INTEGER,
    DBW_UNKN,

} DBWType;

typedef enum DBWResType {
    DBW_TUPLES = 0,
    DBW_TYPES,
} DBWResType;

typedef enum DBWError {
    DBW_ERR = -1,
    DBW_ERR_NOT_FOUND = -2,
    DBW_ERR_UNKN_DB = -3,
} DBWDBError;

typedef struct DBWResult {
    int res_type;
    rvec_t(bstring) head_vec;
    union {
        rvec_t(bstring) res_vec;
        rvec_t(int) types_vec;
    };
} DBWResult;

typedef enum DBWDBType {
    DBW_POSTGRESQL,
    DBW_SQLITE3,
} DBWDBType;

typedef struct DBWHandler {
    int DBWDBType;
    void *conn;
} DBWHandler;

DBWHandler *dbw_connect(int DBWDBType, const bstring url, int *err);
DBWResult *dbw_get_table_types(DBWHandler *h, const bstring table, int *err);
DBWResult *dbw_query(DBWHandler *h, const bstring query, int *err);
int dbw_close(DBWHandler *h);
int dbw_print(DBWResult *res);
int DBWResult_destroy(DBWResult *res);
