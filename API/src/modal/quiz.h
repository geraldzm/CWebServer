#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZ_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZ_H

#include <semaphore.h>
#include "question.h"


typedef enum {
    QUIZ_CREATED,
    QUIZ_WAITING,
    QUIZ_STARTED,
    QUIZ_FINISHED
} QuizStatus;

typedef struct Quiz {
    unsigned int id;
    unsigned int roomID;
    char* name;
    unsigned int createdByUserId;
    QuizStatus status;
    sem_t quizSem;
} Quiz;

typedef struct QuizQuestion {
    unsigned int id;
    unsigned int quizId;
    unsigned int questionId;
} QuizQuestion;

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZ_H
