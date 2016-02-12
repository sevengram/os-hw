#ifndef OS_HW_UTIL_H
#define OS_HW_UTIL_H

#define max(x, y) (((x) > (y)) ? (x) : (y))

#define min(x, y) (((x) < (y)) ? (x) : (y))

#define sub(x, y) (((x) > (y)) ? ((x)-(y)) : ((y)-(x)))

void reverse_array(char **array, int size);

void print_argv(int argc, char **argv);

#endif //OS_HW_UTIL_H
