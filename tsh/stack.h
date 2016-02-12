#ifndef ALGORITHM_STACK_H
#define ALGORITHM_STACK_H

typedef char *element_t;

struct stack_record;

typedef struct stack_record *stack;

int get_capacity(stack s);

int is_empty(stack s);

int is_full(stack s);

stack create_stack(int capacity);

void dispose_stack(stack s);

void make_empty(stack s);

void push(stack s, element_t e);

element_t top(stack s);

void pop(stack s);

element_t top_and_pop(stack s);

#endif //ALGORITHM_STACK_H
