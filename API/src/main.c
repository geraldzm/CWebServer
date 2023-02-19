#include "../../httpServer/httpServer.h"
#include "repository/staticCache.h"
#include "repository/fileManager.h"
#include "routes/sessionRouter.h"
#include "routes/quizRouter.h"
#include "routes/adminRouter.h"
#include "repository/quizRepository.h"
#include "routes/staticRoutes.h"
#include "controller/quizController.h"
#include <signal.h>
#include <unistd.h>

#define PORT 2201

void handler404(ResponseHttp* response, RequestHttp* request) {

    response->contentType = HTML;
    response->statusCode = 404;

    File *staticFile = getFile("404.html");

    response->body = malloc(sizeof(File));
    response->body->fsize = staticFile->fsize + 1;
    response->body->content = malloc(response->body->fsize);
    memcpy(response->body->content, staticFile->content, response->body->fsize);

    sendResponse(response, request);
}

void welcome(ResponseHttp* response, RequestHttp* request) {

    response->contentType = HTML;
    response->statusCode = 200;

    File *staticFile = getFile("welcome.html");

    response->body = malloc(sizeof(File));
    response->body->fsize = staticFile->fsize + 1;
    response->body->content = malloc(response->body->fsize);
    memcpy(response->body->content, staticFile->content, response->body->fsize);

    // remove admin session if exists
    if(getCookie(request, "adminuser") != NULL) removeCookie(response, "adminuser");
    // remove user session if exists
    if(getCookie(request, "username") != NULL) removeCookie(response, "username");
    // remove quiz session if exists
    if(getCookie(request, "quizID") != NULL) removeCookie(response, "quizID");

    sendResponse(response, request);
}

void favicon(ResponseHttp* response, RequestHttp* request) {

    response->contentType = JPEG;
    response->statusCode = 200;

    char path[sizeof STATIC_FILES_PATH + sizeof "favicon.ico" + 1];
    mergePath(path, STATIC_FILES_PATH, "favicon.ico");
    response->body = getMediaFile(path);

    sendResponse(response, request);
}

void insertTestData() {
    // insert testing data

    // Quiz 0
    Quiz *quiz0 = createQuiz("Capitales", 1);
    logInfo("Quiz created with id %d\0", quiz0->id);

    //
    Question *question01 = createQuestion("¿Cuál es la capital de Francia?", 1, 8, "/static?media=france.png", IMAGE, SINGLE_CHOICE);
    createAnswer("Londres", 0, question01->id);
    createAnswer("París", 1, question01->id);
    createAnswer("Roma", 0, question01->id);
    createAnswer("Berlín", 0, question01->id);
    //
    Question *question02 = createQuestion("¿Cuál es la capital de China?", 2, 10, "/static?media=china.png", IMAGE, SINGLE_CHOICE);
    createAnswer("Pekín", 1, question02->id);
    createAnswer("Tokio", 0, question02->id);
    createAnswer("Seúl", 0, question02->id);
    createAnswer("Nueva York", 0, question02->id);
    //
    Question *question03 = createQuestion("¿Cuál es la capital de Jamaica?", 4, 15, "/static?media=jamaica.png", IMAGE, SINGLE_CHOICE);
    createAnswer("La Paz", 0, question03->id);
    createAnswer("Hanoi", 0, question03->id);
    createAnswer("Kingston", 1, question03->id);
    createAnswer("Cairo", 0, question03->id);
    //
    Question *question04 = createQuestion("¿Cuál es la capital de Australia?", 3, 10, "/static?media=australia.png", IMAGE, SINGLE_CHOICE);
    createAnswer("Canberra", 1, question04->id);
    createAnswer("Sidney", 0, question04->id);
    createAnswer("Brisbane", 0, question04->id);
    createAnswer("Buenos Aires", 0, question04->id);
    //

    addQuestionToQuiz(quiz0->id, question01->id);
    addQuestionToQuiz(quiz0->id, question02->id);
    addQuestionToQuiz(quiz0->id, question03->id);
    addQuestionToQuiz(quiz0->id, question04->id);

    // create quiz room
    startQuizRoom(quiz0->id);

    // Quiz 1
    Quiz *quiz1 = createQuiz("Continentes", 1);
    logInfo("Quiz created with id %d\0", quiz1->id);

    //
    Question *question11 = createQuestion("¿En cuál es la continente se encuentra Jamaica?", 4, 10, "/static?media=jamaica.png", IMAGE, SINGLE_CHOICE);
    createAnswer("America", 1, question11->id);
    createAnswer("Europa", 0, question11->id);
    createAnswer("Asia", 0, question11->id);
    //
    Question *question12 = createQuestion("¿En cuál es la continente se encuentra Russia?",  5, 10, "/static?media=rusia.png", IMAGE, MULTIPLE_CHOICE);
    createAnswer("Europa", 1, question12->id);
    createAnswer("America", 0, question12->id);
    createAnswer("Asia", 1, question12->id);
    //
    Question *question13 = createQuestion("¿En cuál es la continente se encuentra Italia?",  2, 10, "/static?media=italy.png",IMAGE, SINGLE_CHOICE);
    createAnswer("America", 0, question13->id);
    createAnswer("Asia", 0, question13->id);
    createAnswer("Europa", 1, question13->id);
    //
    Question *question14 = createQuestion("¿En cuál es la continente se encuentra China?", 2, 10, "/static?media=china.png",IMAGE, SINGLE_CHOICE);
    createAnswer("Asia", 1, question14->id);
    createAnswer("Europa", 0, question14->id);
    createAnswer("America", 0, question14->id);
    //

    addQuestionToQuiz(quiz1->id, question11->id);
    addQuestionToQuiz(quiz1->id, question12->id);
    addQuestionToQuiz(quiz1->id, question13->id);
    addQuestionToQuiz(quiz1->id, question14->id);

    // create quiz room
    startQuizRoom(quiz1->id);
}

