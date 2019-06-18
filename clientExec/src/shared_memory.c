#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include "errExit.h"
#include "shared_memory.h"

 int alloc_shared_memory_no_creat(key_t shmKey, size_t size) {
    // get, or create, a shared memory segment
    int shmid = shmget(shmKey, size, S_IRUSR | S_IWUSR);
    if (shmid == -1)
        errExit("shmget failed");

    return shmid;
}

void *get_shared_memory(int shmid, int shmflg) {
    // attach the shared memory
    void *ptr_sh = shmat(shmid, NULL, shmflg);
    if (ptr_sh == (void *)-1)
        errExit("shmat failed");

    return ptr_sh;
}