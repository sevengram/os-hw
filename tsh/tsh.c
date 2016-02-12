/*
 * tsh - A tiny shell program
 *
 * jianxiang.fan@colorado.edu - Jianxiang Fan
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include "errmsg.h"
#include "job.h"
#include "sigutil.h"
#include "stack.h"
#include "util.h"

/* Misc manifest constants */
#define MAXLINE         1024  /* max line size */
#define MAXARGS         128   /* max args on a command line */
#define MAXPIPE         128
#define HISTORY_LIMIT   256

/* command line prompt */
static int current = 0;

static char cmd_history[HISTORY_LIMIT][MAXLINE];

static char cwd[MAXLINE];

static char buf[MAXLINE];

void eval(const char *cmdline);

int parse_line(const char *cmdline, int *p_argc, char **argv);

int parse_pipe(int argc, char **argv, int *cmd_postions);

void parse_redirect(char **argv, int *input_fd, int *output_fd);

void history_exec(int start, int n);

void save_history(const char *cmd);

void change_dir(const char *path);

int subs_exec(int argc, char **argv);

void usage(void);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    FILE *fp;
    int bash_mode = 0; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = (char) getopt(argc, argv, "hp")) != EOF) {
        switch (c) {
            case 'h':             /* print help message */
                usage();
                break;
            case 'p':             /* don't print a prompt */
                bash_mode = 0;  /* handy for automatic testing */
                break;
            default:
                usage();
        }
    }

    /* Install the signal handlers */
    bind_signal(SIGINT, sigint_handler);   /* ctrl-c */
    bind_signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    bind_signal(SIGCHLD, sigchld_handler);  /* terminated or stopped child */
    bind_signal(SIGQUIT, sigquit_handler);  /* so parent can cleanly terminate child*/

    /* Initialize the job list */
    initjobs(jobs);

    if (argc > 1 && argv[1][0] != '-') {
        fp = fopen(argv[1], "r");
        bash_mode = 1;
    } else {
        fp = stdin;
    }

    /* Execute the shell's read/eval loop */
    while (1) {
        /* Read command line */
        if (!bash_mode) {
            getcwd(cwd, MAXLINE);
            printf("%s $ ", cwd);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, fp) == NULL) && ferror(fp))
            app_error("fgets error");
        if (feof(fp)) { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }
        if (bash_mode) {
            printf("%s", cmdline);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stderr);
    }

    exit(0); /* control never reaches here */
}

void single_exec(char **argv, int input_fd, int output_fd)
{
    parse_redirect(argv, &input_fd, &output_fd);
    if (output_fd != -1) {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
    if (input_fd != -1) {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    if (execvp(argv[0], argv) < 0) {
        fprintf(stderr, "%s: Command not found.\n", argv[0]);
        exit(1);
    }
}

void pipe_exec(char **argv, int *pos, int cmd_count)
{
    int i, j, k;
    int result;
    int status;
    int pipe_count = cmd_count - 1;
    int pipefds[2 * pipe_count];
    for (i = 0; i < pipe_count; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            fprintf(stderr, "couldn't pipe");
            exit(1);
        }
    }
    j = 0;
    result = 0;
    for (i = 0; i < cmd_count; i++, j += 2) {
        if (fork() == 0) {
            if (i != cmd_count - 1) {
                dup2(pipefds[j + 1], STDOUT_FILENO);
            }
            if (i != 0) {
                dup2(pipefds[j - 2], STDIN_FILENO);
            }
            for (k = 0; k < 2 * pipe_count; k++) {
                if (k != j + 1 && k != j - 2) {
                    close(pipefds[k]);
                }
            }
            single_exec(argv + pos[i], -1, -1);
        }
    }
    for (i = 0; i < 2 * pipe_count; i++) {
        close(pipefds[i]);
    }
    for (i = 0; i < cmd_count; i++) {
        wait(&status);
        result |= status;
    }
    exit(result);
}

void line_exec(int argc, char **argv, int input_fd, int output_fd)
{
    int cmd_postions[MAXPIPE];
    int cmd_count = parse_pipe(argc, argv, cmd_postions);
    if (cmd_count > 1) {
        pipe_exec(argv, cmd_postions, cmd_count);
    } else {
        single_exec(argv, input_fd, output_fd);
    }
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(int argc, char **argv, int input_fd, int output_fd)
{
    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "exit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
        return 1;
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs, output_fd);
        return 1;
    }
    if (!strcmp(argv[0], "cd")) {
        if (argc >= 2) {
            change_dir(argv[1]);
        }
        return 1;
    }
    if (!strcmp(argv[0], "bg") || !(strcmp(argv[0], "fg"))) {
        do_bgfg(argv, output_fd);
        return 1;
    }
    if (!strcmp(argv[0], "fc")) {
        if (argc >= 3 && argv[1][0] == '-' && argv[2][0] == '-') {
            int a = atoi(argv[1] + 1);
            int b = atoi(argv[2] + 1);
            int start = (current - max(a, b) + HISTORY_LIMIT) % HISTORY_LIMIT;
            history_exec(start, sub(a, b) + 1);
        } else {
            fprintf(stderr, "fc: argument error!\n");
        }
        return 1;
    }
    return 0;
}


