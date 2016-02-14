#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "bookmark.h"
#include "errmsg.h"
#include "linked_hash_table.h"

#define BUFSIZE 1024

linked_ht bookmarks;

static const char *default_file = ".tshinfo";

const char *get_home_dir()
{
    struct passwd *pw = getpwuid(getuid());
    return pw->pw_dir;
}

int load_bookmarks(char *filename)
{
    if (bookmarks == NULL) {
        if ((bookmarks = create_linked_ht()) == NULL) {
            return -1;
        }
    }
    FILE *fp;
    char buf[BUFSIZE];
    char filebuf[BUFSIZE];
    char alias[BUFSIZE];
    size_t count;
    if (!filename) {
        filename = filebuf;
        sprintf(filename, "%s/%s", get_home_dir(), default_file);
    }
    if ((fp = fopen(filename, "r")) == NULL) {
        return 0;
    }
    while (!feof(fp)) {
        if (fgets(buf, BUFSIZE, fp) != NULL) {
            count = strlen(buf);
            if (count > 1 && buf[count - 1] == '\n') {
                buf[count - 1] = '\0';
            }
            if (strlen(buf) != 0) {
                if (buf[0] != '/') {
                    strcpy(alias, buf);
                } else if (put_linked_ht(bookmarks, alias, buf) < 0) {
                    return -1;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}

int save_bookmarks(char *filename)
{
    if (bookmarks == NULL) {
        return 0;
    }
    FILE *fp;
    int size = size_linked_ht(bookmarks);
    char *keys[size];
    char *values[size];
    char filebuf[BUFSIZE];
    get_all_linked_ht_data(bookmarks, keys, values, size);
    if (!filename) {
        filename = filebuf;
        sprintf(filename, "%s/%s", get_home_dir(), default_file);
    }
    if ((fp = fopen(filename, "w")) == NULL) {
        app_error("can't open bookmark file!");
        return -1;
    }
    for (int i = 0; i < size; i++) {
        fprintf(fp, "%s\n", keys[i]);
        fprintf(fp, "%s\n", values[i]);
    }
    fclose(fp);
    return 0;
}

void list_bookmarks()
{
    if (bookmarks == NULL) {
        return;
    }
    int size = size_linked_ht(bookmarks);
    char *keys[size];
    char *values[size];
    get_all_linked_ht_data(bookmarks, keys, values, size);
    for (int i = 0; i < size; i++) {
        printf("%s => %s\n", keys[i], values[i]);
    }
}

char *get_bookmark(char *alias)
{
    return bookmarks != NULL ? get_linked_ht(bookmarks, alias) : NULL;
}

int remove_bookmark(char *alias)
{
    return bookmarks != NULL ? remove_linked_ht(bookmarks, alias) : -1;
}

int add_bookmark(char *alias, char *path)
{
    if (bookmarks == NULL) {
        if ((bookmarks = create_linked_ht()) == NULL) {
            return -1;
        }
    }
    return put_linked_ht(bookmarks, alias, path);
}
