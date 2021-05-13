#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "util.h"

class FileSystem {
public:
    static string getFullPath(const char *rootdir, const char *filename);
    static bool fileExists(const char *path);
    static size_t fileSize(const char *path);
    static void readAll(const char *path, char *buf, size_t len);
};


class PageWriter {
public:
    enum WriteState {BUFFER_SHORTAGE, FILE_NOT_EXIST, SUCCESS};
    PageWriter(const char *rootdir);
    WriteState write(const char *page, char *buf, size_t maxlen);
private:
    const char *rootdir;
};

#endif