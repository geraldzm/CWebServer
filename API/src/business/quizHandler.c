#include <unistd.h>
#include "quizHandler.h"
#include "../repository/quizRepository.h"
#include "../../../util/tools.h"


QuizHandler* createQuizHandler(unsigned int quizId) {
    QuizHandler* quizHandler = malloc(sizeof(QuizHandler));
    quizHandler->id = quizId;

    sem_init(&quizHandler->sem, 0, 1);
    sem_init(&quizHandler->guests_sem, 0, 1);
    sem_init(&quizHandler->currentQuestion_sem, 0, 1);

    for(int i = 0; i < MAX_GUESTS; i++)
        quizHandler->guests[i] = NULL;

    quizHandler->guestsAmount = 0;
    quizHandler->status = QUIZ_HANDLER_CREATED;

    quizHandler->currentQuestion = NULL;
    quizHandler->currentQuestionJson = NULL;

    return quizHandler;
}

void destroyQuizHandler(QuizHandler* quizHandler) {

    for(int i = 0; i < MAX_GUESTS; i++) {
        if(quizHandler->guests[i] != NULL) {
            destroyGuest(quizHandler->guests[i]);
        }
    }

    sem_destroy(&quizHandler->sem);
    sem_destroy(&quizHandler->guests_sem);

    free(quizHandler);
}

char* handlerStatusToMessage(QuizHandlerStatus status) {
    char* str = malloc(50);

    switch (status) {
        case QUIZ_HANDLER_WAITING_FOR_ADMIN:
            strcpy(str, "\"QUIZ_HANDLER_WAITING_FOR_ADMIN\"");
            break;
        case QUIZ_HANDLER_INITIALIZED:
            strcpy(str, "\"QUIZ_HANDLER_INITIALIZED\"");
            break;
        case QUIZ_HANDLER_ENDED:
            strcpy(str, "\"QUIZ_HANDLER_ENDED\"");
            break;
        default:
          strcpy(str, "\"Unkonwn message\"");
    }

    return str;
}


char* wrapMessage(char* json, MessageType type) {

    char* newJson = malloc(strlen(json) + 50);

    char typeString[20];
    if(type == CHANGE_QUESTION) {
        strcpy(typeString, "CHANGE_QUESTION");
    } else if(type == FINISH_QUIZ) {
        strcpy(typeString, "FINISH_QUIZ");
    } else if(type == CHANGE_STATUS) {
        strcpy(typeString, "CHANGE_STATUS");
    } else if(type == UPDATE_SCORE) {
        strcpy(typeString, "UPDATE_SCORE");
    } else {
        strcpy(typeString, "UNKNOWN");
    }

    sprintf(newJson, "{\"type\": \"%s\", \"data\": %s}", typeString, json);
    free(json);

    return newJson;
}

void updateStatusChanged(QuizHandlerStatus status, Guest* guest) {
    char* json = wrapMessage(handlerStatusToMessage(status), CHANGE_STATUS);
    (*guest->onMessageToGuest)(*guest->client, json);
    free(json);
}

void updateGuestScore(Guest* guest, unsigned int points) {
    guest->score += points;

    // to string
    char* score = malloc(sizeof(char) * 10);
    sprintf(score, "%d", guest->score);

    // create json
    char* json = wrapMessage(score, UPDATE_SCORE);

    // send
    (*guest->onMessageToGuest)(*guest->client, json);

    // free
    free(json);
}

void updateStatusChangedAllGuests(QuizHandler* quizHandler, QuizHandlerStatus status) {

    char* json = wrapMessage(handlerStatusToMessage(status), CHANGE_STATUS);

    sem_wait(&quizHandler->guests_sem);
    for(int i = 0; i < MAX_GUESTS; i++) {
        if(quizHandler->guests[i] != NULL) {
            Guest* guest = quizHandler->guests[i];
            (*guest->onMessageToGuest)(*guest->client, json);
        }
    }
    sem_post(&quizHandler->guests_sem);

    free(json);
}

