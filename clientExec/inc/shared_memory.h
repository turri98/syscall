#ifndef _SHARED_MEMORY_HH
#define _SHARED_MEMORY_HH

// the Request structure defines a request sent by a client
struct Entry {
    char userID[100];
    unsigned long int key;
    time_t timeStart;
};

// The alloc_shared_memory method creates, if it does not exist, a shared
// memory segment with size bytes and shmKey key.
// It returns the shmid on success, otherwise it terminates the calling process
int alloc_shared_memory_no_creat(key_t shmKey, size_t size);

// The get_shared_memory attaches a shared memory segment in the logic address space
// of the calling process.
// It returns a pointer to the attached shared memory segment,
// otherwise it terminates the calling process
void *get_shared_memory(int shmid, int shmflg);

#endif
