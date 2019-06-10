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
#include <sys/sem.h>
#include <sys/types.h>

#include "errExit.h"
#include "request.h"
#include "response.h"
#include "semaphore.h"
#include "shared_memory.h"
#include "constants.h"

extern char *pathFifoServer;
extern char *baseFifoClient;
extern char *services[];

int serverFIFO, serverFIFO_extra;

unsigned long int keyStampa = 1;
unsigned long int keySalva = MAX_SERVICE_1 + 1;
unsigned long int keyInvia = MAX_SERVICE_2 + 1;

int shmid, shmNum, *num; // *num contatore dei client = 0 /shmid  file system shared mem /shmNum numero di segmenti attaccati
struct Entry *shm_entry;
int semid;

pid_t keyManager;
pid_t server;

void printSHM() {
    int i;
    for (i=0;i<*num;i++) {
        printf("\n %d) userID: %s, key: %lu, timeStart: %d\n", i, shm_entry[i].userID, shm_entry[i].key, (int)shm_entry[i].timeStart);
    }

}

void quit(int sig) {
    if (getpid()==server) {
        //send SIGTERM to keyManager
        printf("<Server> killing keyManager...\n");
        if (kill(keyManager, SIGTERM) == -1)
            errExit("kill failed");

        // Close the FIFO
        printf("<Server> closing the fifo...\n");
        if (serverFIFO != 0 && close(serverFIFO) == -1)
            errExit("close failed");

        printf("<Server> closing the extra fifo...\n");
        if (serverFIFO_extra != 0 && close(serverFIFO_extra) == -1)
            errExit("close failed");

        // Remove the FIFO
        printf("<Server> unlinking the fifo...\n");
        if (unlink(pathFifoServer) != 0)
            errExit("unlink failed");

        printf("<Server> detaching shm...\n");
        free_shared_memory(shm_entry);
        printf("<Server> detaching shmNum...\n");
        free_shared_memory(num);
        printf("<Server> removing shm...\n");
        remove_shared_memory(shmid);
        printf("<Server> removing shmNum...\n");
        remove_shared_memory(shmNum);

        printf("<Server> removing sem...\n");
        if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1)
            errExit("semctl IPC_RMID failed");

        fflush(stdout);
        while(wait(NULL) == -1);
        printf("<Server> turning off...\n");
        // terminate the process
        exit(0);
    } else {
        printf("<KeyManager> turning off...\n");
        exit(0);
    }
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

void addEntry(struct Entry myEntry) {
    strcpy(shm_entry[*num].userID, myEntry.userID);
    shm_entry[*num].key = myEntry.key;
    shm_entry[*num].timeStart = myEntry.timeStart;
}
void swap(int x, int y) {
    struct Entry tmp = *(shm_entry + x);
    *(shm_entry + x) = *(shm_entry + y);
    *(shm_entry + y) = tmp;
}

void delEntry() {
    time_t now = time(NULL);
    printf("\n<KeyManager> starting to delete old entries...\n\n");
    fflush(stdout);

    for(int i=0; i<*num; i++) {
        if(now - shm_entry[i].timeStart >= TTL) {
            //swapping current entry with the last one

            swap(i, *num-1);
            //last entry is not considered anymore
            (*num)--;
            //restart from current position in order to check if the swapped entry is valid
            i--;
        }
    }
}

int create_sem_set(key_t semkey) {
    // Create a semaphore set with 2 semaphores
    int semid = semget(semkey, 2, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1)
        errExit("semget failed");

    // Initialize the semaphore set
    unsigned short values[] = {1, 1};
    union semun arg;
    arg.array = values;

    if (semctl(semid, 0, SETALL, arg) == -1)
        errExit("semctl SETALL failed");

    return semid;
}

unsigned long int getKey(struct Request *request) {

    unsigned long int myKey = 0;

    char myService[10];
    strcpy(myService, request->service);
    strlwr(myService);

    if (strcmp(services[0], myService) == 0) { //STAMPA
        if (keyStampa == MAX_SERVICE_1)
            keyStampa = 1;
        return keyStampa++;
    } else if (strcmp(services[1], myService) == 0) { //SALVA
        if (keySalva == MAX_SERVICE_2)
            keySalva = MAX_SERVICE_1 + 1;
        return keySalva++;
    } else if (strcmp(services[2], myService) == 0) { //INVIA
        if (keyInvia == MAX_SERVICE_3)
            keyInvia = MAX_SERVICE_2 + 1;
        return keyInvia++;
    } else { // service error
        return myKey; //in clientReq if the return value is 0, the service request wasn't successful
    }
}

