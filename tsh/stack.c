#include <stddef.h>
#include <malloc.h>
#include "stack.h"
#include "errmsg.h"

#define EMPTY_TOS (-1)


struct stack_record
{
    int capacity;
    int top_of_stack;
    element_t *array;
};

int get_capacity(stack s)
{
    return s->capacity;
}

int is_empty(stack s)
{
    return s->top_of_stack == EMPTY_TOS;
}

int is_full(stack s)
{
    return s->top_of_stack == s->capacity - 1;
}

stack create_stack(int capacity)
{
    if (capacity < 0) {
        app_error("invalid stack size!");
    }
    stack s = malloc(sizeof(struct stack_record));
    if (s == NULL) {
        unix_error("out of space!!");
    }
    s->array = malloc(sizeof(element_t) * capacity);
    if (s->array == NULL) {
        unix_error("out of space!!");
    }
    s->capacity = capacity;
    make_empty(s);
    return s;
}

void dispose_stack(stack s)
{
    if (s->array != NULL) {
        free(s->array);
    }
    free(s);
}

void make_empty(stack s)
{
    s->top_of_stack = EMPTY_TOS;
}

void push(stack s, element_t e)
{
    if (is_full(s)) {
        app_error("full stack");
    } else {
        s->array[++s->top_of_stack] = e;
    }
}

element_t top(stack s)
{
    if (is_empty(s)) {
        app_error("empty stack");
        return NULL;
    } else {
        return s->array[s->top_of_stack];
    }
}

void pop(stack s)
{
    if (is_empty(s)) {
        app_error("empty stack");
    } else {
        s->top_of_stack--;
    }
}

element_t top_and_pop(stack s)
{
    if (is_empty(s)) {
        app_error("empty stack");
        return NULL;
    } else {
        return s->array[s->top_of_stack--];
    }
}
