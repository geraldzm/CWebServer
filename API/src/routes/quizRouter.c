#include "quizRouter.h"
#include "../../../httpServer/httpServer.h"
#include "../../../util/tools.h"
#include "../repository/staticCache.h"
#include "../modal/quiz.h"
#include "../repository/quizRepository.h"
#include "../../../httpServer/webSocket/webSocketConnection.h"
#include "../controller/quizController.h"


void quizInfo(ResponseHttp* response, RequestHttp* request) {
    // log the request params
    logInfo("Quiz info request");
    sendResponse400(response, request, "Quiz info not implemented yet");
}



void joinQuizRequest(ResponseHttp* response, RequestHttp* request) {
    logInfo("join quiz request");

    // log the request params
    char *username = getParam(request, "username");
    char *quizCode = getParam(request, "quizcode");

    if (username == NULL || quizCode == NULL) {
        logWarning("Username or quizcode is NULL\0");
        redirect(response, request, "/");
        return;
    }

    logInfo("join request params username: %s, quizcode: %s", username, quizCode);

    // get quiz info
    // 1. try to parse the quizcode to long
    unsigned int quizID;
    int ok = parseStrToUnsInt(quizCode, &quizID);
    if (!ok) {
        logWarning("quizcode is not a number\0");
        setCookie(response, "message", "No quiz was found with that code");
        redirect(response, request, "/");
        return;
    }

    Quiz *quiz = findQuizById(quizID);

    if (quiz == NULL) {
        logWarning("quiz with id %d was not found\0", quizID);
        setCookie(response, "message", "Quiz not found\0");
        redirect(response, request, "/");
        return;
    }

    // 2. check if the quiz is active
    if (quiz->status == QUIZ_FINISHED) {
        logWarning("quiz with id %d has finished", quizID);
        setCookie(response, "message", "Quiz has finished");
        redirect(response, request, "/");
        return;
    }

    // 3. return quiz-home.html
    File *quizHome = getFile("quiz-home.html");

    response->statusCode = 200;
    response->contentType = HTML;
    response->body = malloc(sizeof(File));
    response->body->fsize = quizHome->fsize + 1;
    response->body->content = malloc(response->body->fsize);
    memcpy(response->body->content, quizHome->content, quizHome->fsize);
    response->body->content[quizHome->fsize] = '\0';

    char roomID[10];
    sprintf(roomID, "%d", quizID);
    setCookie(response, "quizID", roomID);
    setCookie(response, "username", username);

    sendResponse(response, request);
}


void joinQuizSocket(ResponseHttp* response, RequestHttp* request) {
    logInfo("Socket connection request");

    // log the request params
    char* quizIDStr = getParam(request, "quizID");

    if (quizIDStr == NULL) {
        logWarning("No quizID param was found");
        redirect(response, request, "/");
        return;
    }

    unsigned int quizID;
    int ok = parseStrToUnsInt(quizIDStr, &quizID);
    if (!ok) {
        logWarning("quizID is not a number\0");
        setCookie(response, "message", "No quiz was found with that code");
        redirect(response, request, "/");
        return;
    }

    // create a new guest
    char* username = getParam(request, "username");
    if (username == NULL) {
        logWarning("No username param was found");
        redirect(response, request, "/");
        return;
    }

    // ----------------------------------------------------------------------
    // everything is ok, create join the quiz
    WebSocketConnection connection;
    connection.onOpen = &controlOnOpen;
    connection.onClose = &controlOnClose;
    connection.onMessage = &controlOnMessage;

    // create a new guest and join the quiz
    ok = joinQuiz(quizID, username, request->ipNum, &connection.client, &connection.sendMessage);
    if (!ok) {
        setCookie(response, "message", "Error joining quiz\0");
        sendResponse500(response, request, "Error creating guest or joining quiz");
        return;
    }

    logInfo("Guest %s joined quiz %d", username, quizID);

    // upgrade to websocket
    ok = upgradeConnection(request, response, &connection);

    if(!ok) {
        setCookie(response, "message", "Error establishing a websocket connection");
        sendResponse500(response, request, "Error establishing a websocket connection");
        return;
    }

}


// ----------------------------------------------------------------------

void quizzes(ResponseHttp* response, RequestHttp* request) {
    logInfo("Quizzes request");

    char* jsonQuizzes = getAllQuizzes();

    response->statusCode = 200;
    response->contentType = JSON;
    response->body = malloc(sizeof(File));
    response->body->fsize = strlen(jsonQuizzes);
    response->body->content = malloc(response->body->fsize);
    memcpy(response->body->content, jsonQuizzes, response->body->fsize);

    free(jsonQuizzes);

    sendResponse(response, request);
}