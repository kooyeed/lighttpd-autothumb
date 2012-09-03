#include "store.h"

/** should be used for conditional compilation */
#include "store_plugin/fastdfs_store.h"
#include "store_plugin/file_store.h"
#include "store_plugin/gridfs_store.h"
#include "store_plugin/redis_store.h"







int main() {
    store_plugin_t *sp = NULL;
    sp = STORE_INIT(file, "/home/tony/app/fastdfs-3.0.6/data2/file.conf");

    char *f = NULL;

    char *p = "HI";

    char **pp = &p;

    //char *file_id = "file.conf";
    char *file_id = "mm.jpg";
    char *file_buff;
    int64_t file_size;
    //sp->fetch_file(file_id, &file_buff, &file_size);
    //printf("file_size:[%d]\n", file_size);
    //sp->fetch_file(file_id, &file_buff, &file_size);
    //printf("file_size:[%d]\n", file_size);
    //sp->get_file_info(file_id, NULL);


    //

    STORE_FREE(file, sp);

    printf("over\n");
}



