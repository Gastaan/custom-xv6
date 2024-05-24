#include "kernel/types.h"
#include "user.h"
#include "kernel/top.h"

int
main(int argc, char *argv[])
{
    int father_pid = getpid();

    for (int i = 0; i < 10; i++)
        if (getpid() == father_pid)
            fork();


    for (int i = 0; i < 1000 * 1000 * 1000; i++)
    {

    }
//    for (int i = 0; i < 1000 * 1000 * 1000; i++)
//    {
//
//    }
//    for (int i = 0; i < 1000 * 1000 * 1000; i++)
//    {
//
//    }
//    for (int i = 0; i < 1000 * 1000 * 1000; i++)
//    {
//
//    }
//    for (int i = 0; i < 1000 * 1000 * 1000; i++)
//    {
//
//    }

    printf("The process with id of %d is finished!\n", getpid());
    return 0;
}