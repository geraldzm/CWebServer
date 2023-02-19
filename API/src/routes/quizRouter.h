#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZROUTER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZROUTER_H


#include "../../../httpServer/responseHttp.h"

void quizInfo(ResponseHttp* response, RequestHttp* request);
void joinQuizRequest(ResponseHttp* response, RequestHttp* request);
void joinQuizSocket(ResponseHttp* response, RequestHttp* request);

void quizzes(ResponseHttp* response, RequestHttp* request);


#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZROUTER_H
