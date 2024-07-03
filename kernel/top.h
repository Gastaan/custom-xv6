#include "param.h"

struct proc_info{
    char name[16];
    uint cpu;
    uint time;
    int pid;
    int ppid;
    enum procstate state;
    float mem_usage_percentage; // Add this field
};

struct top{
    long uptime;
    int total_process;
    int running_process;
    int sleeping_process;
    int stop;
    int total_memory; // Add this field
    int used_memory;  // Add this field
    int free_memory;  // Add this field
    struct proc_info p_list[NPROC];
};
