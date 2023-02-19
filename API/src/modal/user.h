#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_USER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_USER_H

typedef enum {
    ADMIN,
    USER
} UserType;

typedef struct User {
    unsigned int id;
    char* username;
    char* password;
    UserType type;
} User;

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_USER_H
