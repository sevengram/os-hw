#ifndef _PROCINFO_H_
#define _PROCINFO_H_

#include <linux/ioctl.h>
#include <linux/types.h>

typedef struct procinfo {
    pid_t pid;      /* Process ID */
    pid_t ppid;     /* Parent process ID */
    struct timespec start_time; /* Process start time */
    int num_sib;    /* Number of siblings */
} procinfo_t;

typedef struct procinfo_arg {
    pid_t pid;
    procinfo_t *info;
} procinfo_arg_t;

#define PROCINFO_TYPE_MAGIC 78

#define GET_PROCINFO _IOR(PROCINFO_TYPE_MAGIC, 1, procinfo_arg_t *)

#endif
