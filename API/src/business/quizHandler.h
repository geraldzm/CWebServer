#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZHANDLER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZHANDLER_H

// The quizHandler is in charge of one single quiz
// it will synchronize the quiz with the users
// it will change the questions and send them to the users

#include "roomPattern/room.h"
#include "../modal/quiz.h"

typedef enum {
    QUIZ_HANDLER_CREATED, // quiz is created but not started
    QUIZ_HANDLER_WAITING_FOR_ADMIN,
    QUIZ_HANDLER_INITIALIZED,
    QUIZ_HANDLER_ENDED,
} QuizHandlerStatus;

typedef enum {
    CHANGE_QUESTION,
    FINISH_QUIZ,
    CHANGE_STATUS,
    UPDATE_SCORE,
} MessageType;

// the quizHandler, the room and the quiz have the same ID
typedef struct QuizHandler {
    unsigned int id;
    sem_t sem;

    Guest* guests[MAX_GUESTS];
    int guestsAmount;
    sem_t guests_sem;

    Question* currentQuestion;
    sem_t currentQuestion_sem;

    char* currentQuestionJson;

    QuizHandlerStatus status;
} QuizHandler;

struct ThreadParams {
    QuizHandler* quizHandler;
    Quiz* quiz;
    unsigned int* questionIds;
    unsigned int amountOfQuestions;
};

QuizHandler* createQuizHandler(unsigned int quizId);
void destroyQuizHandler(QuizHandler* quizHandler);

int addGuestToQuizHandler(QuizHandler* quizHandler, Guest* guest);

void *handleQuiz(void* arg);
int initHandler(QuizHandler* quizHandler);

// called when the guest joins the room
void onOpen(QuizHandler* quizHandler, Guest* guest);
void onClose(QuizHandler* quizHandler, Guest* guest);
void onMessage(QuizHandler* quizHandler, Guest* guest, const unsigned char *msg, uint64_t msg_size);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZHANDLER_H
