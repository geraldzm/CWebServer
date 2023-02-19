#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZZESMANAGER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZZESMANAGER_H

// The roomManager is in charge of all the quizzes
// it will create a quizHandler for each quiz
// it will keep track of the amount of active quizzes
// it will start and stop the quizHandlers

#include "quizHandler.h"

#define MAX_QUIZZES_SAMULTANEOUSLY 10
#define MAX_CONNECTIONS MAX_QUIZZES_SAMULTANEOUSLY * MAX_GUESTS


typedef struct ConnectionInfo {
    int position;
    Guest *guest;
    QuizHandler *quizHandler;
} ConnectionInfo;

typedef enum {
    QUIZ_MANAGER_OPEN, // quiz is open for new guests
    QUIZ_MANAGER_CLOSED // quiz is closed for new guests
} QuizzesManagerStatus;

typedef struct QuizzesManager{
    QuizHandler* quizHandlers[MAX_QUIZZES_SAMULTANEOUSLY];
    ConnectionInfo* connections[MAX_CONNECTIONS];

    // create semaphores for managing rooms
    sem_t handlers_sem;
    sem_t connections_sem;

    // status
    QuizzesManagerStatus state; // 1 if destroying, 0 if not
} QuizzesManager;

QuizzesManager* createQuizzesManager();

// the admin uses this function to create a quiz
QuizHandler* createQuizRoom(QuizzesManager* quizManager, unsigned int quizId);// returns the ID of the room created
void closeQuizRoom(QuizzesManager* quizManager, unsigned int quizId);

int initHandlerRoom(QuizzesManager* quizManager, unsigned int quizId);

// a socket try to connect to a quiz and a guest is created
int joinQuizRoom(QuizzesManager* quizManager, unsigned int quizId, Guest* guest);

// the program is stopped, so we need to close all the rooms and free resources
void destroyQuizzesManager(QuizzesManager* quizManager);

void manageOnOpen(QuizzesManager* quizManager, uint32_t guestId);
void manageOnClose(QuizzesManager* quizManager, uint32_t guestId);
void manageOnMessage(QuizzesManager* quizManager, uint32_t guestId, const unsigned char *msg, uint64_t msg_size);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZZESMANAGER_H
