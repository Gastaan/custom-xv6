#include "kernel/types.h"
#include "user.h"
#include "kernel/top.h"

int
main(int argc, char *argv[])
{
    if (argc != 1)
        exit(-1);

    struct top currentTop;
    currentTop.stop = 0;

    do {
        top(&currentTop);

        if (currentTop.stop)
            break;

        print_size = 0;

        printf("uptime: %d seconds\n", currentTop.uptime / 10);
        printf("total process: %d\n", currentTop.total_process);
        printf("running process: %d\n", currentTop.running_process);
        printf("sleeping process: %d\n", currentTop.sleeping_process);
        printf("Total Memory: %d KB\n", currentTop.total_memory);
        printf("Used Memory: %d KB\n", currentTop.used_memory);
        printf("Free Memory: %d KB\n", currentTop.free_memory);
        printf("name    PID     PPID    state    mem_usage_percentage\n");

        for(int i = 0; i < currentTop.total_process; i++) {
            for (int j = 0; j < 16; j++) {
                if (currentTop.p_list[i].name[j] == '\0')
                    break;
                printf("%c", currentTop.p_list[i].name[j]);
            }
            printf("    %d    %d    ", currentTop.p_list[i].pid, currentTop.p_list[i].ppid);

            switch (currentTop.p_list[i].state) {
                case USED:
                    printf("USED");
                    break;
                case SLEEPING:
                    printf("SLEEPING");
                    break;
                case RUNNABLE:
                    printf("RUNNABLE");
                    break;
                case RUNNING:
                    printf("RUNNING");
                    break;
                case ZOMBIE:
                    printf("ZOMBIE");
                    break;
                default:
                    printf("Unknown state");
                    break;
            }

            printf("    %f%%\n", currentTop.p_list[i].mem_usage_percentage);
            printf("Age of the process: %d seconds\n", currentTop.p_list[i].time / 10);
            printf("CPU usage of the process: %f\n", (1.0 * currentTop.p_list[i].cpu) / currentTop.uptime);
        }

        sleep(10);
        reset_console();

    } while (1);
    return 0;
}
