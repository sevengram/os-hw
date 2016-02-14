#include <string.h>
#include <stdlib.h>
#include "linked_hash_table.h"
#include "errmsg.h"

#define HASH_TABLE_SIZE_EXP 8

typedef unsigned int index_t;

struct linked_node_record;

typedef struct linked_node_record *linked_node;

struct linked_node_record
{
    linked_node next;
    linked_node dnext;
    linked_node dprev;
    hkey_t key;
    value_t value;
};

struct linked_ht_record
{
    int r;
    index_t capacity;
    index_t size;
    linked_node dheader;
    linked_node *headers;
};

index_t bkdr_hash(hkey_t str, index_t cap)
{
    unsigned int seed = 131;
    unsigned int hash = 0;
    while (*str) {
        hash = hash * seed + (*str++);
    }
    return (hash & 0x7FFFFFFF) % cap;
}

linked_ht create_linked_ht()
{
    linked_ht t;
    if ((t = malloc(sizeof(struct linked_ht_record))) == NULL) {
        app_error("out of space!!");
        return NULL;
    }
    t->r = HASH_TABLE_SIZE_EXP;
    t->capacity = 1U << t->r;
    t->size = 0;
    if ((t->dheader = malloc(sizeof(struct linked_node_record))) == NULL) {
        app_error("out of space!!");
        free(t);
        return NULL;
    }
    t->dheader->dnext = NULL;
    t->dheader->dprev = NULL;
    if ((t->headers = malloc(sizeof(linked_node) * t->capacity)) == NULL) {
        app_error("out of space!!");
        free(t);
        free(t->dheader);
        return NULL;
    }
    for (index_t i = 0; i < t->capacity; i++) {
        if ((t->headers[i] = malloc(sizeof(struct linked_node_record))) == NULL) {
            t->capacity = i;
            app_error("out of space!!");
            break;
        }
        t->headers[i]->next = NULL;
    }
    return t;
}

int is_empty_linked_ht(linked_ht t)
{
    return t->size == 0;
}

int size_linked_ht(linked_ht t)
{
    return t->size;
}

void clear_linked_ht(linked_ht t)
{
    for (index_t i = 0; i < t->capacity; i++) {
        while (t->headers[i]->next != NULL) {
            linked_node n = t->headers[i]->next;
            t->headers[i]->next = n->next;
            free(n->value);
            free(n->key);
            free(n);
        }
    }
    t->size = 0;
}

void dispose_linked_ht(linked_ht t)
{
    clear_linked_ht(t);
    for (index_t i = 0; i < t->capacity; i++) {
        free(t->headers[i]);
    }
    free(t->headers);
    free(t->dheader);
    free(t);
}

void insert_dnext(linked_node a, linked_node b)
{
    if (a && b) {
        b->dnext = a->dnext;
        if (a->dnext) {
            a->dnext->dprev = b;
        }
        a->dnext = b;
        b->dprev = a;
    }
}

int put_linked_ht(linked_ht t, hkey_t k, value_t v)
{
    index_t i = bkdr_hash(k, t->capacity);
    linked_node p = t->headers[i];
    linked_node node;
    while (p->next != NULL && strcmp(p->next->key, k) != 0) {
        p = p->next;
    }
    if (p->next == NULL) {
        if ((node = malloc(sizeof(struct linked_node_record))) == NULL) {
            app_error("out of space!!");
            return -1;
        }
        if ((node->key = malloc(strlen(k) + 1)) == NULL) {
            app_error("out of space!!");
            return -1;
        }
        strcpy(node->key, k);
        if ((node->value = malloc(strlen(v) + 1)) == NULL) {
            app_error("out of space!!");
            return -1;
        }
        strcpy(node->value, v);
        node->next = NULL;
        p->next = node;
        insert_dnext(t->dheader, node);
        t->size++;
    } else {
        free(p->next->value);
        if ((p->next->value = malloc(strlen(v) + 1)) == NULL) {
            app_error("out of space!!");
            return -1;
        }
        strcpy(p->next->value, v);
    }
    return 0;
}

linked_node search_linked_ht(linked_ht t, hkey_t k)
{
    index_t i = bkdr_hash(k, t->capacity);
    linked_node p = t->headers[i];
    while (p->next != NULL && strcmp(p->next->key, k) != 0) {
        p = p->next;
    }
    return p->next;
}

value_t get_linked_ht(linked_ht t, hkey_t k)
{
    linked_node result = search_linked_ht(t, k);
    return result == NULL ? NULL : result->value;
}

int remove_linked_ht(linked_ht t, hkey_t k)
{
    index_t i = bkdr_hash(k, t->capacity);
    linked_node p = t->headers[i];
    while (p->next != NULL && strcmp(p->next->key, k) != 0) {
        p = p->next;
    }
    if (p->next != NULL) {
        linked_node n = p->next;
        p->next = n->next;
        n->dprev->dnext = n->dnext;
        n->dnext->dprev = n->dprev;
        free(n->value);
        free(n->key);
        free(n);
        t->size--;
        return 0;
    } else {
        return -1;
    }
}

void get_all_linked_ht_data(linked_ht t, hkey_t *keys, value_t *values, int size)
{
    linked_node n = t->dheader->dnext;

    for (int i = 0; i < size; i++) {
        if (n == NULL) {
            break;
        }
        keys[i] = n->key;
        values[i] = n->value;
        n = n->dnext;
    }
}