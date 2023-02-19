#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZREPOSITORY_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZREPOSITORY_H

#include "../modal/question.h"
#include "../modal/quiz.h"
#include <stdlib.h>

// full CRUD for Question
Question* findAllQuestions(unsigned int *amount);
Question* findQuestionById(unsigned int id);
Question* createQuestion(char* question, unsigned int points, unsigned int time, char* mediaSrc, QuestionMediaType mediaType, QuestionType type);
Question* updateQuestion(unsigned int id, char* question, Answer* answers, unsigned int points, unsigned int time);
void deleteQuestion(unsigned int id);

// full CRUD for Answer
Answer* findAnswerById(unsigned int id);
Answer** findAnswersByQuestionId(unsigned int questionId, unsigned int *amount);
Answer* createAnswer(char* text, char correct, unsigned int questionId);
Answer* updateAnswer(unsigned int id, char* text, char correct);
void deleteAnswer(unsigned int id);

// full CRUD for Quiz
Quiz* findAllQuizzes(unsigned int *amount);
Quiz* findQuizById(unsigned int id);
Quiz* createQuiz(char* name, unsigned int createdByUserId);
void updateQuizStatus(unsigned int id, QuizStatus status);

int addQuestionToQuiz(unsigned int quizId, unsigned int questionId);
unsigned int* findQuestionsByQuizId(unsigned int quizId, unsigned int *amount);
Quiz* updateQuiz(unsigned int id, char* name, Question* questions, unsigned int createdByUserId);
void deleteQuiz(unsigned int id);

// json transformation
char* questionAndAnswerToJson(Question* question,  Answer** answers, unsigned int amountOfAnswers);

//
char isAnswerCorrect(unsigned int questionId, const unsigned int answerId[10], int answerIdsAmount);

// control functions
void initQuizRepository();
void destroyQuizRepository();

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_QUIZREPOSITORY_H
