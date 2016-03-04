#include <stdio.h>
#include <fcntl.h>
#include "procinfo.h"

int getprocinfo(pid_t pid, procinfo_t *info)
{
    int dev;
    procinfo_arg_t arg;
    if ((dev = open("/dev/procinfo", O_RDONLY)) == -1) {
        perror("open /dev/procinfo");
        return 1;
    }
    arg.pid = pid;
    arg.info = info;
    if (ioctl(dev, GET_PROCINFO, &arg) == -1) {
        perror("ioctl /dev/procinfo");
        return 1;
    }
    printf("pid: %d\nppid: %d\nstart_time (monotonic): %ld.%ld\nnum_sib: %d\n",
           info->pid,
           info->ppid,
           info->start_time.tv_sec,
           info->start_time.tv_nsec,
           info->num_sib);
    close(dev);
    return 0;
}

int main(int argc, char *argv[])
{
    pid_t pid;
    procinfo_t info;
    if (argc == 1) {
        pid = 0;
    } else {
        pid = atoi(argv[1]);
    }
    getprocinfo(pid, &info);
    return 1;
}