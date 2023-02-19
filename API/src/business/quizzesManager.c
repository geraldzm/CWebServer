#include <malloc.h>
#include "quizzesManager.h"

// --------------------------------

int addConnection(QuizzesManager* quizManager, Guest* guest, QuizHandler* quizHandler) {

    // validate the state of the quizHandler
    if(quizHandler->status == QUIZ_HANDLER_ENDED) {
        logWarning("Quiz %d is closed, cannot add connection", quizHandler->id);
        return 0;
    }

    sem_wait(&quizManager->connections_sem);
    for(int i = 0; i < MAX_CONNECTIONS; i++) {
        if(quizManager->connections[i] == NULL) {
            // space found, create connection

            int ok = addGuestToQuizHandler(quizHandler, guest);

            if(ok) {
                ConnectionInfo* connectionInfo = malloc(sizeof(ConnectionInfo));
                connectionInfo->quizHandler = quizHandler;
                connectionInfo->guest = guest;
                connectionInfo->position = i;
                quizManager->connections[i] = connectionInfo;
            }

            sem_post(&quizManager->connections_sem);
            return ok;
        }
    }

    sem_post(&quizManager->connections_sem);
    logWarning("Max connections reached (%d), guest %s will not be added to quiz %d", MAX_CONNECTIONS, guest->username, quizHandler->id);

    return 0;
}

ConnectionInfo* findConnectionByGuestId(QuizzesManager* quizManager, uint32_t guestId) {

    logInfo("Searching for connection with guest id %d", guestId);
    // validate state
    if(quizManager->state == QUIZ_MANAGER_CLOSED) {
        logWarning("QuizzesManager is closed, cannot communicate with guests");
        return NULL;
    }

    sem_wait(&quizManager->connections_sem);
    for(int i = 0; i < MAX_CONNECTIONS; i++)
        if(quizManager->connections[i] != NULL && quizManager->connections[i]->guest->id == guestId) {
            sem_post(&quizManager->connections_sem);
            return quizManager->connections[i];
        }
    sem_post(&quizManager->connections_sem);
    return NULL;
}

QuizHandler* findHandler(QuizzesManager* quizManager, unsigned int quizId) {

    // validate state
    if(quizManager->state == QUIZ_MANAGER_CLOSED) {
        logWarning("QuizzesManager is closed, cannot communicate with guests");
        return NULL;
    }

    sem_wait(&quizManager->handlers_sem);
    for(int i = 0; i < MAX_QUIZZES_SAMULTANEOUSLY; i++)
        if(quizManager->quizHandlers[i] != NULL && quizManager->quizHandlers[i]->id == quizId) {
            sem_post(&quizManager->handlers_sem);
            return quizManager->quizHandlers[i];
        }
    sem_post(&quizManager->handlers_sem);
    return NULL;
}

void removeConnection(QuizzesManager* quizManager, ConnectionInfo* connectionInfo ) {
    sem_wait(&quizManager->connections_sem);

    // save position
    int i = connectionInfo->position;

    // free connection
    destroyGuest(quizManager->connections[i]->guest);
    free(quizManager->connections[i]);
    quizManager->connections[i] = NULL;

    sem_post(&quizManager->connections_sem);
}

void manageOnOpen(QuizzesManager* quizManager, uint32_t guestId) {
    ConnectionInfo* connectionInfo = findConnectionByGuestId(quizManager, guestId);
    if(connectionInfo != NULL)
        onOpen(connectionInfo->quizHandler, connectionInfo->guest);
}

void manageOnClose(QuizzesManager* quizManager, uint32_t guestId) {
    ConnectionInfo* connectionInfo = findConnectionByGuestId(quizManager, guestId);
    if(connectionInfo != NULL) {
        onClose(connectionInfo->quizHandler, connectionInfo->guest);
        removeConnection(quizManager, connectionInfo);
    }
}

void manageOnMessage(QuizzesManager* quizManager, uint32_t guestId, const unsigned char *msg, uint64_t msg_size){
    ConnectionInfo* connectionInfo = findConnectionByGuestId(quizManager, guestId);
    if(connectionInfo != NULL)
        onMessage(connectionInfo->quizHandler, connectionInfo->guest, msg, msg_size);
}

// --------------------------------

