#ifndef FASTDFS_STORE_H_INCLUDED
#define FASTDFS_STORE_H_INCLUDED

#include <logger.h>
#include <fdfs_client.h>

#include "../common.h"


store_plugin_t *fastdfs_store_init(char *cfg);
void *fastdfs_store_free();

#endif // FASTDFS_STORE_H_INCLUDED