/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(const char *cmdline)
{
    char *argv[MAXARGS];    /* Argument list execve() */
    int bg;                 /* Should the job run in bg or fg? */
    int argc;
    bg = parse_line(cmdline, &argc, argv);
    if (argv[0] != NULL && !builtin_cmd(argc, argv, STDIN_FILENO, STDOUT_FILENO)) {
        pid_t pid;
        mask_signal(SIG_BLOCK, SIGCHLD);
        if ((pid = fork()) == 0) {   /* Child */
            mask_signal(SIG_UNBLOCK, SIGCHLD);
            if (setpgid(0, 0) < 0) { /* put the child in a new process group */
                unix_error("eval: setpgid failed");
            }
            if (subs_exec(argc, argv) == 0) {
                exit(0);
            }
            exit(1);
        } else {/* Parent */
            if (!bg)
                addjob(jobs, pid, FG, cmdline);
            else
                addjob(jobs, pid, BG, cmdline);

            mask_signal(SIG_UNBLOCK, SIGCHLD);

            /* handle the started job */
            if (!bg)
                waitfg(pid, STDOUT_FILENO);
            else
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }
    }
    if (argv[0] != NULL && strcmp(argv[0], "fc") != 0) {
        save_history(cmdline);
    }
}

void parse_redirect(char **argv, int *input_fd, int *output_fd)
{
    int i = 0;
    int argc = 0;
    int last = 0;
    int fd;
    while (argv[i] != NULL) {
        if (strcmp(argv[i], ">") == 0) {
            last = 1;
        } else if (strcmp(argv[i], "<") == 0) {
            last = 2;
        } else {
            if (last == 0) {
                argv[argc++] = argv[i];
            } else if (last == 1) {
                if ((fd = open(argv[i], O_CREAT | O_TRUNC | O_RDWR, 0644)) == -1) {
                    app_error("Fail to create the file!");
                } else {
                    *output_fd = fd;
                }
            } else if (last == 2) {
                if ((fd = open(argv[i], O_RDONLY)) == -1) {
                    app_error("Fail to open the file!");
                } else {
                    *input_fd = fd;
                }
            }
            last = 0;
        }
        i++;
    }
    argv[argc] = NULL;
}

int parse_pipe(int argc, char **argv, int *cmd_postions)
{
    int j = 1;
    cmd_postions[0] = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0 && i != 0) {
            if (i < argc - 1) {
                cmd_postions[j++] = i + 1;
            }
            argv[i] = NULL;
        }
    }
    return j;
}

void save_history(const char *cmd)
{
    strcpy(cmd_history[current], cmd);
    current = (current + 1) % HISTORY_LIMIT;
}

void history_exec(int start, int n)
{
    const char *cmd;
    if (0 <= start && start < HISTORY_LIMIT) {
        for (int i = 0; i < n; i++) {
            cmd = cmd_history[(start + i) % HISTORY_LIMIT];
            eval(cmd);
        }
    }
}

int subs_exec(int argc, char **argv)
{
    char *subargv[argc];
    char *arg;
    int i, j;
    int size;
    int fds[2];
    int buf_pos = 0;
    int flag = 0;
    int cmd_count = 0;
    int status;
    int result = 0;
    stack s = create_stack(argc);
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], ")") != 0) {
            push(s, argv[i]);
        } else {
            j = 0;
            while (!is_empty(s)) {
                arg = top_and_pop(s);
                if (!strcmp(arg, "<(")) {
                    subargv[j] = NULL;
                    flag = 0;
                    break;
                } else if (!strcmp(arg, ">(")) {
                    subargv[j] = NULL;
                    flag = 1;
                    break;
                } else {
                    subargv[j++] = arg;
                }
            }
            reverse_array(subargv, j);
            pipe(fds);
            if (fork() == 0) {
                close(fds[flag]);
                line_exec(j, subargv, !flag ? -1 : fds[1 - flag], flag ? -1 : fds[1 - flag]);
            } else {
                close(fds[1 - flag]);
                size = sprintf(buf + buf_pos, "/proc/%d/fd/%d", getpid(), fds[flag]) + 1;
                push(s, buf + buf_pos);
                buf_pos += size;
            }
        }
    }
    j = 0;
    while (!is_empty(s)) {
        subargv[j++] = top_and_pop(s);
    }
    subargv[j] = NULL;
    reverse_array(subargv, j);
    line_exec(j, subargv, -1, -1);
    for (i = 0; i < cmd_count; i++) {
        wait(&status);
        result |= status;
    }
    exit(result);
}

