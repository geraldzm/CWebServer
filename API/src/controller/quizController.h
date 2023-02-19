#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZCONTROLLER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZCONTROLLER_H

#include "../business/quizzesManager.h"
#include "../../../httpServer/webSocket/webSocketConnection.h"


// -------------------------------

void controlOnOpen(Client *client);
void controlOnClose(Client *client);
void controlOnMessage(Client *client, const unsigned char *msg, uint64_t msg_size);
// -------------------------------


int initQuizRoom(unsigned int quizId); // 1 success, 0 fail
int startQuizRoom(unsigned int quizId); // 1 success, 0 fail
int joinQuiz(unsigned int quizId, const char* username, uint32_t ipNum, Client **client, void(**sendMessage)(Client *client,char *msg));

void initQuizController();

// -------------------------------

char* getAllQuizzes();
char* getStatisticsOfQuiz(unsigned int quizId);
char* getQuestions();
void createQuizFromQuestions(char* name, unsigned int* questions, unsigned int questionsSize, unsigned int* quizID);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZCONTROLLER_H