void initRepositories() {
    initQuizRepository();
}

void initControllers() {
    initQuizController();
}

void destroyRepositories() {
    destroyQuizRepository();
}

void sigintHandler(int sig_num) {
    logWarning("SIG %d received. Stopping server...", sig_num);

    logWarning("flushing static files...");
    flushStaticFiles();
    logWarning("cleaning resources...");
    destroyRepositories();

    logWarning("Server stopped.");
    exit(0);
}

int main() {

    // signals handling
    signal(SIGINT, sigintHandler); // ctrl + c
    signal(SIGTERM, sigintHandler); // kill
    signal(SIGHUP, sigintHandler); // kill -HUP

    // set up the server
    loadLogFileName();
    initRepositories();
    initControllers();
    loadStaticFiles();

    insertTestData();

    // standard routes
    handleFunc("/", &welcome, GET);
    handleFunc("/favicon.ico", &favicon, GET);

    // session routes
    handleFunc("/join", &joinQuizRequest, GET);
    handleFunc("/web-socket", &joinQuizSocket, GET);

    // admin routes
    handleFunc("/login", &logIn, GET);
    handleFunc("/admin", &adminDashboard, GET);

    handleFunc("/admin/statistics", &adminStatistics, GET);
    handleFunc("/admin/statistics/info", &adminJSONStatisticsOfQuiz, GET);

    handleFunc("/admin/quiz/init", &adminInitQuiz, GET); // html for creating a quiz

    // QUIZ CRUD
    handleFunc("/admin/create-quiz", &adminCreateQuizHTML, GET); // html for creating a quiz
    handleFunc("/admin/quiz/create", &adminCreateQuiz, GET); // endpoint for saving the quiz
    handleFunc("/admin/question", &adminQuestion, GET);
    handleFunc("/admin/questions", &adminQuestions, GET); // returns a Json with all the questions in the bank
    handleFunc("/admin/question/new", &adminCreateQuestion, GET);

    //
    handleFunc("/quizzes", &quizzes, GET);

    // static routes
    handleFunc("/static", &serveStaticFiles, GET);

    // quiz routes
    handleFunc("/quiz", &quizInfo, GET);

    // 404
    setNotFoundHandler(&handler404);

    // start server
    startHttpServer(PORT);

    // free static files
    flushStaticFiles();
    destroyRepositories();

    return 0;
}