void sendRankings(QuizHandler* handler) {
    // final json of the form: { finalScore: 10, ranking: [ { username: "user1", score: 10, position: 1 }, { username: "user2", score: 5, position: 2 } ] }


    sem_wait(&handler->guests_sem);

    // create ranking json list of the form: [ { username: "user1", score: 10, position: 1 }, { username: "user2", score: 5, position: 2 } ]
    char* rankingJson = malloc(sizeof(char) * 1024);
    strcpy(rankingJson, "[");

    // sort guests by score
    for(int i = 0; i < handler->guestsAmount; i++) {
        for(int j = i + 1; j < handler->guestsAmount; j++) {
            if(handler->guests[i]->score < handler->guests[j]->score) {
                Guest* temp = handler->guests[i];
                handler->guests[i] = handler->guests[j];
                handler->guests[j] = temp;
            }
        }
    }

    // create json for each guest
    int position = 1;
    for(int i = 0; i < MAX_GUESTS; i++) {
        if(handler->guests[i] != NULL) {
            Guest* guest = handler->guests[i];
            char* guestJson = malloc(sizeof(char) * 128);
            sprintf(guestJson, "{\"username\": \"%s\", \"score\": %d, \"position\": %d}", guest->username, guest->score, position++);
            if(position == handler->guestsAmount + 1) {
                strcat(rankingJson, guestJson);
            } else {
                strcat(rankingJson, guestJson);
                strcat(rankingJson, ",");
            }

            free(guestJson);
        }
    }
    strcat(rankingJson, "]");

    // send ranking to all guests
    for(int i = 0; i < MAX_GUESTS; i++) {
        if(handler->guests[i] != NULL) {
            Guest* guest = handler->guests[i];
            char* json = malloc(sizeof(char) * 1024);
            sprintf(json, "{\"finalScore\": %d, \"ranking\": %s}", guest->score, rankingJson);
            json = wrapMessage(json, FINISH_QUIZ);
            (*guest->onMessageToGuest)(*guest->client, json);
            free(json);
        }
    }

    free(rankingJson);

    sem_post(&handler->guests_sem);
}

void* handleQuiz(void* arg) {
    struct ThreadParams *params = arg;

    Quiz* quiz = params->quiz;
    QuizHandler* quizHandler = params->quizHandler;

    quizHandler->status = QUIZ_HANDLER_WAITING_FOR_ADMIN;

    logInfo("Quiz %d is waiting for admin to init the quiz...", quizHandler->id);

    // waiting for admin to init the quiz
    sem_wait(&quizHandler->sem);
    quizHandler->status = QUIZ_HANDLER_INITIALIZED;
    updateQuizStatus(quiz->id, QUIZ_STARTED);
    updateStatusChangedAllGuests( quizHandler, quizHandler->status);

    // init quiz
    logInfo("Quiz %d has been initialized", quizHandler->id);

    // quiz loop
    for(int i = 0; i < params->amountOfQuestions; i++) {
        // fetch question
        unsigned int questionId = params->questionIds[i];
        Question* question = findQuestionById(questionId);
        if(question == NULL) {
            logWarning("Question %d not found", questionId);
            continue;
        }
        sem_wait(&quizHandler->currentQuestion_sem);
        quizHandler->currentQuestion = question;
        sem_post(&quizHandler->currentQuestion_sem);

        // get answers of the question
        unsigned int amountOfAnswers;
        Answer** answers =  findAnswersByQuestionId(questionId, &amountOfAnswers);
        if(answers == NULL) {
            logWarning("Answers of question %d not found", questionId);
            continue;
        }

        logInfo("Question %d has %d answers", questionId, amountOfAnswers);
        // log answers
        for(int j = 0; j < amountOfAnswers; j++) {
            logInfo("Answer %d: %s", answers[j]->id, answers[j]->text);
        }

        // transform answers and question to json
        sem_wait(&quizHandler->currentQuestion_sem);
        if(quizHandler->currentQuestionJson != NULL) free(quizHandler->currentQuestionJson);
        quizHandler->currentQuestionJson = wrapMessage(questionAndAnswerToJson(question, answers, amountOfAnswers), CHANGE_QUESTION);
        sem_post(&quizHandler->currentQuestion_sem);

        // send question
        logInfo("Quiz %d is sending question %d: \"%s\" for %d points, %d seconds", quizHandler->id, questionId, question->question, question->points, question->time);

        sem_wait(&quizHandler->guests_sem);
        logInfo("Sending question to %d amount of guests", quizHandler->guestsAmount);
        for(int j = 0; j < quizHandler->guestsAmount; j++) {
            Guest* guest = quizHandler->guests[j];
            if(guest != NULL)
                (*guest->onMessageToGuest)(*guest->client, quizHandler->currentQuestionJson);
        }
        sem_post(&quizHandler->guests_sem);

        // free memory
        free(answers);

        sleep(question->time);
    }

    // end quiz
    quizHandler->status = QUIZ_HANDLER_ENDED;
    updateQuizStatus(quiz->id, QUIZ_FINISHED);
    updateStatusChangedAllGuests( quizHandler, quizHandler->status);

    sendRankings(quizHandler);
    logInfo("Quiz %d has ended", quizHandler->id);

    sem_wait(&quizHandler->currentQuestion_sem);
    if(quizHandler->currentQuestionJson != NULL) free(quizHandler->currentQuestionJson);
    quizHandler->currentQuestionJson = NULL;
    sem_post(&quizHandler->currentQuestion_sem);

    free(params->questionIds);
    free(params);

    return ((void *)0);
}