/*
 * first_tok - Returns a pointer to the first (lowest addy) of the four pointers
 *     that isn't NULL.
 */
char *first_tok(const char *space, const char *input, const char *output, const char *pipe, const char *right)
{
    const char *possible[5];
    unsigned long min;
    int n = 0;
    if (space != NULL) {
        possible[n++] = space;
    }
    if (input != NULL) {
        possible[n++] = input;
    }
    if (output != NULL) {
        possible[n++] = output;
    }
    if (pipe != NULL) {
        possible[n++] = pipe;
    }
    if (right != NULL) {
        possible[n++] = right;
    }
    if (n == 0) {
        return NULL;
    }
    min = (unsigned long) possible[0];
    for (int i = 1; i < n; i++) {
        if (((unsigned long) possible[i]) < min)
            min = (unsigned long) possible[i];
    }
    return (char *) min;
}

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
* parse_line - Parse the command line and build the argv array.
* Return true (1) if the user has requested a BG job, false
* if the user has requested a FG job.
*/
int parse_line(const char *cmdline, int *p_argc, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim_space;          /* points to first space delimiter */
    char *delim_in;             /* points to the first < delimiter */
    char *delim_out;            /* points to the first > delimiter */
    char *delim_pipe;           /* points to the first | delimiter */
    char *delim_right;          /* points to the first ) delimiter */
    char *delim;                /* points to the first delimiter */
    int argc;               /* number of args */
    int bg;                     /* background job? */
    char *last_space = NULL;    /* The address of the last space  */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    delim_space = strchr(buf, ' ');
    delim_in = strchr(buf, '<');
    delim_out = strchr(buf, '>');
    delim_pipe = strchr(buf, '|');
    delim_right = strchr(buf, ')');
    while ((delim = first_tok(delim_space, delim_in, delim_out, delim_pipe, delim_right))) {
        if (delim == delim_space) {
            *delim = '\0';
            if (strlen(buf) != 0) {
                argv[argc++] = buf;
            }
            last_space = delim;
        } else if (delim == delim_in) {
            if ((last_space && last_space != (delim - 1)) || !last_space) {
                *delim = '\0';
                if (strlen(buf) != 0) {
                    argv[argc++] = buf;
                }
            }
            if (*(delim + 1) == '(') {
                delim++;
                argv[argc++] = "<(";
            } else {
                argv[argc++] = "<";
            }
            last_space = 0;
        } else if (delim == delim_out) {
            if ((last_space && last_space != (delim - 1)) || !last_space) {
                *delim = '\0';
                if (strlen(buf) != 0) {
                    argv[argc++] = buf;
                }
            }
            if (*(delim + 1) == '(') {
                delim++;
                argv[argc++] = ">(";
            } else {
                argv[argc++] = ">";
            }
            last_space = 0;
        } else if (delim == delim_pipe) {
            if ((last_space && last_space != (delim - 1)) || !last_space) {
                *delim = '\0';
                if (strlen(buf) != 0) {
                    argv[argc++] = buf;
                }
            }
            argv[argc++] = "|";
            last_space = 0;
        } else if (delim == delim_right) {
            if ((last_space && last_space != (delim - 1)) || !last_space) {
                *delim = '\0';
                if (strlen(buf) != 0) {
                    argv[argc++] = buf;
                }
            }
            argv[argc++] = ")";
            last_space = 0;
        }
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;
        delim_space = strchr(buf, ' ');
        delim_in = strchr(buf, '<');
        delim_out = strchr(buf, '>');
        delim_pipe = strchr(buf, '|');
        delim_right = strchr(buf, ')');
    }
    argv[argc] = NULL;
    if (argc == 0)  /* ignore blank line */
        return 1;
    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL;
    *p_argc = argc;
    return bg;
}

void change_dir(const char *path)
{
    if (chdir(path) < 0) {
        if (errno == ENOTDIR) {
            printf("cd: %s: Not a directory\n", path);
        } else if (errno == ENOENT) {
            printf("cd: %s: No such file or directory\n", path);
        } else {
            printf("cd: %s: fail to change directory\n", path);
        }
    }
}
