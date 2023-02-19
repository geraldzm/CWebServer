#include "staticRoutes.h"
#include "../repository/staticCache.h"

void __serveFile(ResponseHttp* response, RequestHttp* request, char* fileName) {

    File *js = getFile(fileName);

    if (js == NULL) {
        logWarning("%s not found", fileName);
        response->statusCode = 404;
        response->contentType = HTML;
        sendResponse(response, request);
        return;
    }

    response->statusCode = 200;
    response->contentType = JS;
    response->body = malloc(sizeof(File));
    response->body->fsize = js->fsize;
    response->body->content = malloc(response->body->fsize);
    memcpy(response->body->content, js->content, js->fsize);

    sendResponse(response, request);
}

void __serveMedia(ResponseHttp* response, RequestHttp* request, char* mediaName) {

    response->contentType = JPEG;
    response->statusCode = 200;

    char path[sizeof STATIC_IMAGES + sizeof mediaName + 1];
    mergePath(path, STATIC_IMAGES, mediaName);
    response->body = getMediaFile(path);

    if(response->body == NULL) {
        logWarning("%s not found", mediaName);
        response->statusCode = 404;
        response->contentType = HTML;
        sendResponse(response, request);
        return;
    }

    sendResponse(response, request);
}

void serveStaticFiles(ResponseHttp* response, RequestHttp* request) {
    char *fileName = getParam(request, "file");
    char *media = getParam(request, "media");

    if (fileName == NULL && media == NULL) {
        logWarning("file param not found");
        response->statusCode = 404;
        response->contentType = HTML;
        sendResponse(response, request);
        return;
    }

    if (fileName != NULL) {
        __serveFile(response, request, fileName);
    } else if (media != NULL) {
        __serveMedia(response, request, media);
    }

}