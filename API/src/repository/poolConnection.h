#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_POOLCONNECTION_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_POOLCONNECTION_H

#include "sqlite3.h"

#define CONN_POOL_MAX_COUNT 10

typedef struct _SQL_NODE
{
    sqlite3* db;
    pthread_mutex_t lock;
    int used;
    int index;
    enum {
        DB_DISCONN, DB_CONN
    }sql_state;

}SQL_NODE;

typedef struct _SQL_CONN_POOL
{
    SQL_NODE conn_pool[CONN_POOL_MAX_COUNT];
    int conn_count;
    int busy_count;
    int shutdown;
    char databaseName[64];
}SQL_CONN_POOL;


SQL_CONN_POOL* sql_pool_create(int conn_count, char databaseName[]);
void sql_pool_destroy(SQL_CONN_POOL* sql_conn_pool);
int sql_conn_create(SQL_CONN_POOL* sql_conn_pool, SQL_NODE* sql_node);
SQL_NODE* sql_conn_get(SQL_CONN_POOL* sql_conn_pool);
void sql_conn_release(SQL_CONN_POOL* sql_conn_pool, SQL_NODE* sql_node);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_POOLCONNECTION_H
