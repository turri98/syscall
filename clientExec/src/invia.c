#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "message.h"
#include "errExit.h"

#define STR_SIZE(X) sizeof(char)*strlen(X)
int main (int argc, char *argv[]) {
    printf("Hi, I'm Invia program!\n");

    // check command line input arguments
    if(argc < 2) {
        printf("Usage: %s filename ...\n", *argv);
        return 1;
    }
    // read the message queue key defined by user
    int msgKey = atoi(argv[1]);

    if(msgKey<=0){
        printf("\npls put a msg grater than zero");
        return 1;
    }
    // get the message queue identifier
    int msgid =msgget(msgKey, IPC_CREAT| S_IWUSR);
    if (msgid == -1)
        errExit("msgget failed");

    srand(time(NULL));
    struct message_t msg;
    msg.mtype=rand()%100 +1;
    strcpy(msg.message, "");
    for(int i=0; i<argc-2; i++) {
        strncat(msg.message, *(argv + i + 2), STR_SIZE(*(argv + i + 2)));
        strcat(msg.message, "\n");
    }
    strcat(msg.message, "\0");

    //Print to terminal the message
    //printf("mtype: %ld\n", msg.mtype);
    //printf("message: %s\n", msg.message);

    //calculate msg size and them sends it to the msq
    size_t msgSize = sizeof(struct message_t) - sizeof(long);
    if(msgsnd(msgid, &msg, msgSize, IPC_NOWAIT) == -1) {
        if(errno == EAGAIN) {
            printf("message_queue full\n");
            return 1;
        }
        else
            errExit("msgsnd failed");
    }

    printf("Message successfully sent to the message_queue %s\n", argv[1]);

    return 0;
}