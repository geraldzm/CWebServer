#include "httpServer.h"
#include "global.h"
#include "../util/tools.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>


#define MAX_ENDPOINTS 100

char isListening = 0;
int server_sockfd;

typedef struct {
    Method method;
    char* endpoint;
    void (*handler)(ResponseHttp*, RequestHttp*);
} Route;

Route routes[MAX_ENDPOINTS];
void (*notFoundHandler)(ResponseHttp*, RequestHttp*);

int routesIndex = 0;


Route* foundRoute(char* endpoint, Method method) {
    for (int i = 0; i < routesIndex; i++) {
        if (routes[i].method == method && strcmp(routes[i].endpoint, endpoint) == 0) {
            return &routes[i];
        }
    }
    return NULL;
}

void __getParams(char* params, RequestHttp* requestHttp) {
    int paramsAmount = 0;
    char paramsCopy[strlen(params) + 1];
    strcpy(paramsCopy, params);
    char* rest = paramsCopy;
    char* token;

    // count params
    while (strtok_r(rest, "&", &rest)) paramsAmount++;

    Param* paramsArray = malloc(sizeof(Param) * paramsAmount);

    rest = params;
    int tokenIndex = 0;
    while ((token = strtok_r(rest, "&", &rest))) {
        char *key;
        char *value;
        int tokenIndex2 = 0;
        char* rest2 = token;
        while ((token = strtok_r(rest2, "=", &rest2))) {
            tokenIndex2++;
            if(tokenIndex2 == 1) {
                key = token;
            } else if(tokenIndex2 == 2) {
                value = token;
                break;
            }
        }
        paramsArray[tokenIndex].key = malloc(strlen(key) + 1);
        strcpy(paramsArray[tokenIndex].key, key);
        paramsArray[tokenIndex].value = malloc(strlen(value) + 1);
        strcpy(paramsArray[tokenIndex].value, value);
        tokenIndex++;
    }

    requestHttp->params = paramsArray;
    requestHttp->paramsAmount = paramsAmount;
}

void setCookieByString_r(char* cookie, Cookie *cookieStruct) {
    // trim cookie
    char* cookieTrimmed = trim(cookie);

    char *saveptr;
    char *token = strtok_r(cookieTrimmed, "=", &saveptr);
    char *key = token;
    token = strtok_r(NULL, "=", &saveptr);
    char *value = token;

    cookieStruct->key = malloc(strlen(key) + 1);
    strcpy(cookieStruct->key, key);
    // remove \r from value
    unsigned long valueLength = strlen(value);
    if(value[valueLength - 1] == '\r') value[valueLength - 1] = '\0';
    cookieStruct->value = malloc(valueLength + 1);
    strcpy(cookieStruct->value, value);

    free(cookieTrimmed);
}

// Separate cookies by ; and create a Cookie struct for each one
// and save it in listOfCookies
void extractCookies(Cookie** listOfCookies, int *cookiesAmount, char* cookies) {
    char *saveptr = cookies;
    char *token = NULL;
    int amount = *cookiesAmount;
    while ((token = strtok_r(saveptr, ";", &saveptr))) {
        *listOfCookies = realloc(*listOfCookies, sizeof(Cookie) * (amount + 1));
        setCookieByString_r(token, &(*listOfCookies)[amount++]);
    }
    *cookiesAmount = amount;
}

