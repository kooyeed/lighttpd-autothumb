#ifndef REDIS_STORE_H_INCLUDED
#define REDIS_STORE_H_INCLUDED

#include "../common.h"

store_plugin_t *redis_store_init(char *cfg);
void *redis_store_free();

#endif // REDIS_STORE_H_INCLUDED
