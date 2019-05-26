#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include "errExit.h"

int main (int argc, char *argv[]) {
    printf("Hi, I'm Salva program!\n");

    if (argc==1) {
        errExit("wrong number of arguments");
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    if (fd==-1) {
        errExit("file doesn't exist");
    }

    int i;

    for (i=2;i<argc;i++) {
        // check if write completed successfully
        if (write(fd, argv[i], strlen(argv[i])) != strlen(argv[i]) )
            errExit("write failed");
        if (write(fd, " ", strlen(" ")) != strlen(" ") )
            errExit("write failed");
    }
    if (write(fd, "\n", strlen("\n")) != strlen("\n") )
        errExit("write failed");

    // close file descriptor
    if (close(fd)== -1)
        errExit("close failed");

    return 0;
}
