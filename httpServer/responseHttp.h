#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_RESPONSEHTTP_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_RESPONSEHTTP_H

#include "requestHttp.h"
#include "../API/src/repository/fileManager.h"

typedef struct ResponseHttp {
    unsigned int statusCode;
    File* body;

    ContentType contentType;

    Header *__headers;

    int __headersAmount;

} ResponseHttp;

void sendResponse200HTML(ResponseHttp* response, RequestHttp* request, File *staticFile);
void sendResponse400(ResponseHttp* response, RequestHttp* request, char* message);
void sendResponse500(ResponseHttp* response, RequestHttp* request, char* message);

void sendResponse(ResponseHttp* response, RequestHttp* request);
void setHeader(ResponseHttp* response, char* key, char* value);
void getStatusCode(char* statusCodeString, unsigned int statusCode);
void setCookie(ResponseHttp* response, char* key, char* value);
void removeCookie(ResponseHttp* response, char* key);
char* getCookie(RequestHttp* request, char* key);
char* getParam(RequestHttp* request, char* key);
char* getHeader(RequestHttp* request, char* key);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_RESPONSEHTTP_H

