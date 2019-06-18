#ifndef SYSCALL_CONSTANTS_H
#define SYSCALL_CONSTANTS_H

#define SEMAPHORE_KEY 11
#define SHARED_MEM_KEY 21
#define SHARED_NUM_KEY 22

#define MAX_CLIENT 1000

#define SEM_SHM 0

#define MAX_SERVICE_1 1400000000
#define MAX_SERVICE_2 2800000000
#define MAX_SERVICE_3 4200000000

char *services[] = {
        "stampa", "salva", "invia"
};


#endif //SYSCALL_CONSTANTS_H
