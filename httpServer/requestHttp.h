#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_REQUESTHTTP_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_REQUESTHTTP_H

#include <stdint-gcc.h>
#include "global.h"
#include "../util/logger.h"

typedef struct Param {
    char *key;
    char *value;
} Param;

typedef struct Cookie {
    char *key;
    char *value;
} Cookie;

typedef struct RequestHttp{

    char* endpoint;
    Param* params;
    int paramsAmount;

    Cookie* cookies;
    int cookiesAmount;

    Header *headers;
    int headersAmount;

    Method method;
    char* methodStr;

    // ip address
    char ip[46];
    // ip address as a number
    uint32_t ipNum;
    // port
    uint16_t port;

    int client_sockfd;

} RequestHttp;


#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_REQUESTHTTP_H
