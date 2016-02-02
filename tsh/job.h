#ifndef OS_HW_JOB_H
#define OS_HW_JOB_H

#define MAXJID    1<<16   /* max job ID */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXLINE    1024   /* max line size */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

struct job_t
{
    /* The job struct */
    pid_t pid;
    /* job PID */
    int jid;
    /* job ID [1, 2, ...] */
    int state;
    /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};

/* The job list */
struct job_t jobs[MAXJOBS];

void initjobs(struct job_t *jobs);

int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);

int deletejob(struct job_t *jobs, pid_t pid);

struct job_t *getjobpid(struct job_t *jobs, pid_t pid);

int pid2jid(pid_t pid);

pid_t fgpid(struct job_t *jobs);

void listjobs(struct job_t *jobs, int output_fd);

void do_bgfg(char **argv, int output_fd);

void waitfg(pid_t pid, int output_fd);

#endif //OS_HW_JOB_H
