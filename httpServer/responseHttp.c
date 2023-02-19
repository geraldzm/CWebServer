#include "responseHttp.h"
#include "requestHttp.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void setHeader(ResponseHttp* response, char* key, char* value) {

    // resize headers array
    response->__headers = realloc(response->__headers, sizeof(Header) * (response->__headersAmount + 1));

    // set header
    response->__headers[response->__headersAmount].key = malloc(strlen(key) + 1);
    strcpy(response->__headers[response->__headersAmount].key, key);
    response->__headers[response->__headersAmount].value = malloc(strlen(value) + 1);
    strcpy(response->__headers[response->__headersAmount].value, value);
    response->__headersAmount++;
}

void setCookie(ResponseHttp* response, char* key, char* value) {

    unsigned long totalLength = strlen(key) + strlen(value) + 2;

    char cookie[totalLength];

    strcpy(cookie, key);
    strcat(cookie, "=");
    strcat(cookie, value);

    setHeader(response, "Set-Cookie", cookie);
}

void removeCookie(ResponseHttp* response, char* key) {

    unsigned long totalLength = strlen(key) + strlen("=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;") + 1;

    char cookie[totalLength];

    strcpy(cookie, key);
    strcat(cookie, "=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;");

    setHeader(response, "Set-Cookie", cookie);
}

void getContentType(char* contentType, ContentType type) {
    switch (type) {
        case JSON:
            strcat(contentType, "Content-Type: application/json\r\n");
            break;
        case HTML:
            strcat(contentType, "Content-Type: text/html\r\n");
            break;
        case TEXT_PLAIN:
            strcat(contentType, "Content-Type: text/plain\r\n");
            break;
        case JPEG:
            strcat(contentType, "Content-Type: image/jpeg\r\n");
            break;
        case JS:
            strcat(contentType, "Content-Type: text/javascript\r\n");
            break;
    }
}

void getStatusCode(char* statusCodeString, unsigned int statusCode) {
    switch (statusCode) {
        case 101:
            strcat(statusCodeString, "101 Switching Protocols\r\n");
            break;
        case 301:
            strcat(statusCodeString, "301 Moved Permanently\r\n");
            break;
        case 302:
            strcat(statusCodeString, "302 Found\r\n");
            break;
        case 303:
            strcat(statusCodeString, "303 See Other\r\n");
            break;
        case 200:
            strcpy(statusCodeString, "200 OK\r\n");
            break;
        case 201:
            strcpy(statusCodeString, "201 Created\r\n");
            break;
        case 202:
            strcpy(statusCodeString, "202 Accepted\r\n");
            break;
        case 204:
            strcpy(statusCodeString, "204 No Content\r\n");
            break;
        case 400:
            strcpy(statusCodeString, "400 Bad Request\r\n");
            break;
        case 401:
            strcpy(statusCodeString, "401 Unauthorized\r\n");
            break;
        case 403:
            strcpy(statusCodeString, "403 Forbidden\r\n");
            break;
        case 404:
            strcpy(statusCodeString, "404 Not Found\r\n");
            break;
        case 405:
            strcpy(statusCodeString, "405 Method Not Allowed\r\n");
            break;
        case 406:
            strcpy(statusCodeString, "406 Not Acceptable\r\n");
            break;
        case 409:
            strcpy(statusCodeString, "409 Conflict\r\n");
            break;
        case 500:
            strcpy(statusCodeString, "500 Internal Server Error\r\n");
            break;
        case 501:
            strcpy(statusCodeString, "501 Not Implemented\r\n");
            break;
        case 503:
            strcpy(statusCodeString, "503 Service Unavailable\r\n");
            break;
        case 504:
            strcpy(statusCodeString, "504 Gateway Timeout\r\n");
            break;
        default:
            strcpy(statusCodeString, "500 Internal Server Error\r\n");
            break;
    }
}

void getGeneralHeaders(char* generalHeaders, unsigned int statusCode, ContentType contentType) {
    getStatusCode(generalHeaders, statusCode);
    getContentType(generalHeaders, contentType);
//    strcat(generalHeaders, "Connection: close\r\n");
    strcat(generalHeaders, "Server: C-HTTP-Server\r\n");
//    strcat(generalHeaders, "Accept-Encoding: identity\r\n");
//    strcat(generalHeaders, "Transfer-Encoding: chunked\r\n");
}

void sendResponse(ResponseHttp* response, RequestHttp* request) {
    
    char headers[512];
    memset(headers, 0, 512);
    getGeneralHeaders(headers, response->statusCode, response->contentType);

    write(request->client_sockfd, "HTTP/1.1 ", 9);
    write(request->client_sockfd, headers, strlen(headers));

    for(int i = 0; i < response->__headersAmount; i++) {
        write(request->client_sockfd, response->__headers[i].key, strlen(response->__headers[i].key));
        write(request->client_sockfd, ": ", 2);
        write(request->client_sockfd, response->__headers[i].value, strlen(response->__headers[i].value));
        write(request->client_sockfd, "\r\n", 2);
    }

    write(request->client_sockfd, "\r\n", 2);

    if(response->body != NULL && response->body->fsize > 0) {
        write(request->client_sockfd, response->body->content, response->body->fsize);
    }

}

void sendResponse200HTML(ResponseHttp* response, RequestHttp* request, File *staticFile) {
    response->statusCode = 200;
    response->contentType = HTML;
    if(staticFile != NULL) {
        response->body = malloc(sizeof(File));
        response->body->fsize = staticFile->fsize + 1;
        response->body->content = malloc(response->body->fsize);
        memcpy(response->body->content, staticFile->content, response->body->fsize);
    }
    sendResponse(response, request);
}

void sendResponse400(ResponseHttp* response, RequestHttp* request, char* message) {
    logWarning(message);
    response->statusCode = 400;
    response->contentType = TEXT_PLAIN;
    sendResponse(response, request);
}

void sendResponse500(ResponseHttp* response, RequestHttp* request, char* message) {
    logWarning(message);
    response->statusCode = 500;
    response->contentType = TEXT_PLAIN;
    sendResponse(response, request);
}

char* getParam(RequestHttp* request, char* param) {

    for(int i = 0; i < request->paramsAmount; i++) {
        if(strcmp(request->params[i].key, param) == 0) {
            return request->params[i].value;
        }
    }

    return NULL;
}

char* getCookie(RequestHttp* request, char* key) {

    for(int i = 0; i < request->cookiesAmount; i++) {
        if(strcmp(request->cookies[i].key, key) == 0) {
            return request->cookies[i].value;
        }
    }

    return NULL;
}

char* getHeader(RequestHttp* request, char* key) {

    for(int i = 0; i < request->headersAmount; i++) {
        if(strcmp(request->headers[i].key, key) == 0) {
            return request->headers[i].value;
        }
    }

    return NULL;
}