#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_ADMINROUTER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_ADMINROUTER_H

#include "../../../httpServer/responseHttp.h"

void adminStatistics(ResponseHttp* response, RequestHttp* request);
void adminJSONStatisticsOfQuiz(ResponseHttp* response, RequestHttp* request);

void adminDashboard(ResponseHttp* response, RequestHttp* request);
void adminCreateQuizHTML(ResponseHttp* response, RequestHttp* request);
void adminCreateQuiz(ResponseHttp* response, RequestHttp* request);
void adminCreateQuestion(ResponseHttp* response, RequestHttp* request);
void adminQuestion(ResponseHttp* response, RequestHttp* request);
void adminQuestions(ResponseHttp* response, RequestHttp* request);

//void adminStartQuiz(ResponseHttp* response, RequestHttp* request);
void adminInitQuiz(ResponseHttp* response, RequestHttp* request);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_ADMINROUTER_H
