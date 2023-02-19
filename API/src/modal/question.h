#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_QUESTION_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_QUESTION_H

typedef struct Answer {
    unsigned int id;
    unsigned int questionId;
    char* text;
    char correct; // true or false (1 or 0)
} Answer;

typedef enum {
    AUDIO,
    IMAGE,
    VIDEO,
    NONE
} QuestionMediaType;

typedef enum {
    SINGLE_CHOICE,
    MULTIPLE_CHOICE,
} QuestionType;

typedef struct Question {
    unsigned int id;

    char* question;
    unsigned int points;
    unsigned int time; // duration in seconds

    char* mediaSrc;
    QuestionMediaType mediaType;

    QuestionType type;
} Question;

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_QUESTION_H
