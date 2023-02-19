#include "quizRepository.h"
#include <semaphore.h>
#include <string.h>
#include <stdio.h>

Question *questionsDB[500];
unsigned int questionCount = 0;
sem_t questionCountSem;

QuizQuestion *quizQuestionsDB[1000];
unsigned int quizQuestionCount = 0;
sem_t quizQuestionCountSem;

Answer *answersDB[500];
unsigned int answerCount = 0;
sem_t answerCountSem;

Quiz *quizzesDB[500];
unsigned int quizCount = 0;
sem_t quizCountSem;

//// full CRUD for Question

Question *findAllQuestions(unsigned int *amount) {

    sem_wait(&questionCountSem);
    if(questionCount == 0) {
        sem_post(&questionCountSem);
        return NULL;
    }
    Question *questions = malloc(sizeof(Question) * questionCount);
    for (int i = 0; i < questionCount; i++) {
        questions[i] = *questionsDB[i];
    }
    *amount = questionCount;
    sem_post(&questionCountSem);

    return questions;
}

Question* findQuestionById(unsigned int id) {
    if(id > questionCount) {
        return NULL;
    }
    return questionsDB[id];
}

Question* createQuestion(char* question, unsigned int points, unsigned int time, char* mediaSrc, QuestionMediaType mediaType, QuestionType type) {
    Question* newQuestion = malloc(sizeof(Question));

    sem_wait(&questionCountSem);
    newQuestion->id = questionCount;
    questionsDB[questionCount++] = newQuestion;
    sem_post(&questionCountSem);

    newQuestion->question = malloc(strlen(question) + 1);
    strcpy(newQuestion->question, question);
    newQuestion->points = points;
    newQuestion->time = time;

    if(mediaType != NONE) {
        newQuestion->mediaSrc = malloc(strlen(mediaSrc) + 1);
        strcpy(newQuestion->mediaSrc, mediaSrc);
    } else {
        newQuestion->mediaSrc = NULL;
    }

    newQuestion->mediaType = mediaType;
    newQuestion->type = type;

    return newQuestion;
}

//Question* updateQuestion(unsigned int id, char* question, Answer* answers, unsigned int points);
//void deleteQuestion(unsigned int id);

//// full CRUD for Answer
Answer* findAnswerById(unsigned int id) {
    if(id > answerCount) {
        return NULL;
    }
    return answersDB[id];
}


Answer* createAnswer(char* text, char correct, unsigned int questionId) {
    Answer* newAnswer = malloc(sizeof(Answer));

    sem_wait(&answerCountSem);
    newAnswer->id = answerCount;
    answersDB[answerCount++] = newAnswer;
    sem_post(&answerCountSem);

    newAnswer->text = malloc(strlen(text) + 1);
    strcpy(newAnswer->text, text);
    newAnswer->correct = correct;
    newAnswer->questionId = questionId;

    return newAnswer;
}

//Answer* updateAnswer(unsigned int id, char* text, char correct);
//void deleteAnswer(unsigned int id);

//// full CRUD for Quiz
Quiz *findAllQuizzes(unsigned int *size) {
    sem_wait(&quizCountSem);
    if(quizCount == 0) {
        sem_post(&quizCountSem);
        return NULL;
    }
    Quiz *quizzes = malloc(sizeof(Quiz) * quizCount);
    for (int i = 0; i < quizCount; i++) {
        quizzes[i] = *quizzesDB[i];
    }
    *size = quizCount;
    sem_post(&quizCountSem);

    return quizzes;
}

Quiz* findQuizById(unsigned int id) {
    if(id > quizCount) {
        return NULL;
    }
    return quizzesDB[id];
}

void updateQuizStatus(unsigned int id, QuizStatus status) {
    Quiz* quiz = findQuizById(id);
    sem_wait(&quiz->quizSem);
    quiz->status = status;
    sem_post(&quiz->quizSem);
}

Quiz* createQuiz(char* name, unsigned int createdByUserId) {
    Quiz* newQuiz = malloc(sizeof(Quiz));

    sem_wait(&quizCountSem);
    newQuiz->id = quizCount;
    quizzesDB[quizCount++] = newQuiz;
    sem_post(&quizCountSem);

    newQuiz->name = malloc(strlen(name) + 1);
    strcpy(newQuiz->name, name);
    newQuiz->createdByUserId = createdByUserId;
    newQuiz->status = QUIZ_CREATED;
    sem_init(&newQuiz->quizSem, 0, 1);

    return newQuiz;
}

int addQuestionToQuiz(unsigned int quizId, unsigned int questionId) {
    Quiz* quiz = findQuizById(quizId);
    if(quiz == NULL) {
        return 0;
    }

    Question* question = findQuestionById(questionId);
    if(question == NULL) {
        return 0;
    }

    QuizQuestion* quizQuestion = malloc(sizeof(QuizQuestion));

    sem_wait(&quizQuestionCountSem);
    quizQuestion->id = quizQuestionCount;
    quizQuestion->quizId = quizId;
    quizQuestion->questionId = questionId;
    quizQuestionsDB[quizQuestionCount++] = quizQuestion;
    sem_post(&quizQuestionCountSem);

    return 1;
}

