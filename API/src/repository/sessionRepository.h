#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_SESSIONREPOSITORY_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_SESSIONREPOSITORY_H

#include "../modal/user.h"
#include <stdlib.h>

User* findUserByUsername(char* username);
User* findUserById(unsigned int id);
User* createUser(char* username, char* password, UserType type);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_SESSIONREPOSITORY_H
