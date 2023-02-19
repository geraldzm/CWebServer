#include "logger.h"

char logFileName[69];

char* getLevelString(LogLevel level) {
    switch (level) {
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

void getTimeSpam(char *buffer) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buffer, "%d-%d-%d--%d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void loadLogFileName() {
    logFileName[0] = '/';
    logFileName[1] = 't';
    logFileName[2] = 'm';
    logFileName[3] = 'p';
    logFileName[4] = '/';
    getTimeSpam(logFileName+5);
    logInfo("Name: %s", logFileName);
}

void __log(char* message, LogLevel level, char* color) {
    char* levelString = getLevelString(level);
    char timeString[64];
    getTimeSpam(timeString);
    // print with color, then reset color
    printf("%s %s %s: %s %s\n", color, timeString, levelString, message, "\033[0m");
    unsigned long length = snprintf( NULL, 0, "echo %s >> %s", message, logFileName);
    char command[length + 1];
    sprintf( command, "echo \"%s\" >> %s", message, logFileName);

    system(command);
}

void logInfo(char* message, ...) {

    va_list args;
    va_start(args, message);
    char formattedMessage[sizeof(message) + 1024];
    vsprintf(formattedMessage, message, args);
    va_end(args);
    __log(formattedMessage, INFO, "\033[0;32m");

}

void logError(char* message, ...) {

    char* levelString = getLevelString(ERROR);
    char timeString[64];
    getTimeSpam(timeString);

    va_list args;
    va_start(args, message);
    char formattedMessage[sizeof(message) + 512];
    vsprintf(formattedMessage, message, args);
    va_end(args);

    char logBuffer[1024];
    snprintf(logBuffer, sizeof(logBuffer) ,"%s %s: %s", timeString, levelString, formattedMessage);

    perror(logBuffer);
}

void logWarning(char* message, ...) {
    va_list args;
    va_start(args, message);
    char formattedMessage[sizeof(message) + 512];
    vsprintf(formattedMessage, message, args);
    va_end(args);
    __log(formattedMessage, WARNING, "\033[3;43;30m");
}