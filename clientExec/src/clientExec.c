#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "errExit.h"
#include "constants.h"
#include "semaphore.h"
#include "shared_memory.h"

extern char *services[];

void printEntry(char s[], struct Entry k) {
    printf("\n %s: %s, %lu, %ld", s, k.userID, k.key, k.timeStart);
}

char *getService(unsigned long int myKey) {
    if (myKey>=1 && myKey<MAX_SERVICE_1) {
        return services[0];
    } else if (myKey>=MAX_SERVICE_1 && myKey<=MAX_SERVICE_2) {
        return services[1];
    } else if (myKey>=MAX_SERVICE_2 && myKey<=MAX_SERVICE_3) {
        return services[2];
    } else {
        return "error";
    }
}

int search(struct Entry *shmStart, int *len, char *uid, unsigned long int myKey) {
    int i;
    int myEntry=-1;

    for (i=0; i<*len; i++) {


        if (strcmp(shmStart[i].userID, uid)==0 && shmStart[i].key==myKey) {
            return i;
        }

        if (strcmp(shmStart[i].userID, uid)) {
            if (myEntry==-3) {
                myEntry = -4;
            } else {
                myEntry = -2;
            }
        }

        if (shmStart[i].key==myKey) {
            if (myEntry==-2) {
                myEntry=-4;
            } else {
                myEntry=-3;
            }
        }

    }

    return myEntry;
}

void delEntry(struct Entry *shmStart, int *len, int n) {
    struct Entry tmp = shmStart[n];
    shmStart[n] = shmStart[*len-1];
    shmStart[*len-1] = tmp;
    (*len)--;
}

int main (int argc, char *argv[]) {
    printf("Hi, I'm ClientExec program!\n\n");
    if (argc < 3) {
        printf("Usage: %s user_identifier server_key ...\n", *argv);
        return 1;
    }

    //get the id of the semaphore
    int semid = create_sem_set(SEMAPHORE_KEY);

    //get the id of the shared memory
    int shmid = alloc_shared_memory_no_creat(SHARED_MEM_KEY, sizeof(struct Entry)*MAX_CLIENT);

    //attach the shared memory segment
    struct Entry *shm_entry;
    shm_entry = (struct Entry *) get_shared_memory(shmid, 0);

    //get the id of the shared memory which contains the number
    int shmNum = alloc_shared_memory_no_creat(SHARED_NUM_KEY, sizeof(int));

    //attach the shared memory segment which contains the length
    int *num;
    num = (int *) get_shared_memory(shmNum, 0);

    //before searching into the server, tries to get the semaphore to access
    semOp(semid, SEM_SHM, -1);

    printf("\n<clientExec> printing shm before...\n");
    int i;
    for (i=0;i<*num;i++) {
        printf("\n %d) userID: %s, key: %lu, timeStart: %d\n", i, shm_entry[i].userID, shm_entry[i].key, (int)shm_entry[i].timeStart);
    }

    unsigned long int temp_key = strtoul (argv[2], NULL, 10);

    int entry=search(shm_entry, num, argv[1], temp_key);

    if (entry>-1) {
        delEntry(shm_entry, num, entry);
    }

    //leaves the semaphore
    semOp(semid, SEM_SHM, 1);

    //switch over entry to know if we have found an entry or we have errors
    switch (entry) {
        case -1: {
            printf("error code %d >> user_identifier and server_key don't appear in the shm_entry.\n", entry*(-1));
            printf("\t1. You might have asked for an unsupported service.\n");
            printf("\t2. Your key has already been used.\n");
            printf("\t3. Your key has expired.\n");
            printf("\t4. You are trying to access before requesting a key.\n");
            exit(1);
        }
        case -2: {
            printf("error code %d >> invalid key for user_identifier.\n", entry*(-1));
            exit(1);
        }
        case -3: {
            printf("error code %d >> invalid user_identifier for key.\n", entry*(-1));
            exit(1);
        }
        case -4: {
            printf("error code %d >> there is no combination (user_identifier, key).\n", entry*(-1));
            exit(1);
        }
        default: {
            printf("\nSoon starting service: %s ... \n\n", getService(temp_key));

            //create the argument vector for the exec call
            char **argVec;
            int i;
            argVec = (char **) malloc(sizeof(char *)*(argc-1));
            for (i=3;i<argc;i++) {
                argVec[i-2] = argv[i];
            }

            argVec[0] = (char *) malloc(sizeof(char)*100);
            strcpy(argVec[0], "./");
            strcat(argVec[0], getService(temp_key));
            argVec[argc-2]=NULL;

            //printf("\nPrinting argVec\n");
            //for (i=0;i<argc-2;i++) {
            //    printf("%s ", argVec[i]);
            //}
            //fflush(stdout);

            if (strcmp(getService(temp_key), services[0])==0) { //STAMPA

                if (execvp("./stampa", argVec)==-1) {
                    errExit("execvp failed");
                }

            } else if (strcmp(getService(temp_key), services[1])==0) { //SALVA

                if (execvp("./salva", argVec)==-1)
                    errExit("execvp failed");

            } else if (strcmp(getService(temp_key), services[2])==0) { //INVIA

                if (execvp("./invia", argVec)==-1)
                    errExit("execvp failed");

            } else {
                printf("<ClientExec> service not available...");
            }


        }
    }

return 0;
}
