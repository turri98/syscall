#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "request.h"
#include "response.h"
#include "errExit.h"
#include "constants.h"

extern char *path2serverFIFO;
extern char *baseClientFIFO;
extern char *services[];

int main (int argc, char *argv[]) {

    struct Request request;

    printf("Hi, I'm ClientReq program!\n");
    printf("Ecco i servizi di sistema:\n -stampa\n -salva\n -invia\n");

    printf(" Per favore inserisci il tuo nome:\n");
    scanf("%s", request.userID);
    printf("Per favore inserisci il nome del servizio che vuoi utilizzare:\n");
    scanf("%s",request.service);

    // create the client fifo path using the base baseFifoClient path and adding his pid
    char path2clientFIFO[100];
    pid_t myPid = getpid();
    request.pid = myPid;
    sprintf(path2clientFIFO, "%s%d", baseFifoClient, myPid);

    printf("<Client> making FIFO...\n");

    // create the fifo in writing and reading mode
    if (mkfifo(path2clientFIFO, S_IRUSR | S_IWUSR) == -1)
        errExit("mkfifo failed");

    // opening the fifo for writing the response
    int sd = open(path2serverFIFO, O_WRONLY);
    if (sd == -1)
        errExit("open failed");

    // sending the request on the fifo
    if (write(sd, &request,sizeof(struct Request))
        != sizeof(struct Request))
        errExit("write failed");

    // opening the fifo for reading only
    int cd = open(path2clientFIFO, O_RDONLY);
    if (cd == -1)
        errExit("open failed");

    // reading the response from the fifo
    struct Response response;

    int bR = read(cd, &response, sizeof(struct Response));
    if(bR == -1)
        errExit("read failed");
    else if(bR != sizeof(struct Response))
        printf("Bad response\n");
    else {
        // printing the result on the terminal
        if (response.key!=0) {
            printf("\n");
            printf("Key:\t%lu\n", response.key);
            printf("UserId:\t%s\n", request.userID);
        } else {
            printf("<Client> this service does not exist or the shared memory is full\n");
        }
    }

    // closing the fifo
    if (close(cd) == -1)
        errExit("close failed");

    // removing the fifo
    if (unlink(path2clientFIFO) != 0)
        errExit("unlink failed");
    return 0;
}