int initHandlerRoom(QuizzesManager* quizManager, unsigned int quizId) {
    // validate state
    if(quizManager->state == QUIZ_MANAGER_CLOSED) {
        logWarning("QuizzesManager is closed, cannot init the quiz %d", quizId);
        return 0;
    }

    // validate quizId
    QuizHandler* quizHandler = findHandler(quizManager, quizId);
    if(quizHandler == NULL) {
        logWarning("QuizHandler %d not found, cannot init the quiz", quizId);
        return 0;
    }

    return initHandler(quizHandler);
}

QuizzesManager* createQuizzesManager() {
    QuizzesManager* quizManager = malloc(sizeof(QuizzesManager));

    // init semaphores
    sem_init(&quizManager->handlers_sem, 0, 1);
    sem_init(&quizManager->connections_sem, 0, 1);

    // init rooms
    for(int i = 0; i < MAX_QUIZZES_SAMULTANEOUSLY; i++)
        quizManager->quizHandlers[i] = NULL;

    // init connections
    for(int i = 0; i < MAX_CONNECTIONS; i++)
        quizManager->connections[i] = NULL;

    quizManager->state = QUIZ_MANAGER_OPEN;

    return quizManager;
}

QuizHandler* createQuizRoom(QuizzesManager* quizManager, unsigned int quizId) {

    // validate state
    if(quizManager->state == QUIZ_MANAGER_CLOSED) {
        logWarning("QuizzesManager is closed, cannot create new quiz");
        return 0;
    }

    // find space in quizManager
    sem_wait(&quizManager->handlers_sem);
    for(int i = 0; i < MAX_QUIZZES_SAMULTANEOUSLY; i++) {
        if(quizManager->quizHandlers[i] == NULL) {
            // space found, quizHandler
            quizManager->quizHandlers[i] = createQuizHandler(quizId);
            sem_post(&quizManager->handlers_sem);

            return quizManager->quizHandlers[i];
        }
    }
    sem_post(&quizManager->handlers_sem);

    return NULL;
}

void closeQuizRoom(QuizzesManager* quizManager, unsigned int quizId) {
    sem_wait(&quizManager->handlers_sem);
    for(int i = 0; i < MAX_QUIZZES_SAMULTANEOUSLY; i++) {
        if(quizManager->quizHandlers[i] != NULL && quizManager->quizHandlers[i]->id == quizId) {
            // room found, close it
//            closeRoom(quizManager->quizHandlers[i]->room);
            sem_post(&quizManager->handlers_sem);
            return;
        }
    }

    sem_post(&quizManager->handlers_sem);
}

int joinQuizRoom(QuizzesManager* quizManager, unsigned int quizId, Guest* guest) {

    // validate state
    if(quizManager->state == QUIZ_MANAGER_CLOSED) {
        logWarning("QuizzesManager is closed, cannot join quiz room");
        return 0;
    }

    //
    sem_wait(&quizManager->handlers_sem);
    for(int i = 0; i < MAX_QUIZZES_SAMULTANEOUSLY; i++) {
        if(quizManager->quizHandlers[i] != NULL && quizManager->quizHandlers[i]->id == quizId) {
            int ok = addConnection(quizManager, guest, quizManager->quizHandlers[i]);
            sem_post(&quizManager->handlers_sem);
            return ok;
        }
    }
    sem_post(&quizManager->handlers_sem);

    logWarning("Quiz room %d not found, guest %s will not be added", quizId, guest->username);
    return 0;
}

void destroyQuizManager(QuizzesManager* quizManager) {

    // change quizManager state to closed
    quizManager->state = QUIZ_MANAGER_CLOSED;

    // close all rooms
    for(int i = 0; i < MAX_QUIZZES_SAMULTANEOUSLY; i++) {
        if(quizManager->quizHandlers[i] != NULL) {
            destroyQuizHandler(quizManager->quizHandlers[i]);
        }
    }

    // close all connections
    for(int i = 0; i < MAX_CONNECTIONS; i++) {
        if(quizManager->connections[i] != NULL) {
            removeConnection(quizManager, quizManager->connections[i]);
        }
    }

    // destroy semaphores
    sem_destroy(&quizManager->handlers_sem);
    sem_destroy(&quizManager->connections_sem);

    free(quizManager);
}