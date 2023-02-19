#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_LOGGER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_LOGGER_H

#include <stdio.h>
#include <string.h>
#include <bits/types/time_t.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum {
    INFO,
    WARNING,
    ERROR
} LogLevel;


void logInfo(char* message, ...);
void logWarning(char* message, ...);
void logError(char* message, ...);
void loadLogFileName();

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_LOGGER_H