unsigned int* findQuestionsByQuizId(unsigned int quizId, unsigned int *amount) {
    sem_wait(&quizQuestionCountSem);

    // count the amount of answers
    int i;
    *amount = 0;
    for(i = 0; i < quizQuestionCount; i++) {
        if(quizQuestionsDB[i]->quizId == quizId) {
            *amount += 1;
        }
    }

    // create the array of pointers
    unsigned int* questions = malloc(sizeof(int) * (*amount));
    // fill the array
    int j = 0;
    for(i = 0; i < quizQuestionCount; i++) {
        if(quizQuestionsDB[i]->quizId == quizId) {
            questions[j++] = quizQuestionsDB[i]->questionId;
        }
    }

    sem_post(&quizQuestionCountSem);

    return questions;
}


Answer** findAnswersByQuestionId(unsigned int questionId, unsigned int *amount) {

    sem_wait(&answerCountSem);

    // count the amount of answers
    *amount = 0;
    for(int i = 0; i < answerCount; i++) {
        if(answersDB[i]->questionId == questionId) {
            (*amount) = 1 + (*amount);
        }
    }

    if(*amount == 0) {
        sem_post(&answerCountSem);
        return NULL;
    }

    // create the array of pointers
    Answer** answers = malloc(sizeof(Answer*) * (*amount));
    // fill the array
    int j = 0;
    for(int i = 0; i < answerCount; i++) {
        if(answersDB[i]->questionId == questionId) {
            answers[j++] = answersDB[i];
        }
    }

    sem_post(&answerCountSem);

    return answers;
}

void questionMediaTypeToStr(QuestionMediaType type, char* str) {
    switch(type) {
        case NONE:
            strcpy(str, "none");
            break;
        case IMAGE:
            strcpy(str, "image");
            break;
        case VIDEO:
            strcpy(str, "video");
            break;
        case AUDIO:
            strcpy(str, "audio");
            break;
    }
}

void questionTypeToStr(QuestionType type, char* str) {
    switch(type) {
        case SINGLE_CHOICE:
            strcpy(str, "SINGLE_CHOICE");
            break;
        case MULTIPLE_CHOICE:
            strcpy(str, "MULTIPLE_CHOICE");
            break;
    }
}

char* questionAndAnswerToJson(Question* question,  Answer** answers, unsigned int amountOfAnswers) {
    char* json = malloc(2048);

    sprintf(json, "{\"id\":%d,\"question\":\"%s\",\"points\":%d,\"time\":%d", question->id, question->question, question->points, question->time);

    char mediaType[20];
    questionTypeToStr(question->type, mediaType);
    sprintf(json, "%s,\"questionType\":\"%s\"", json, mediaType);

    // create type
    if(question->mediaType != NONE) {
        char type[10];
        questionMediaTypeToStr(question->mediaType, type);
        sprintf(json, "%s,\"%s\":\"%s\"", json, type, question->mediaSrc);
    }

    sprintf(json, "%s,\"answers\":[", json);

    for(int i = 0; i < amountOfAnswers; i++) {
        Answer* answer = answers[i];
        if(answer != NULL) {
            sprintf(json, "%s{\"id\":%d,\"text\":\"%s\"}", json, answer->id, answer->text);
            if(i < amountOfAnswers - 1) {
                sprintf(json, "%s,", json);
            }
        }
    }
    sprintf(json, "%s]}", json);
    return json;
}

char isAnswerCorrect(unsigned int questionId, const unsigned int answerId[10], int answerIdsAmount) {
    unsigned int amountOfAnswers;
    Answer** answers = findAnswersByQuestionId(questionId, &amountOfAnswers);
    if(answers == NULL) {
        return 0;
    }

    char isCorrect = 1;
    int amountOfCorrect = 0;
    for(int i = 0; i < amountOfAnswers; i++) {
        Answer* answer = answers[i];
        if(answer->correct == 1) {
            amountOfCorrect++;

            int j;
            for(j = 0; j < answerIdsAmount; j++) {
                if(answerId[j] == answer->id) {
                    break;
                }
            }
            if(j == answerIdsAmount) {
                isCorrect = 0;
                break;
            }
        }
    }

    if(amountOfCorrect != answerIdsAmount) {
        isCorrect = 0;
    }

    free(answers);

    return isCorrect;
}


//Quiz* updateQuiz(unsigned int id, char* name, Question* questions, unsigned int createdByUserId);
//void deleteQuiz(unsigned int id);

void initQuizRepository() {
    sem_init(&quizQuestionCountSem, 0, 1);
    sem_init(&questionCountSem, 0, 1);
    sem_init(&answerCountSem, 0, 1);
    sem_init(&quizCountSem, 0, 1);
}

void destroyQuizRepository() {

    // free memory of all
    for(int i = 0; i < quizCount; i++) {
        free(quizzesDB[i]->name);
        free(quizzesDB[i]);
    }

    for(int i = 0; i < quizQuestionCount; i++) {
        free(quizQuestionsDB[i]);
    }

    for(int i = 0; i < questionCount; i++) {
        if(questionsDB[i]->mediaType != NONE) free(questionsDB[i]->mediaSrc);
        free(questionsDB[i]->question);
        free(questionsDB[i]);
    }

    for(int i = 0; i < answerCount; i++) {
        free(answersDB[i]->text);
        free(answersDB[i]);
    }

    // destroy semaphores
    sem_destroy(&quizQuestionCountSem);
    sem_destroy(&questionCountSem);
    sem_destroy(&answerCountSem);
    sem_destroy(&quizCountSem);
}