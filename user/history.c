#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 2)
        exit(1);

    history(atoi(argv[1]));

    return 0;
}