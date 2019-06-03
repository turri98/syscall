#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include "errExit.h"
#include "constants.h"
#include "semaphore.h"
#include "shared_memory.h"

extern char *services[];
extern int numService;

char *getService(unsigned long int myKey) {
    if (myKey>=1 && myKey<MAX_SERVICE_1) {
        return services[0];
    } else if (myKey>=MAX_SERVICE_1 && myKey<=MAX_SERVICE_2) {
        return services[0];
    } else if (myKey>=MAX_SERVICE_2 && myKey<=MAX_SERVICE_3) {
        return services[0];
    } else {
        return "error";
    }
}

//(shm_entry, num, argv[1], argv[2])
int search(struct Entry *shmStart, int *len, char *uid, unsigned long int myKey) {
    int i;
    int myEntry=-1;

    for (i=0; i<len; i++) {
        //entry found
        if (strcmp(shmStart[i].userID, uid) && shmStart[i].key==myKey) {
            return i;
        }

        if (strcmp(shmStart[i].userID, uid) {
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
    struct Entry tmp = *(shmStart + n);
    *(shmStart + n) = *(shmStart + len);
    *(shmStart + len) = tmp;
    (*len)--;
}

int main (int argc, char *argv[]) {
    printf("Hi, I'm ClientExec program!\n");
    if (argc < 3) {
        printf("Usage: %s user_identifier server_key ...\n", *argv);
        return 1;
    }

    //get the id of the semaphore
    int semid = semget(SEMAPHORE_KEY, 1, S_IRUSR | S_IWUSR);
    if (semid == -1)
        errExit("semget failed");

    //get the id of the shared memory
    int shmid = shmget(SEMAPHORE_KEY, sizeof(struct Entry)*MAX_CLIENT, S_IRUSR | S_IWUSR);
    if (shmid == -1)
        errExit("shmget failed");

    //attach the shared memory segment
    struct Entry shm_entry * = (struct Entry shm_entry*) shmat(shmid, NULL, 0);
    if (shm_entry == (struct Entry *) NULL)
        errExit("shmat failed");

    //get the id of the shared memory which contains the number
    int shmNum = shmget(SHARED_NUM_KEY, sizeof(int), S_IRUSR | S_IWUSR);
    if (shmNum == -1)
        errExit("shmget failed");

    //attach the shared memory segment which contains the length
    int *num = (int *) shmat(shmNum, NULL, 0);
    if (num == (int *) NULL)
        errExit("shmat failed");

    //before searching into the server, tries to get the semaphore to access
    semOp(semid, 0, -1);
    int entry=search(shm_entry, num, argv[1], argv[2]);
    if (entry>-1) {
        delEntry(shm_entry, num, entry);
    }
    semOp(semid, 0, 1);

    //switch over entry to know if we have found an entry or we have errors
    switch (entry) {
        case -1: {
            printf("code [%d]: user_identifier and server_key don't appear in the shm_entry.\n", entry);
            printf("\t1. You might have asked for an unsupported service.\n");
            printf("\t2. Your key has already been used.\n");
            printf("\t3. Your key has expired.\n");
            printf("\t4. You are trying to access before requesting a key.\n");
            exit(1);
        }
        case -2: {
            printf("code [%d]: invalid key for user_identifier.\n", entry);
            exit(1);
        }
        case -3: {
            printf("code [%d]: invalid user_identifier for key.\n", entry);
            exit(1);
        }
        case -4: {
            printf("code [%d]: there is no combination (user_identifier, key).\n", entry);
            exit(1);
        }
        default: {
            printf("\nSoon starting service: %s ... \n\n", getService(argv[2]));
            //create the argument vector for the exec call
            //TODO create argVector



        }
    }
}

return 0;
}
