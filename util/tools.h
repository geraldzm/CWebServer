#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_TOOLS_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_TOOLS_H

// returns 1 if success, 0 if error
int parseStrToUnsInt(char* str,  unsigned int* result);
char* sha1AndBase64(char* msg);
char *url_decode(char *str);

char* trim(char *str);

#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_TOOLS_H
