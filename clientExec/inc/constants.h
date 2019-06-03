#ifndef SYSCALL_CONSTANTS_H
#define SYSCALL_CONSTANTS_H

#define SEMAPHORE_KEY 11
#define SHARED_MEM_KEY 21
#define SHARED_NUM_KEY 22

#define MAX_CLIENT 1000

#define SEM_SHM 0
#define SEM_NUM 1

char *pathFifoServer = "tmp/fifoServer";
char *baseFifoClient = "tmp/fifoClient.";
char *services[] = {
        "stampa", "salva", "invia"
};


#endif //SYSCALL_CONSTANTS_H
