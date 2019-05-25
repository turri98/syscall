#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "errExit.h"
#include "request.h"
#include "response.h"

char *pathFifoServer = "/tmp/fifoServer";
char *baseFifoClient = "/tmp/fifoClient.";

int serverFIFO, serverFIFO_extra;

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

void sendResponse(struct Request *request) {

    // make the path of client's FIFO
    char pathFifoClient [100];
    sprintf(pathFifoClient, "%s%d", baseFifoClient, request.pid);

    printf("<Server> opening FIFO %s...\n", pathFifoClient);
    // Open the client's FIFO in write-only mode
    int clientFIFO = open(pathFifoClient, O_WRONLY);
    if (clientFIFO == -1) {
        printf("<Server> open failed");
        return;
    }

    // Prepare the response for the client
    struct Response response;
    response.key = getKey(request->userID, request->service); //getKey is a keyManger method
    // TODO implement getKey()

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

    // set of signals
    sigset_t mySet;
    // initialize mySet to contain all signals
    sigfillset(&mySet);
    // remove SIGTERM from mySet
    sigdelset(&mySet, SIGTERM);
    // blocking all signals but SIGINT
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    // set the function sigHandler as handler for the signal SIGINT
    if (signal(SIGTERM, quit) == SIG_ERR)
        errExit("change signal handler failed");

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
