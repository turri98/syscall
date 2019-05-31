#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/shm.h>

#include "errExit.h"
#include "request.h"
#include "response.h"
#include "semaphore.h"
#include "shared_memory.h"

#define MAX_SERVICE_1 1400000000
#define MAX_SERVICE_2 2800000000
#define MAX_SERVICE_3 4200000000

char *pathFifoServer = "tmp/fifoServer";
char *baseFifoClient = "tmp/fifoClient.";

int serverFIFO, serverFIFO_extra;

char *services[] = {
        "stampa", "salva", "invia"
};

unsigned long int keyStampa = 1;
unsigned long int keySalva = MAX_SERVICE_1 + 1;
unsigned long int keyInvia = MAX_SERVICE_2 + 1;

void quit(int sig) {

    // Close the FIFO
    if (serverFIFO != 0 && close(serverFIFO) == -1)
        errExit("close failed");

    if (serverFIFO_extra != 0 && close(serverFIFO_extra) == -1)
        errExit("close failed");

    // Remove the FIFO
    if (unlink(pathFifoServer) != 0)
        errExit("unlink failed");

    // terminate the process
    _exit(0);
}

void strlwr(char s[]) {
    int c = 0;

    while (s[c] != '\0') {
        if (s[c] >= 'A' && s[c] <= 'Z') {
            s[c] = s[c] + 32;
        }
        c++;
    }
}

unsigned long int getKey(struct Request *request) {

    unsigned long int myKey=0;

    char myService[10];
    strcpy(myService, request->service);
    strlwr(myService);

    if (strcmp(services[0], myService)==0) { //STAMPA
        if (keyStampa==MAX_SERVICE_1)
            keyStampa=1;
        return keyStampa++;
    } else if (strcmp(services[1], myService)==0) { //SALVA
        if (keySalva==MAX_SERVICE_2)
            keySalva=MAX_SERVICE_1+1;
        return keySalva++;
    } else if (strcmp(services[2], myService)==0) { //INVIA
        if (keyInvia==MAX_SERVICE_3)
            keyInvia=MAX_SERVICE_2+1;
        return keyInvia++;
    } else { // service error
        return myKey; //TODO in clientReq if the return value is 0, the service request wasn't successful
    }
}

void sendResponse(struct Request *request) {

    // make the path of client's FIFO
    char pathFifoClient [100];
    sprintf(pathFifoClient, "%s%d", baseFifoClient, request->pid);

    printf("<Server> opening FIFO %s...\n", pathFifoClient);
    // Open the client's FIFO in write-only mode
    int clientFIFO = open(pathFifoClient, O_WRONLY);
    if (clientFIFO == -1) {
        printf("<Server> open failed");
        return;
    }

    // Prepare the response for the client
    struct Response response;
    response.key = getKey(request);
    // TODO implement getKey()

    // TODO write to sharedMemory (request->userID, response.key, time_t current)

    printf("<Server> sending a response\n");
    // Write the Response into the opened FIFO
    if (write(clientFIFO, &response, sizeof(struct Response))
            != sizeof(struct Response)) {
        //printf("<Server> write failed");
        errExit("write failed");
    }

    // Close the FIFO
    if (close(clientFIFO) != 0)
        printf("<Server> close failed");
}

int main (int argc, char *argv[]) {
    printf("Hi, I'm Server program!\n");

    printf("keyStampa: %lu \n", keyStampa);
    printf("keySalva: %lu \n", keySalva);
    printf("keyInvia: %lu \n", keyInvia);

    //for (int k=1;k<MAX_SERVICE; k++) {
    //    printf("keySalva: %lu \n", keySalva+=1400000);
    //}

    // TODO fork for keyManager

    // set of signals
    sigset_t mySet;
    // initialize mySet to contain all signals
    sigfillset(&mySet);
    // remove SIGTERM from mySet
    sigdelset(&mySet, SIGTERM);
    // blocking all signals but SIGINT
    //sigprocmask(SIG_SETMASK, &mySet, NULL); //TODO remove the // at the start of the line for blocking all signals

    // set the function sigHandler as handler for the signal SIGINT
    if (signal(SIGTERM, quit) == SIG_ERR)
        errExit("change signal handler failed");

    // set the function sigHandler as handler for the signal SIGINT
    if (signal(SIGINT, quit) == SIG_ERR)
        errExit("change signal handler failed"); //TODO remove this part before submitting

    printf("<Server> Making FIFO...\n");
    // make a FIFO with the following permissions:
    // user:  read, write
    // group: write
    // other: no permission
    if (mkfifo(pathFifoServer, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
        errExit("mkfifo failed");

    printf("<Server> FIFO %s created!\n", pathFifoServer);

    // Wait for client in read-only mode. The open blocks the calling process
    // until another process opens the same FIFO in write-only mode
    printf("<Server> waiting for a client...\n");
    serverFIFO = open(pathFifoServer, O_RDONLY);
    if (serverFIFO == -1)
        errExit("open read-only failed");

    // Open an extra descriptor, so that the server does not see end-of-file
    // even if all clients closed the write end of the FIFO
    serverFIFO_extra = open(pathFifoServer, O_WRONLY);
    if (serverFIFO_extra == -1)
        errExit("open write-only failed");


    struct Request request;
    int bR = -1;
    do {
        printf("<Server> waiting for a Request...\n");
        // Read a request from the FIFO
        bR = read(serverFIFO, &request, sizeof(struct Request));

        // Check the number of bytes read from the FIFO
        if (bR == -1)
            printf("<Server> it looks like the FIFO is broken\n");
        else if (bR != sizeof(struct Request) || bR == 0)
            printf("<Server> it looks like I did not receive a valid request\n");
        else
            sendResponse(&request);

    } while (bR != -1);

    quit(0);
}
