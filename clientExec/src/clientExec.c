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
#include "constants.h"
#include "semaphore.h"
#include "shared_memory.h"

extern char *path2ServerFIFO;
extern char *baseClientFIFO;
extern char *services[];

int main (int argc, char *argv[]) {
    printf("Hi, I'm ClientExec program!\n");
    return 0;
}
