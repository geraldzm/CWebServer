#include "adminRouter.h"
#include "../repository/staticCache.h"
#include "../repository/quizRepository.h"
#include "../../../httpServer/httpServer.h"
#include "../../../util/tools.h"
#include "../controller/quizController.h"

int getSession(ResponseHttp* response, RequestHttp* request, char** username) {
    *username = getCookie(request, "adminuser");

    if (*username == NULL) {
        logWarning("some one try to enter as admin without being logged in");
        setCookie(response, "message", "You are not logged in as Admin");
        redirect(response, request, "/");
        return 0;
    }

    return 1;
}

void adminStatistics(ResponseHttp* response, RequestHttp* request) {
    logInfo("admin statistics request");

    // validate session
    char *username;
    if (!getSession(response, request, &username)) return;

    // find quiz code
    char* quizcode = getParam(request, "quizcode");
    if (quizcode == NULL) {
        logWarning("quizcode was not found");
        setCookie(response, "message", "No quiz was found with that code");
        redirect(response, request, "/admin");
        return;
    }

    setCookie(response, "quizcode", quizcode);

    // find html file admin-statistics-quiz.html
    File* staticFile = getFile("admin-statistics-quiz.html");
    sendResponse200HTML(response, request, staticFile);

}

void adminJSONStatisticsOfQuiz(ResponseHttp* response, RequestHttp* request) {

    // validate session
    char *username;
    if (!getSession(response, request, &username)) return;

    // find quiz code on cookie
    char* quizcode = getCookie(request, "quizcode");
    if (quizcode == NULL) {
        logWarning("quizcode was not found");
        setCookie(response, "message", "No quiz was found with that code");
        redirect(response, request, "/admin");
        return;
    }

    unsigned int quizID;
    int ok = parseStrToUnsInt(quizcode, &quizID);
    if (!ok) {
        logWarning("quizcode is not a number");
        removeCookie(response, "quizcode");
        setCookie(response, "message", "No quiz was found with that code");
        redirect(response, request, "/admin");
        return;
    }

    // get statistics of quiz
    char* json = getStatisticsOfQuiz(quizID);
    if (json == NULL) {
        logWarning("quiz with id %d was not found", quizID);
        setCookie(response, "message", "Quiz not found");
        redirect(response, request, "/admin");
        return;
    }

    // send response
    response->statusCode = 200;
    response->contentType = JSON;
    response->body = malloc(sizeof(File));
    response->body->fsize = strlen(json);
    response->body->content = json;

    sendResponse(response, request);
}

void adminDashboard(ResponseHttp* response, RequestHttp* request) {
        // log the request params
        logInfo("Admin dashboard request");

        char *username;
        if (!getSession(response, request, &username)) return;

        logInfo("Admin user name: %s", username);

        // get file admin-dashboard.html
        File *staticFile = getFile("admin-dashboard.html");
        sendResponse200HTML(response, request, staticFile);
}

void adminCreateQuizHTML(ResponseHttp* response, RequestHttp* request) {
    logInfo("Admin create quiz request");

    // validate session
    char *username;
    if (!getSession(response, request, &username)) return;

    // get create-quiz.html
    File *staticFile = getFile("admin-create-quiz.html");
    sendResponse200HTML(response, request, staticFile);
}


void adminCreateQuiz(ResponseHttp* response, RequestHttp* request) {
    // http://localhost:7906/admin/quiz?quizName=asdf&question0=0
    logInfo("Admin create quiz request");

    // validate session
    char *username;
    if (!getSession(response, request, &username)) return;

    // get quiz name
    char* quizName = getParam(request, "quizName");
    if (quizName == NULL) {
        logWarning("No name was found for this quiz");
        setCookie(response, "message", "Quiz name was not found");
        redirect(response, request, "/admin/create-quiz");
        return;
    }

    // get questions
    unsigned int questionsAmount = 0;
    unsigned int questionsIds[100];
    for(int i = 0; i < request->paramsAmount; i++) {
        // look if request->params[i].key starts with "question"
        if (strncmp(request->params[i].key, "question", 8) == 0) {
            // parse question id
            int ok = parseStrToUnsInt(request->params[i].value, &questionsIds[questionsAmount]);
            if (!ok) {
                logWarning("question id is not a number");
                setCookie(response, "message", "Question id is not a number");
                redirect(response, request, "/admin/create-quiz");
                return;
            }
            questionsAmount++;
        }
    }

    // test prints
    logInfo("Quiz name: %s", quizName);
    for(int i = 0; i < questionsAmount; i++) {
        logInfo("question id: %d", questionsIds[i]);
    }

    // create quiz
    unsigned int  quizID;
    char *quizNameDecoded = url_decode(quizName);
    createQuizFromQuestions(quizNameDecoded, questionsIds, questionsAmount, &quizID);
    free(quizNameDecoded);

    // redirect to admin dashboard
    char message[50];
    sprintf(message, "Quiz created with id %d", quizID);
    setCookie(response, "message", message);
    redirect(response, request, "/admin");
}

