#ifndef FILE_STORE_H_INCLUDED
#define FILE_STORE_H_INCLUDED

#include "../common.h"

store_plugin_t *file_store_init(char *cfg);
void *file_store_free();

#endif // FILE_STORE_H_INCLUDED
