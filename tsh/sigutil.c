#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "sigutil.h"
#include "errmsg.h"
#include "job.h"

typedef void handler_t(int);

sigset_t mask_signal(int how, int sig)
{
    sigset_t signals, result;
    if (sigemptyset(&signals) < 0)
        unix_error("mask_signal: sigemptyset failed");
    if (sigaddset(&signals, sig) < 0)
        unix_error("mask_signal: sigaddset failed");
    if (sigprocmask(how, &signals, &result) < 0)
        unix_error("mask_signal: sigprocmask failed");
    return result;
}

int send_signal(pid_t pid, int sig)
{
    if (kill(pid, sig) < 0) {
        if (errno != ESRCH)
            unix_error("send_signal: kill failed");
    }
    return 0;
}

/*
 * bind_signal - wrapper for the sigaction function
 */
handler_t *bind_signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("bind_signal error");
    return old_action.sa_handler;
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    printf("siquit_handler: terminating after receipt of SIGQUIT signal\n");
    exit(1);
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)
{
    pid_t pid = fgpid(jobs);
    int jid = pid2jid(pid);

    if (pid != 0) {
        printf("Job [%d] (%d) stopped by signal %d\n", jid, pid, sig);
        getjobpid(jobs, pid)->state = ST;
        send_signal(-pid, sig);
    }
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig)
{
    pid_t pid = fgpid(jobs);
    int jid = pid2jid(pid);

    if (pid != 0) {
        printf("Jobs [%d] (%d) terminated by signal %d\n", jid, pid, sig);
        deletejob(jobs, pid);
        send_signal(-pid, sig);
    }
}

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */
void sigchld_handler(int sig)
{
    pid_t pid;
    int status, child_sig;

    /* reap any zombies and handle status reports */
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {

        if (WIFSTOPPED(status)) {
            sigtstp_handler(WSTOPSIG(status));
        }
        else if (WIFSIGNALED(status)) {
            child_sig = WTERMSIG(status);
            if (child_sig == SIGINT)
                sigint_handler(child_sig);
            else
                unix_error("sigchld_handler: uncaught signal\n");
        }
        else
            deletejob(jobs, pid); /* remove the job */
    }

    if (pid == -1 && errno != ECHILD)
        unix_error("sigchld_handler: waitpid error");

    return;
}


