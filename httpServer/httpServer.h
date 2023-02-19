#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_HTTPSERVER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_HTTPSERVER_H

#include "global.h"
#include "responseHttp.h"
#include "requestHttp.h"


void startHttpServer(int port);
void stopHttpServer();
void handleFunc(char* _endPoint, void (*_func)(struct ResponseHttp* _request, struct RequestHttp* _response),  Method _method);
void setNotFoundHandler(void (*_func)(struct ResponseHttp* _request, struct RequestHttp* _response));
void redirect(struct ResponseHttp* _request, struct RequestHttp* _response, char* _url);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_HTTPSERVER_H
