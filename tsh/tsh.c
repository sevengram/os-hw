/*
 * tsh - A tiny shell program with job control
 *
 * jianxiang.fan@colorado.edu - Jianxiang Fan
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "errmsg.h"
#include "job.h"
#include "sigutil.h"

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */

/* command line prompt */
char prompt[] = "tsh> ";

void eval(char *cmdline);

int builtin_cmd(char **argv, int input_fd, int output_fd);

int parseline(const char *cmdline, char **argv);

char *first_tok(const char *space, const char *input, const char *output, const char *pipe);

void usage(void);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

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
                emit_prompt = 0;  /* handy for automatic testing */
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

    /* Execute the shell's read/eval loop */
    while (1) {
        /* Read command line */
        if (emit_prompt) {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin)) { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stderr);
    }

    exit(0); /* control never reaches here */
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
void eval(char *cmdline)
{
    char *argv[MAXARGS];    /* Argument list execve() */
    char buf[MAXLINE];
    int bg;                 /* Should the job run in bg or fg? */
    pid_t pid;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);

    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    if (!builtin_cmd(argv, STDIN_FILENO, STDOUT_FILENO)) {
        mask_signal(SIG_BLOCK, SIGCHLD);

        if ((pid = fork()) == 0) {   /* Child */
            mask_signal(SIG_UNBLOCK, SIGCHLD);

            if (setpgid(0, 0) < 0) { /* put the child in a new process group */
                unix_error("eval: setpgid failed");
            }

            /* run the command */
            if (execvp(argv[0], argv) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(1);
            }
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
    return;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv, int input_fd, int output_fd)
{
    if (!strcmp(argv[0], "quit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
        return 1;
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs, output_fd);
        return 1;
    }
    if (!strcmp(argv[0], "bg") || !(strcmp(argv[0], "fg"))) {
        do_bgfg(argv, output_fd);
        return 1;
    }
    return 0;
}

/*
* parseline - Parse the command line and build the argv array.
* Return true (1) if the user has requested a BG job, false
* if the user has requested a FG job.
*/
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim_space;          /* points to first space delimiter */
    char *delim_in;             /* points to the first < delimiter */
    char *delim_out;            /* points to the first > delimiter */
    char *delim_pipe;           /* points to the first | delimiter */
    char *delim;                /* points to the first delimiter */
    int argc;                   /* number of args */
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
    while ((delim = first_tok(delim_space, delim_in, delim_out, delim_pipe))) {
        if (delim == delim_space) {
            argv[argc++] = buf;
            *delim = '\0';
            last_space = delim;
        } else if (delim == delim_in) {
            if ((last_space && last_space != (delim - 1)) || (!last_space)) {
                argv[argc++] = buf;
                *delim = '\0';
            }
            argv[argc++] = "<";
            last_space = 0;
        } else if (delim == delim_out) {
            if ((last_space && last_space != (delim - 1)) || (!last_space)) {
                argv[argc++] = buf;
                *delim = '\0';
            }
            argv[argc++] = ">";
            last_space = 0;
        } else if (delim == delim_pipe) {
            if ((last_space && last_space != (delim - 1)) || (!last_space)) {
                argv[argc++] = buf;
                *delim = '\0';
            }
            argv[argc++] = "|";
            last_space = 0;
        }
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;
        delim_space = strchr(buf, ' ');
        delim_in = strchr(buf, '<');
        delim_out = strchr(buf, '>');
        delim_pipe = strchr(buf, '|');
    }
    argv[argc] = NULL;

    if (argc == 0)  /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL;

    return bg;
}

/*
 * first_tok - Returns a pointer to the first (lowest addy) of the four pointers
 *     that isn't NULL.
 */
char *first_tok(const char *space, const char *input, const char *output, const char *pipe)
{
    const char *possible[4];
    unsigned int min;
    int i = 1, n = 0;
    if (space == NULL && input == NULL && output == NULL && pipe == NULL)
        return NULL;
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
    min = (unsigned) possible[0];
    for (; i < n; i++) {
        if (((unsigned) possible[i]) < min)
            min = (unsigned) possible[i];
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
