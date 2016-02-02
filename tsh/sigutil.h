#ifndef OS_HW_SIGUTIL_H
#define OS_HW_SIGUTIL_H

typedef void handler_t(int);

sigset_t mask_signal(int how, int sig);

int send_signal(pid_t pid, int sig);

handler_t *bind_signal(int signum, handler_t *handler);

void sigquit_handler(int sig);

void sigtstp_handler(int sig);

void sigint_handler(int sig);

void sigchld_handler(int sig);

#endif //OS_HW_SIGUTIL_H
