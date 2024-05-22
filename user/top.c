#include "kernel/types.h"
#include "user.h"
#include "kernel/top.h"

int
main(int argc, char *argv[])
{
    if (argc != 1)
        exit(-1);

    struct top currentTop;
    top(&currentTop);

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

        switch(currentTop.p_list[i].state) {
            case USED:
                printf("USED\n");
                break;
            case SLEEPING:
                printf("SLEEPING\n");
                break;
            case RUNNABLE:
                printf("RUNNABLE\n");
                break;
            case RUNNING:
                printf("RUNNING\n");
                break;
            case ZOMBIE:
                printf("ZOMBIE\n");
                break;
            default:
                printf("Unknown state\n");
                break;
        }

        printf("Age of the process: %d seconds\n", currentTop.p_list[i].time / 10);
        printf("CPU usage of the process: %f\n", (1.0 * currentTop.p_list[i].cpu) / currentTop.uptime);

    }

    return 0;
}