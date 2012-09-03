#ifndef STORE_H_INCLUDED
#define STORE_H_INCLUDED

#include "common.h"
#include "store_plugin/fastdfs_store.h"
#include "store_plugin/file_store.h"
//#include "store_plugin/gridfs_store.h"
//#include "store_plugin/redis_store.h"

#define STORE_INIT(type, cfg) \
    type##_store_init(cfg)

#define STORE_FREE(type, sp) \
    type##_store_free(sp)


#endif // STORE_H_INCLUDED










