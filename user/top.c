#include "kernel/types.h"
#include "kernel/top.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 1)
        exit(-1);

    struct top currentTop;
    if (fillTop(&currentTop) == -1)
        return -1;

    printf("uptime:%d seconds\n", currentTop.uptime / 10);
    printf("total process:%d\n", currentTop.total_process);
    printf("running process:%d\n", currentTop.running_process);
    printf("sleeping process:%d\n", currentTop.sleeping_process);
    printf("name    PID     PPID    state\n");

    for(int i = 0; i < currentTop.total_process; i++) {
        for(int j = 0; j < 16; j++) {
            if (currentTop.p_list[i].name[j] == '\0')
                break;
            printf("%c", currentTop.p_list[i].name[j] );
        }
        printf("    %d    %d    ", currentTop.p_list[i].pid, currentTop.p_list[i].ppid);
    }

    return 0;
}