int initHandler(QuizHandler* quizHandler) {

    // validate state of the quizHandler
    if(quizHandler->status != QUIZ_HANDLER_WAITING_FOR_ADMIN) {
        logWarning("Quiz handler %d is not waiting for admin, cannot init the quiz", quizHandler->id);
        return 0;
    }

    // init the quizHandler
    sem_post(&quizHandler->sem);

    return 1;
}


// called when the guest joins the room
void onOpen(QuizHandler* quizHandler, Guest* guest) {
    logInfo("open with: %s on quiz: %d", guest->username, quizHandler->id);

    sem_wait(&quizHandler->guests_sem);
    quizHandler->guestsAmount++;
    sem_post(&quizHandler->guests_sem);

    updateStatusChanged(quizHandler->status, guest);

    // send question if quiz is initialized
    sem_wait(&quizHandler->currentQuestion_sem);
    if(quizHandler->currentQuestionJson != NULL)
        (*guest->onMessageToGuest)(*guest->client, quizHandler->currentQuestionJson);
    sem_post(&quizHandler->currentQuestion_sem);
}

// called when the guest leaves the room (disconnected)
void onClose(QuizHandler* quizHandler, Guest* guest) {
    logInfo("Guest %s disconnected quiz %d", guest->username, quizHandler->id);

    sem_wait(&quizHandler->guests_sem);
    // remove guest from the list
    for(int i = 0; i < MAX_GUESTS; i++)
        if(quizHandler->guests[i] != NULL && quizHandler->guests[i]->id == guest->id) {
            guest->status = GUEST_DISCONNECTED;
            quizHandler->guests[i] = NULL;
            quizHandler->guestsAmount--;
            break;
        }
    sem_post(&quizHandler->guests_sem);
}

// where I will receive the messages
void onMessage(QuizHandler* quizHandler, Guest* guest, const unsigned char *msg, uint64_t msg_size) {
    logInfo("Guest %s sent a message to quiz %d", guest->username, quizHandler->id);
    char ms[msg_size+1];
    memcpy(ms, msg, msg_size);
    ms[msg_size] = '\0';
    logInfo("Message: %s", ms);

    // parse message of the format: 1,2,3,4
    // where the first number is the question id
    // and the rest are the answer ids
    unsigned int id = -1;
    char idFound = 0;
    unsigned int answerIds[10];
    int answerIdsAmount = 0;
    char numberStr[10];
    int numberStrIndex = 0;
    for(int i = 0; i < msg_size; i++) {
        if(msg[i] == ',') {
            numberStr[numberStrIndex] = '\0';
            numberStrIndex = 0;
            if(idFound == 0) {
                parseStrToUnsInt(numberStr, &id);
                idFound = 1;
            } else {
                parseStrToUnsInt(numberStr, &answerIds[answerIdsAmount++]);
            }
        } else {
            numberStr[numberStrIndex++] = (char) msg[i];
        }

        if (i == msg_size - 1) {
            numberStr[numberStrIndex] = '\0';
            parseStrToUnsInt(numberStr, &answerIds[answerIdsAmount++]);
        }
    }

    logInfo("Question answered id: %d by %s", id, guest->username);
    for(int i = 0; i < answerIdsAmount; i++) {
        logInfo("Answer id: %d", answerIds[i]);
    }

    // check if the question is the same as the current question
    sem_wait(&quizHandler->currentQuestion_sem);
    Question *question = quizHandler->currentQuestion;
    if(quizHandler->currentQuestion->id != id) {
        logWarning("Guest %s answered a question that is not the current question", guest->username);
        sem_post(&quizHandler->currentQuestion_sem);
        return;
    }
    sem_post(&quizHandler->currentQuestion_sem);

    // check if the question is correct
    if(isAnswerCorrect(id, answerIds, answerIdsAmount)) {
        logInfo("Question %d was answered by %s correctly", id, guest->username);
        // update the score of the guest
        updateGuestScore(guest, question->points);
    } else {
        logInfo("Question %d was answered by %s incorrectly", id, guest->username);
    }
}


// ------------------------------------------------------------------------------------------
int addGuestToQuizHandler(QuizHandler* quizHandler, Guest* guest) {

    // validate quiz handler status
    if(quizHandler->status == QUIZ_HANDLER_ENDED) {
        logWarning("Quiz %d has ended", quizHandler->id);
        return 0;
    }

    sem_wait(&quizHandler->guests_sem);
    for(int i = 0; i < MAX_GUESTS; i++) {
        if(quizHandler->guests[i] == NULL) {
            quizHandler->guests[i] = guest;
            sem_post(&quizHandler->guests_sem);
            return 1;
        }
    }
    sem_post(&quizHandler->guests_sem);
    logWarning("Quiz %d is full (%d), guest %s can't join", quizHandler->id, MAX_GUESTS, guest->username);

    return 0;
}
