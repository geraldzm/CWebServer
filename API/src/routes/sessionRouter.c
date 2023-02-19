#include "sessionRouter.h"
#include "../../../httpServer/httpServer.h"
#include "../../../util/tools.h"
#include "../../../util/base64.h"
#include "../../../httpServer/webSocket/webSocketConnection.h"
#include <unistd.h>


void logIn(ResponseHttp* response, RequestHttp* request) {
    logInfo("Log as admin request");

    // log the request params
    char *adminuser = getParam(request, "adminuser");
    char *password = getParam(request, "password");

    if (adminuser == NULL || password == NULL) {
        redirect(response, request, "/");
        return;
    }

    if (strcmp(adminuser, "admin") == 0 && strcmp(password, "123") == 0) {
        setCookie(response, "adminuser", adminuser);
        redirect(response, request, "/admin");
        return;
    }

    setCookie(response, "message", "Invalid credentials\0");
    redirect(response, request, "/");
}