void sendResponse(struct Request *request) {

    // make the path of client's FIFO
    char pathFifoClient[100];
    sprintf(pathFifoClient, "%s%d", baseFifoClient, request->pid);

    printf("<Server> opening FIFO %s...\n", pathFifoClient);
    // Open the client's FIFO in write-only mode
    int clientFIFO = open(pathFifoClient, O_WRONLY);
    if (clientFIFO == -1) {
        printf("<Server> open failed\n");
        return;
    }

    // Prepare the response for the client
    struct Response response;
    struct Entry myEntry;
    response.key = getKey(request); //0 if the request is malformed

    // write to sharedMemory if response.key!=0 (request->userID, response.key, time_t current)
    if (response.key!=0) {

        semOp(semid, SEM_SHM, -1);

        if ((*num)+1>=MAX_CLIENT) {
            //shared memory full

            printf("\nThe shared memory is full, please wait...");
            fflush(stdout);

        } else {

            strcpy(myEntry.userID, request->userID);
            myEntry.key = response.key;
            myEntry.timeStart = time(NULL);

            addEntry(myEntry);

            (*num)++;

        }


        //DEBUG
        //printf("## %s, %lu, %ld\n",myEntry.userID, myEntry.key, myEntry.timeStart);

        //printf("<Server> printing current state of shared memory... \n\n");
        //printSHM();

        semOp(semid, SEM_SHM, 1);

    }

    printf("<KeyManager> sending a response\n");
    // Write the Response into the opened FIFO
    if (write(clientFIFO, &response, sizeof(struct Response))
        != sizeof(struct Response)) {
        printf("<KeyManager> write failed");
    }

    // Close the FIFO
    if (close(clientFIFO) != 0)
        printf("<KeyManager> close failed\n");
}

int main(int argc, char *argv[]) {
    printf("Hi, I'm Server program!\n");
    server=getpid();

    // set of signals
    sigset_t mySetServer;
    // initialize mySet to contain all signals
    sigfillset(&mySetServer);
    // remove SIGTERM from mySet
    sigdelset(&mySetServer, SIGTERM);
    //sigdelset(&mySetServer, SIGINT);
    // blocking all signals but SIGTERM
    sigprocmask(SIG_SETMASK, &mySetServer, NULL);

    // set the function sigHandler as handler for the signal SIGTERM
    if (signal(SIGTERM, quit) == SIG_ERR)
        errExit("change signal handler failed");

    // set the function sigHandler as handler for the signal SIGINT
    //if (signal(SIGINT, quit) == SIG_ERR)
    //    errExit("change signal handler failed");

    //printf("keyStampa: %lu \n", keyStampa);
    //printf("keySalva: %lu \n", keySalva);
    //printf("keyInvia: %lu \n", keyInvia);

    //for (int k=1;k<MAX_SERVICE; k++) {
    //    printf("keySalva: %lu \n", keySalva+=1400000);
    //}


    // allocate a shared memory segment for the entries
    printf("<Server> allocating a shared memory segment...\n");
    //shmid = alloc_shared_memory(SHARED_MEM_KEY, sizeof(struct Entry)*MAX_CLIENT);
    shmid = shmget(SHARED_MEM_KEY, sizeof(struct Entry)*MAX_CLIENT, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (shmid == -1)
        errExit("server shmget1 failed");

    // attach the shared memory segment
    printf("<Server> attaching the shared memory segment...\n");
    shm_entry = (struct Entry*)get_shared_memory(shmid, 0);

    // allocate a shared memory segment for the number of entries
    printf("<Server> allocating a shared memory segment...\n");
    //shmNum = alloc_shared_memory(SHARED_NUM_KEY, sizeof(int)), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;
    shmNum = shmget(SHARED_NUM_KEY, sizeof(struct Entry)*MAX_CLIENT, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (shmid == -1)
        errExit("server shmget2 failed");


    // attach the shared memory segment
    printf("<Server> attaching the shared memory segment...\n");
    num = (int *)get_shared_memory(shmNum, 0);

    *num=0;

    // create a semaphore set
    printf("<Server> creating a semaphore set...\n");
    semid = create_sem_set(SEMAPHORE_KEY);


    keyManager = fork();

    if (keyManager == 0) {
        //process keyManager

        printf("<Server> starting keyManager with PID %d ...\n", getpid());

        while(1) {

            sleep(TIME_TO_SLEEP);
            semOp(semid, SEM_SHM, -1);

            //printSHM();
            delEntry();
            printf("##################################\n<keyManager> deleting entries at time %d...", (int)time(NULL));
            fflush(stdout);
            //printf("\n\n<keyManager> printing shared memory\n");
            printSHM();
            printf("\n##################################\n\n");
            fflush(stdout);

            semOp(semid, SEM_SHM, 1);

        }

    }

    //process server
    printf("Server PID: %u, keyManager PID: %u\n", getpid(), keyManager);

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