void adminCreateQuestion(ResponseHttp* response, RequestHttp* request) {
    logInfo("Admin create question request");

    // validate session
    char *username;
    if (!getSession(response, request, &username)) {
        return;
    }

    response->contentType = HTML;

    // find params
    char *question = getParam(request, "questionText");
    char *time = getParam(request, "time");
    char *points = getParam(request, "points");

    if (question == NULL || time == NULL || points == NULL) {
        logWarning("Missing params");
        response->statusCode =  400; // bad request
        sendResponse(response, request);
        return;
    }

    // find answers
    int answerIndex = 0;
    int amountOfCorrectAnswers = 0;
    char *answers[100];
    char correct[100];

    char answerText[14];
    char answerCorrect[17];
    char answerIndexText[4];
    // clean memory
    memset(answerText, 0, 14);
    memset(answerCorrect, 0, 17);
    memset(answerIndexText, 0, 4);

    strncpy(answerText, "answerText", 10);
    strncpy(answerCorrect, "answerCorrect", 13);

    while(1) {
        // concat answerText with answerIndex
        answerText[10] = '\0';
        answerCorrect[13] = '\0';

        sprintf(answerIndexText, "%d", answerIndex);
        strcat(answerCorrect, answerIndexText);
        strcat(answerText, answerIndexText);

        char *answer = getParam(request, answerText);
        if (answer == NULL) break;

        if(getParam(request, answerCorrect) == NULL) {
            correct[answerIndex] = 0;
            amountOfCorrectAnswers++;
        }
        else correct[answerIndex] = 1;

        answers[answerIndex++] = url_decode(answer);
    }

    // logs
    char* questionDecoded = url_decode(question);
    logInfo("Question: %s", questionDecoded);
    for (int i = 0; i < answerIndex; i++) {
        logInfo("Answer %d: %s", i, answers[i]);
        logInfo("Correct %d: %d", i, correct[i]);
    }

    // create question
    QuestionType type = amountOfCorrectAnswers == 1 ? SINGLE_CHOICE : MULTIPLE_CHOICE;
    Question *questionObj = createQuestion(questionDecoded, atoi(points), atoi(time), NULL, NONE, type);
    free(questionDecoded);

    // create answers
    for (int i = 0; i < answerIndex; i++) {
        createAnswer(answers[i], correct[i], questionObj->id);
        free(answers[i]);
    }

    redirect(response, request, "/admin");
}

void adminQuestions(ResponseHttp* response, RequestHttp* request) {
    // validate session
    char *username;
    if (!getSession(response, request, &username)) return;

    // get questions
    char* json = getQuestions();
    if (json == NULL) {
        logWarning("No questions found");
        setCookie(response, "message", "No questions found");
        return;
    }

    // send response
    response->statusCode = 200;
    response->contentType = JSON;
    response->body = malloc(sizeof(File));
    response->body->fsize = strlen(json);
    response->body->content = json;

    sendResponse(response, request);
}

void adminQuestion(ResponseHttp* response, RequestHttp* request) {
    logInfo("Admin question html request");

    // validate session
    char *username;
    if (!getSession(response, request, &username)) return;

    // get admin-create-question.html
    File *staticFile = getFile("admin-create-question.html");
    response->body = malloc(sizeof(File));
    response->body->fsize = staticFile->fsize + 1;
    response->body->content = malloc(response->body->fsize + 1);
    memcpy(response->body->content, staticFile->content, staticFile->fsize);
    response->body->content[response->body->fsize - 1] = '\0';

    response->statusCode = 200;
    response->contentType = HTML;

    sendResponse(response, request);
}

void adminStartQuiz(ResponseHttp* response, RequestHttp* request) {
    logInfo("Admin start quiz request");

    // validate session
    char *username;
    if (!getSession(response, request, &username)) {
        return;
    }

    char *quizId = getParam(request, "quizId");
    if (quizId == NULL) {
        sendResponse400(response, request, "Quiz id is NULL");
        return;
    }

    unsigned int quizIdInt;
    int ok = parseStrToUnsInt(quizId, &quizIdInt);
    if (!ok) {
        sendResponse400(response, request, "Quiz id is not a number");
        return;
    }

    // start the quiz
    ok = startQuizRoom(quizIdInt);
    if (!ok) {
        logWarning("Could not start the quiz");
        setCookie(response, "message", "Error: Could not start the quiz");
        redirect(response, request, "/admin");
        return;
    }

    logInfo("Quiz %d started\0", quizIdInt);

    // redirect to the admin page
    redirect(response, request, "/admin");
}

void adminInitQuiz(ResponseHttp* response, RequestHttp* request) {
    logInfo("Admin init quiz request");

    // validate session
    char *username;
    if (!getSession(response, request, &username)) return;

    char *quizId = getCookie(request, "quizcode");
    if (quizId == NULL) {
        sendResponse400(response, request, "No cookie quizcode was found");
        return;
    }

    unsigned int quizIdInt;
    int ok = parseStrToUnsInt(quizId, &quizIdInt);
    if (!ok) {
        sendResponse400(response, request, "Quiz id is not a number");
        return;
    }

    // init the quiz
    ok = initQuizRoom(quizIdInt);
    if (!ok) {
        logWarning("Could not init the quiz");
        setCookie(response, "message", "Error: Could not init the quiz");
        sendResponse500(response, request, "Could not init the quiz");
        return;
    }

    response->statusCode = 200;
    response->contentType = TEXT_PLAIN;

    sendResponse(response, request);
}
