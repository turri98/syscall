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
    printf("Hi, I'm Stampa program!\n");

    int i;
    for (i=1;i<argc;i++) {
        printf("%s ", argv[i]);
    }

    printf("\n");

    return 0;
}
