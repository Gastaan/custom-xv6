#include "param.h"

struct proc_info{
    char name[16];
    uint cpu;
    uint time;
    int pid;
    int ppid;
    enum procstate state;
};

struct top{
    long uptime;
    int total_process;
    int running_process;
    int sleeping_process;
    struct proc_info p_list[NPROC];
};

int top(struct top * t);