#include "sessionRepository.h"
#include "../../../util/logger.h"
#include <semaphore.h>

User* users[500];
unsigned int userCount = 0;
sem_t createUserSem;


User* findUserByUsername(char* username) {
    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i]->username, username) == 0) {
            return users[i];
        }
    }
    return NULL;
}

User* findUserById(unsigned int id) {
    if (id >= userCount) {
        return NULL;
    }
    return users[id];
}

User* createUser(char* username, char* password, UserType type) {

    if (userCount >= 500) {
        logError("User limit reached\0");
        return NULL;
    }

    // use semaphores to protect the users array
    sem_wait(&createUserSem);

    // search for user with same username
    User* otherUser = findUserByUsername(username);
    if (otherUser != NULL) {
        logError("User with same username already exists\0");
        sem_post(&createUserSem);
        return NULL;
    }

    User* user = malloc(sizeof(User));
    user->id = userCount;
    user->username = malloc(strlen(username) + 1);
    strcpy(user->username, username);
    user->password = malloc(strlen(password) + 1);
    strcpy(user->password, password);
    user->type = type;

    users[userCount++] = user;

    // release the semaphore
    sem_post(&createUserSem);

    return user;
}