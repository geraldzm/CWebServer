#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "poolConnection.h"

SQL_CONN_POOL* sql_pool_create(int conn_count, char databaseName[])
{
    if (conn_count < 1) return NULL;

    SQL_CONN_POOL* sql_conn_pool = NULL;
    sql_conn_pool = (SQL_CONN_POOL*)malloc(sizeof(SQL_CONN_POOL));
    if (sql_conn_pool == NULL) return NULL;

    sql_conn_pool->shutdown = 0;
    sql_conn_pool->conn_count = 0;
    sql_conn_pool->busy_count = 0;
    strcpy(sql_conn_pool->databaseName, databaseName);


    if (conn_count > CONN_POOL_MAX_COUNT) {
        conn_count = CONN_POOL_MAX_COUNT;
    }

    int i;
    for (i = 0; i < conn_count; i++) {

        if (0 != sql_conn_create(sql_conn_pool, &sql_conn_pool->conn_pool[i])) {
            sql_pool_destroy(sql_conn_pool);
            return NULL;
        }

        sql_conn_pool->conn_pool[i].index = i;
        sql_conn_pool->conn_count++;
    }

    return sql_conn_pool;
}

int sql_conn_create(SQL_CONN_POOL* sql_conn_pool, SQL_NODE* sql_node)
{
    if (sql_conn_pool->shutdown == 1) return -1;

    sqlite3* db;
    sqlite3_open(sql_conn_pool->databaseName, &db);
    sql_node->db = &db;

    if (!sql_node->db) {
        sql_node->sql_state = DB_DISCONN;
        return -1;
    }

    pthread_mutex_init(&sql_node->lock, NULL);
    sql_node->used = 0;
    sql_node->sql_state = DB_CONN;

    return 0;
}

void sql_pool_destroy(SQL_CONN_POOL* sql_conn_pool)
{
    sql_conn_pool->shutdown = 1;
    int i;
    int sql_conn_count = sql_conn_pool->conn_count;
    for (i = 0; i < sql_conn_count; i++) {

        if (NULL != sql_conn_pool->conn_pool[i].db) {
            sqlite3_close(sql_conn_pool->conn_pool[i].db);
            sql_conn_pool->conn_pool[i].db = NULL;
        }
        sql_conn_pool->conn_pool[i].sql_state = DB_DISCONN;
        sql_conn_pool->conn_count--;
    }
    free(sql_conn_pool);
    sql_conn_pool = NULL;
}

SQL_NODE* sql_conn_get(SQL_CONN_POOL* sql_conn_pool)
{
    if (sql_conn_pool->shutdown == 1) return NULL;

    srand((int)time(0));
    int start_index = rand() % sql_conn_pool->conn_count;

    int i, index;
    for (i = 0; i < sql_conn_pool->conn_count; i++) {

        index = (start_index + i) % sql_conn_pool->conn_count;
        if (pthread_mutex_trylock(&sql_conn_pool->conn_pool[index].lock)) {
            continue;
        }

        if (DB_DISCONN == sql_conn_pool->conn_pool[index].sql_state) {
            if (0 != sql_conn_create(sql_conn_pool, &(sql_conn_pool->conn_pool[index]))) {
                sql_conn_release(sql_conn_pool, &(sql_conn_pool->conn_pool[index]));
                continue;
            }
        }

        if (NULL == sql_conn_pool->conn_pool[index].db) {
            sql_conn_pool->conn_pool[index].sql_state = DB_DISCONN;
            sql_conn_release(sql_conn_pool, &(sql_conn_pool->conn_pool[index]));
            continue;
        }
        else {
            sql_conn_pool->conn_pool[index].used = 1;
            sql_conn_pool->busy_count++;
            break;
        }
    }

    return i == sql_conn_pool->conn_count ? NULL : &(sql_conn_pool->conn_pool[index]);
}


void sql_conn_release(SQL_CONN_POOL* sql_conn_pool, SQL_NODE* sql_node)
{
    sql_node->used = 0;
    sql_conn_pool->busy_count--;
    pthread_mutex_unlock(&sql_node->lock);
}
