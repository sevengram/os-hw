#ifndef OS_HW_BOOKMARK_H
#define OS_HW_BOOKMARK_H

int load_bookmarks(char *filename);

int save_bookmarks(char *filename);

char *get_bookmark(char *alias);

int remove_bookmark(char *alias);

int add_bookmark(char *alias, char *path);

void list_bookmarks();

#endif //OS_HW_BOOKMARK_H
