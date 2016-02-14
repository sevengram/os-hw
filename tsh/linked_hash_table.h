#ifndef ALGORITHM_HASH_TABLE_H
#define ALGORITHM_HASH_TABLE_H

typedef char *hkey_t;

typedef char *value_t;

typedef struct linked_ht_record *linked_ht;

linked_ht create_linked_ht();

int is_empty_linked_ht(linked_ht t);

int size_linked_ht(linked_ht t);

void clear_linked_ht(linked_ht t);

void dispose_linked_ht(linked_ht t);

int put_linked_ht(linked_ht t, hkey_t k, value_t v);

int remove_linked_ht(linked_ht t, hkey_t k);

value_t get_linked_ht(linked_ht t, hkey_t k);

void get_all_linked_ht_data(linked_ht t, hkey_t *keys, value_t *values, int size);

#endif //ALGORITHM_HASH_TABLE_H