RequestHttp* createRequest(char *buffy, int buffySize, int client_sockfd) {

    int tokenIndex = 0;
    char* rest = buffy;
    char method[10];
    char* endPoint = NULL;
    char* token = NULL;

    // read cookie
    Cookie* listOfCookies = NULL;
    int cookiesAmount = 0;

    Header * listOfHeaders = NULL;
    int headersAmount = 0;

    while ((token = strtok_r(rest, "\n", &rest)) != NULL) {

        if(tokenIndex == 0) {
            tokenIndex++;
            char* rest2 = token;
            char* token2 = NULL;
            token2 = strtok_r(rest2, " ", &rest2);
            strcpy(method, token2);
            token2 = strtok_r(rest2, " ", &rest2);
            endPoint = token2;
            continue;
        }

        // if the line starts with "Cookie: " then it is a cookie
        if (strstr(token, "Cookie: ") != NULL) {
            extractCookies(&listOfCookies, &cookiesAmount, token + 8);
        } else if (strcmp(token, "\r") == 0) {
            // else if token = '\r' then it is the end of the headers
            break;
        } else {
            // this is a header, so save it to listOfHeaders
            listOfHeaders = realloc(listOfHeaders, sizeof(Header) * (headersAmount + 1));
            Header *header = &listOfHeaders[headersAmount++];
            char *saveptr;
            char *token2 = strtok_r(token, ":", &saveptr);
            header->key = malloc(strlen(token2) + 1);
            strcpy(header->key, token2);
            token2 = strtok_r(NULL, ":", &saveptr);
            // remove the \r at the end of the value and the initial space of the value, then save it to the header
            unsigned long valueLength = strlen(token2);
            if(token2[valueLength - 1] == '\r') token2[valueLength - 1] = '\0';
            header->value = malloc(valueLength);
            strcpy(header->value, token2 + 1);
        }
    }

    char *params;
    rest = endPoint;
    char hasParams = 0;
    tokenIndex = 0;
    while ((token = strtok_r(rest, "?", &rest))) {
        tokenIndex++;
        if(tokenIndex == 1) {
            endPoint = token;
        } else if(tokenIndex == 2) {
            params = token;
            hasParams = 1;
            break;
        }
    }

    Method methodEnum = getMethod(method);
    if(methodEnum == UNKNOWN) {
        logError("Unknown method %s", method);
        return NULL;
    }

    // separete params
    RequestHttp *request = malloc(sizeof(RequestHttp));
    request->cookies = listOfCookies;
    request->cookiesAmount = cookiesAmount;
    request->headers = listOfHeaders;
    request->headersAmount = headersAmount;
    request->params = NULL;
    request->paramsAmount = 0;
    // set ip address as a str to ip, and the ip as a 32 bit int to ipNum, and the port to port
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(client_sockfd, (struct sockaddr *)&addr, &addr_size);
    inet_ntop(AF_INET, &(addr.sin_addr), request->ip, INET_ADDRSTRLEN);
    request->ipNum = addr.sin_addr.s_addr;
    request->port = addr.sin_port;

    if(hasParams) {
        __getParams(params, request);
    }

    // save method
    request->method = methodEnum;
    request->methodStr = malloc(strlen(method) + 1);
    strcpy(request->methodStr, method);

    // save endpoint
    request->endpoint = malloc(strlen(endPoint) + 1);
    strcpy(request->endpoint, endPoint);

    request->client_sockfd = client_sockfd;

    char ipNumStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(request->ipNum), ipNumStr, INET_ADDRSTRLEN);
    logInfo("Method: %s\tEndpoint: %s\tIP:%s",  request->methodStr, request->endpoint, ipNumStr);

//    if(request->paramsAmount > 0) {
//        logInfo("Params:");
//        for (int i = 0; i < request->paramsAmount; i++) {
//            logInfo("\t%s: %s", request->params[i].key, request->params[i].value);
//        }
//    }
//    if(request->headersAmount > 0) {
//        logInfo("Headers:");
//        for (int i = 0; i < request->headersAmount; i++) {
//            logInfo("\t..%s: ..%s..", request->headers[i].key, request->headers[i].value);
//        }
//    }

    return request;
}

