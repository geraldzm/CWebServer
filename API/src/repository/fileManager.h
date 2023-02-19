#ifndef SO_PROY1_QUIZWEB_GERALD_DANIELB_FILEMANAGER_H
#define SO_PROY1_QUIZWEB_GERALD_DANIELB_FILEMANAGER_H

//#define STATIC_FILES_PATH "static/\0"
//#define STATIC_HTML "static/html/\0"
//#define STATIC_IMAGES "static/images/\0"

#define STATIC_FILES_PATH "/home/geraldzm/develop/tec/so/so-proy1-quizweb-gerald-danielb/API/static/\0"
#define STATIC_HTML "/home/geraldzm/develop/tec/so/so-proy1-quizweb-gerald-danielb/API/static/html/\0"
#define STATIC_IMAGES "/home/geraldzm/develop/tec/so/so-proy1-quizweb-gerald-danielb/API/static/images/\0"

typedef struct File {
    unsigned long fsize;
    char* content;
} File;

File* getTextFile(char* path);
File* getMediaFile(char* path);
void mergePath(char* buffy, char* path, char* fileName);
int countFilesInDir(char* path);


#endif //SO_PROY1_QUIZWEB_GERALD_DANIELB_FILEMANAGER_H
