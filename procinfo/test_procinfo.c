#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "procinfo.h"

void getprocinfo(pid_t pid, procinfo_t *info)
{
    int dev;
    procinfo_arg_t arg;
    dev = open("/dev/procinfo", O_RDONLY);
    arg.pid = pid;
    arg.info = info;
    if (ioctl(dev, GET_PROCINFO, &arg) != -1) {
        printf("%d\n", info->pid);
        printf("%d\n", info->ppid);
        //printf("%ld\n",info->start_time.tv_sec);
    }
    close(dev);
}

int main()
{
    procinfo_t info;
    getprocinfo(-1, &info);
    return 1;
}