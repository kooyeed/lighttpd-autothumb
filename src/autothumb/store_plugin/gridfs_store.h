#ifndef GRIDFS_STORE_H_INCLUDED
#define GRIDFS_STORE_H_INCLUDED

#include "../common.h"

store_plugin_t *gridfs_store_init(char *cfg);
void *gridfs_store_free();

#endif // GRIDFS_STORE_H_INCLUDED
