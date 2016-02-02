#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "errmsg.h"

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}
