#include "file_system.h"


PageWriter::PageWriter(const char *rootdir) : rootdir(rootdir) {}

PageWriter::WriteState PageWriter::write(const char *page, char *buf, size_t maxlen) {
    string fullpath = FileSystem::getFullPath(rootdir, page);
    const char* path = fullpath.c_str();
    if (!FileSystem::fileExists(path))
        return FILE_NOT_EXIST;
    
    size_t fsize = FileSystem::fileSize(path);
    if (fsize > maxlen) 
        return BUFFER_SHORTAGE;

    FileSystem::readAll(path, buf, fsize);
    return SUCCESS;
}

string FileSystem::getFullPath(const char *rootdir, const char *filename) {
    string res(rootdir);
    if (res.back() != '/')
        res.push_back('/');
    
    res += string(filename);
    debug("fullpath = %s", res.c_str());
    return res;
}

bool FileSystem::fileExists(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) return false;

    if (! S_ISREG(statbuf.st_mode) )
        return false;
    return (access(path, R_OK) != -1);
}

size_t FileSystem::fileSize(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) == -1) 
        return 0;

    return statbuf.st_size;
}

void FileSystem::readAll(const char *path, char *buf, size_t len) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) err_exit("open");

    ssize_t cnt = ::read(fd, buf, len);
    close(fd);

    if (cnt == -1) {
        err_exit("read -1");
    } else if (size_t(cnt) != len)
        err_exit("read cnt != len");
}