#include "connection.h"
#include "util/logger.h"

int clients_count = 0;
int server_sockfd;
char running = 1;

void loadContext() {
    logInfo("Loading context...");
}

void cleanContext() {
    logInfo("Cleaning context...");
}

void startListeningConnections() {
    logInfo("Starting server...");

}

void stopListeningConnections() {
    logInfo("Stopping listening connections...");
}

// -------------------- Connection functions --------------------

void connectClient() {

}