#include <stdio.h>

/*
 * For debug
 */
void print_argv(int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }
}

void reverse_array(char **array, int size)
{
    char *t;
    for (int i = 0; i < size / 2; i++) {
        t = array[i];
        array[i] = array[size - 1 - i];
        array[size - 1 - i] = t;
    }
}
