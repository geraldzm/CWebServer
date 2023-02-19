#include <dirent.h>
#include "staticCache.h"
#include "../../../util/logger.h"


typedef struct StaticFile {
    char* name;
    File *file;
} StaticFile;

StaticFile* staticFilesCache;
int staticFilesAmount;
int staticFilesCacheIndex = 0;

void loadStaticFiles() {

    char *path = STATIC_HTML;

    staticFilesAmount = countFilesInDir(path);
    staticFilesCache = malloc(sizeof(StaticFile) * staticFilesAmount);

    DIR *d;
    struct dirent *dir;
    d = opendir(path);

    if(d == NULL) {
        logError("Error opening static files directory");
        return;
    }

    logInfo("Loading %d static files from %s", staticFilesAmount, path);

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_REG) continue;

        char *fileName = dir->d_name;
        char *filePath = malloc(strlen(path) + strlen(fileName) + 1);
        strcpy(filePath, path);
        strcat(filePath, fileName);

        logInfo("Caching static file: %s", dir->d_name, dir->d_type);


        // create static file
        StaticFile *staticFile = &staticFilesCache[staticFilesCacheIndex++];
        staticFile->name = malloc(strlen(fileName) + 1);
        strcpy(staticFile->name, fileName);
        staticFile->file = getTextFile(filePath);

    }

    closedir(d);
}

void flushStaticFiles() {
    logInfo("Flushing static files cache (%d files)", staticFilesAmount);

    // free the name, the content and the struct of staticFilesCache
    for (int i = 0; i < staticFilesAmount; i++) {
        StaticFile *staticFile = &staticFilesCache[i];
        free(staticFile->name);
        free(staticFile->file->content);
    }

    free(staticFilesCache);
}

File* getFile(char* name) {

    for(int i = 0; i < staticFilesCacheIndex; i++) {
        StaticFile staticFile = staticFilesCache[i];
        if(strcmp(staticFile.name, name) == 0) {
            return staticFile.file;
        }
    }

    return NULL;
}
