#ifndef _REQUEST_HH
#define _REQUEST_HH

struct Request {
    char userID[100];
    char service[10];
    pid_t pid;
};

#endif
