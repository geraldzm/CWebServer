#include <pthread.h>
#include "quizController.h"
#include "../repository/quizRepository.h"

QuizzesManager* quizManager;

// --------------------- echo functions ---------------------
void controlOnOpen(Client *client) {
    manageOnOpen(quizManager, client->ip_num);
}

void controlOnClose(Client *client) {
    manageOnClose(quizManager, client->ip_num);
}

void controlOnMessage(Client *client, const unsigned char *msg, uint64_t msg_size) {
    manageOnMessage(quizManager, client->ip_num, msg, msg_size);
}

// --------------------------------
void changeQuizStatus(Quiz* quiz, QuizStatus status) {
    sem_wait(&quiz->quizSem);
    quiz->status = status;
    sem_post(&quiz->quizSem);
}

int startQuizRoom(unsigned int quizId) {

    // find quiz by id
    Quiz* quiz = findQuizById(quizId);
    if(quiz == NULL) {
        logWarning("Tried to start a nonexistent quiz");
        return 0;
    }

    if(quiz->status != QUIZ_CREATED) {
        logWarning("Tried to start a quiz with status different than CREATED");
        return 0;
    }

    // find list of ids of question of quiz
    unsigned int amountOfQuestions;
    unsigned int* questionIds = findQuestionsByQuizId(quizId, &amountOfQuestions);

    // create quiz, open a room for the clients to join
    QuizHandler* quizHandler = createQuizRoom(quizManager, quizId);
    if(quizHandler == NULL) {
        logWarning("Max number of quizzes reached, cannot start quiz &d", quizId);
        return 0;
    }

    quiz->roomID = quizHandler->id;
    logInfo("Created quiz room with id %d for quiz %d", quiz->roomID , quiz->id);

    // set quiz status to started
    changeQuizStatus(quiz, QUIZ_WAITING);

    // make the handler wait for the admin to initiate the quiz
    sem_wait(&quizHandler->sem);

    // start quiz handler as a new thread
    pthread_t thread;
    struct ThreadParams* params = malloc(sizeof(struct ThreadParams));
    params->quizHandler = quizHandler;
    params->quiz = quiz;
    params->questionIds = questionIds;
    params->amountOfQuestions = amountOfQuestions;

    pthread_create(&thread, NULL, &handleQuiz, params);

    return 1;
}

int initQuizRoom(unsigned int quizId) {
    return initHandlerRoom(quizManager, quizId);
}


int joinQuiz(unsigned int quizId, const char* username, uint32_t ipNum, Client **client, void(**sendMessage)(Client *client, char *msg)) {
    // 1. create a guest
    Guest* guest = createGuest(username, ipNum);
    guest->onMessageToGuest = sendMessage;
    guest->client = client;
    return joinQuizRoom(quizManager, quizId, guest);
}

void initQuizController() {
    quizManager = createQuizzesManager();
}


// -------------------------------

void statusToStr(QuizStatus status, char* str) {
    switch (status) {
        case QUIZ_CREATED:
            strcpy(str, "CREATED");
            break;
        case QUIZ_WAITING:
            strcpy(str, "WAITING");
            break;
        case QUIZ_STARTED:
            strcpy(str, "STARTED");
            break;
        case QUIZ_FINISHED:
            strcpy(str, "FINISHED");
            break;
        default:
            strcpy(str, "UNKNOWN");
            break;
    }
}

char* questionToJson(Question* question) {
    char* json = malloc(1000);
    sprintf(json, "{\"id\":%d,\"question\":\"%s\",\"points\":%d,\"time\":%d}", question->id, question->question, question->points, question->time);
    return json;
}

char* quizToJson(Quiz* quiz) {
    char* json = malloc(1000);
    char status[20];
    statusToStr(quiz->status, status);
    sprintf(json, "{\"id\":%d,\"name\":\"%s\",\"status\":\"%s\",\"roomID\":%d}", quiz->id, quiz->name, status, quiz->roomID);
    return json;
}

char* getQuestions() {
    // get all questions
    unsigned int amountOfQuestions;
    Question* questions = findAllQuestions(&amountOfQuestions);

    // create json
    char* json = malloc(2048);
    strcpy(json, "{\"questions\":[");
    for(int i = 0; i < amountOfQuestions; i++) {
        char* questionJson = questionToJson(&questions[i]);
        strcat(json, questionJson);
        if(i != amountOfQuestions - 1) {
            strcat(json, ",");
        }
        free(questionJson);
    }

    strcat(json, "]}");

    free(questions);

    return json;
}

char* getAllQuizzes() {
    // find all quizzes
    unsigned int quizCount = 0;

    Quiz* quizzes = findAllQuizzes(&quizCount);

    // create json
    char* json = malloc(1000);
    sprintf(json, "{\"quizzes\": [");
    for (int i = 0; i < quizCount; i++) {
        Quiz quiz = quizzes[i];
        char* quizJson = quizToJson(&quiz);
        strcat(json, quizJson);
        if (i < quizCount - 1) {
            strcat(json, ",");
        }
        free(quizJson);
    }
    strcat(json, "]}");

    return json;
}

char* getStatisticsOfQuiz(unsigned int quizId) {
    // find quiz by id
    Quiz* quiz = findQuizById(quizId);
    if(quiz == NULL) {
        logWarning("Tried to get statistics of a nonexistent quiz");
        return NULL;
    }

    // find list of ids of question of quiz
    unsigned int amountOfQuestions;
    unsigned int* questionIds = findQuestionsByQuizId(quizId, &amountOfQuestions);

    // create json
    char* json = malloc(1000);
    char* quizJson = quizToJson(quiz);
    sprintf(json, "{\"quiz\": %s, \"questions\": [", quizJson);
    free(quizJson);

    for (int i = 0; i < amountOfQuestions; i++) {
        unsigned int questionId = questionIds[i];
        Question* question = findQuestionById(questionId);
        char* questionJson = questionToJson(question);
        strcat(json, questionJson);
        if (i < amountOfQuestions - 1) {
            strcat(json, ",");
        }
        free(questionJson);
    }
    strcat(json, "]}");

    free(questionIds);

    return json;
}

void createQuizFromQuestions(char* name, unsigned int* questions, unsigned int questionsSize, unsigned int* quizID) {
    Quiz *quiz = createQuiz(name, 1);
    *quizID = quiz->id;

    for (int i = 0; i < questionsSize; i++) {
        int ok = addQuestionToQuiz(quiz->id, questions[i]);
        if (!ok) {
            logWarning("Could not add question %d to quiz %d", questions[i], quiz->id);
        }
    }

    // create a room for the quiz
    startQuizRoom(quiz->id);
}