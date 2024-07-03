#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096

int
main(int argc, char *argv[])
{
    // Allocate more than half of the available memory using sbrk
    int size = (2 * PGSIZE) / 3;
    char *mem = sbrk(size);

    if (mem == (char*)-1) {
        printf("sbrk failed\n");
        exit(-1);
    }

    // Write to the allocated memory
    for (int i = 0; i < size; i++) {
        mem[i] = i % 256;
    }

    // Fork the process
    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(-1);
    }

    if (pid == 0) {
        // Child: Verify that the memory was copied correctly
        for (int i = 0; i < size; i++) {
            if (mem[i] != i % 256) {
                printf("Memory mismatch at index %d\n", i);
                exit(-1);
            }
        }
        // Modify the memory in the child process
        for (int i = 0; i < size; i++) {
            mem[i] = (i + 1) % 256;
        }
        printf("Child modified memory\n");
        exit(0);
    } else {
        // Parent: Wait for the child to finish
        wait(0);
        // Verify that the parent's memory is unchanged
        for (int i = 0; i < size; i++) {
            if (mem[i] != i % 256) {
                printf("Memory mismatch in parent at index %d\n", i);
                exit(-1);
            }
        }
        printf("Parent memory unchanged\n");
        exit(0);
    }
}
