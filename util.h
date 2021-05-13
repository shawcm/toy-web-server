#ifndef UTIL_H
#define UTIL_H

#include <bits/stdc++.h>
#include <time.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


#define err_exit(msg) do {\
    fprintf(stderr, "(%s : %d): ", __FILE__, __LINE__);\
    fflush(stderr);\
    perror(msg);\
    exit(-1);\
} while (0)



#define do_debug
#undef do_debug

#ifdef do_debug
#define debug(...) do {\
    fprintf(stderr, "DEBUG(%s:%d): ", __FILE__, __LINE__);\
    fprintf(stderr, __VA_ARGS__);\
    fprintf(stderr, "\n");\
    fflush(stderr);\
} while(0)
#else
#define debug(...) do {} while(0)
//(void *)0
#endif


// use main() to debug
//#define accept_main_test

using std::string;
#endif