void *handleConnection(void* arg) {

    int client_sockfd = *(int *)arg;

    char active = 1;
    int buffySize = 1024;
    char buffy[buffySize];
    long n;

    while (active) {

        // buffy set to 0
        memset(buffy, 0, buffySize);

        n = read(client_sockfd, &buffy, buffySize - 1);
        if (n < 0) {
            // error
            perror("read");
            active = 0;
            break;
        }

        if (n == 0) {
            // client disconnected
            active = 0;
            break;
        }

        buffy[n] = '\0';
        logInfo("Received: %s\n", buffy);


        if(n == (buffySize - 1) ) {
            logWarning("Read 1023, bytes may be lost: \n");
        }

        RequestHttp* request = createRequest(buffy, buffySize, client_sockfd);
        if(request == NULL) {
            logError("Error creating request");
            break;
        }

        Route* route = foundRoute(request->endpoint, request->method);
        ResponseHttp* response = malloc(sizeof(ResponseHttp));
        response->body = NULL;
        response->__headers = NULL;
        response->__headersAmount = 0;

        if(route == NULL) {
            notFoundHandler(response, request);
        } else {
            // call handler
            route->handler(response, request);
        }

        if(response->body != NULL) {
            free(response->body->content);
            free(response->body);
        }

        if(response->__headers != NULL) {
            for (int i = 0; i < response->__headersAmount; i++) {
                free(response->__headers[i].key);
                free(response->__headers[i].value);
            }
            free(response->__headers);
        }

        // free request
        free(request->endpoint);
        for(int i = 0; i < request->paramsAmount; i++) {
            free(request->params[i].key);
            free(request->params[i].value);
        }

        if(request->params != NULL) free(request->params);

        for(int i = 0; i < request->cookiesAmount; i++) {
            free(request->cookies[i].key);
            free(request->cookies[i].value);
        }
        if(request->cookies != NULL) free(request->cookies);

        for(int i = 0; i < request->headersAmount; i++) {
            free(request->headers[i].key);
            free(request->headers[i].value);
        }
        if(request->headers != NULL) free(request->headers);

        free(response);
        free(request);

        //char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Hello World!</h1></body></html>";
        //write(client_sockfd, response, strlen(response));

        active = 0;
    }

    close(client_sockfd);
    return NULL;
}

void stopHttpServer() {
    isListening = 1;
}

void startHttpServer(int port) {
    logInfo("Starting server...");

    //Make the necessary includes and set up the variables:
    int client_sockfd;
    unsigned int server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    //Create an unnamed socket for the server:
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        logError("error creating socket");
        exit(1);
    }

    //Name the socket:
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    int b = bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));

    if(b < 0) {
        logError("error binding socket");
        exit(1);
    }

    // Create a connection queue and wait for clients:
    if (listen(server_sockfd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    logInfo("Server listening on port %d\n", port);

    //Accept a connection:
    client_len = sizeof(client_address);

    isListening = 1;
    while (isListening) {
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        if(client_sockfd == -1) {
            logError("accepting connection");
            exit(1);
        }

        char* ip = inet_ntoa(client_address.sin_addr);
        uint16_t port = ntohs(client_address.sin_port);
//        logInfo("Request from %s:%d", ip, port);

        // Handle connection
        pthread_t thread;
        // int res = pthread_create(&thread, NULL, handleConnection, client_sockfd);
        int res = pthread_create(&thread, NULL, handleConnection, &client_sockfd);
        if(res != 0) {
            logError("Error creating thread");
            isListening = 0;
        }
    }

    logInfo("Server stopped");
}

void handleFunc(char* _endPoint, void (*_func)(ResponseHttp* _response, RequestHttp* _request), Method _method) {
    if(isListening) {
        logError("Can't add new endpoint while server is listening");
        return;
    }

    if(routesIndex >= MAX_ENDPOINTS) {
        logError("Can't add new endpoint. Max amount of endpoints reached");
        return;
    }

    routes[routesIndex].endpoint = _endPoint;
    routes[routesIndex].handler = _func;
    routes[routesIndex].method = _method;
    routesIndex++;
}

void setNotFoundHandler(void (*_func)(struct ResponseHttp* _response, struct RequestHttp* _request)) {
    notFoundHandler = _func;
}

void redirect(struct ResponseHttp* _response, struct RequestHttp* _request, char* _url) {
    _response->contentType = HTML;
    _response->statusCode = 303;

    setHeader(_response, "Location\0", _url);

    sendResponse(_response, _request);
}
