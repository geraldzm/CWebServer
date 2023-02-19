#include "fileManager.h"
#include "../../../util/logger.h"
#include <stdio.h>
#include <dirent.h>

File* __getFile(char* path, char* mode) {

    FILE *file;
    file = fopen(path, mode);

    if(file == NULL) {
        logError("Error opening image");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    File *fileStruct = malloc(sizeof(File));
    fileStruct->content = malloc(fsize + 1);
    fileStruct->fsize = fsize;

    fread(fileStruct->content, fsize, 1, file);
    fileStruct->content[fsize] = '\0';
    fclose(file);

    return fileStruct;
}

int countFilesInDir(char* path) {
    DIR *d;
    struct dirent *dir;
    d = opendir(path);

    if(d == NULL) {
        logError("Error opening static files directory");
        return 0;
    }

    int count = 0;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_REG) continue;
        count++;
    }

    closedir(d);

    return count;
}

File* getTextFile(char* path) {
    return __getFile(path, "r");
}

File* getMediaFile(char* path) {
    return __getFile(path, "rb");
}

void mergePath(char* buffy, char* path, char* fileName) {
    strcpy(buffy, path);
    strcat(buffy, fileName);
}