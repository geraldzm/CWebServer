#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_GLOBAL_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_GLOBAL_H

typedef enum {
    JSON,
    HTML,
    JS,
    TEXT_PLAIN,
    JPEG,
} ContentType;

typedef struct {
    char *key;
    char *value;
} Header;

typedef enum {
    GET,
    POST,
    PUT,
    DELETE,
    UNKNOWN
} Method;

Method getMethod(char *method);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_GLOBAL_H
