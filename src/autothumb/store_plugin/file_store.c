#include <sys/stat.h>
#include <unistd.h>

#include "../common.h"
#include "file_store.h"


char store_root[128];

static result_t file_init(char *cfg) {
    result_t result = RESULT_UNSET;
    //TODO:to be continue
    return result;
}

static result_t file_save_file(char *file_buff, void *cfg) {
    result_t result = RESULT_UNSET;
    //TODO:to be continue
    return result;
}

static result_t file_save_slave_file(char *file_buff, int64_t file_size, char *orig_filename, char *suffix, char *file_extension, char *last_filename) {    
    char *p;
    if( 0 == (p = strrchr(orig_filename, '.')) ) {
        return RESULT_FAILED;
    }
    *p = '\0';
    sprintf(last_filename, "%s/%s%s.%s", store_root, orig_filename, suffix, file_extension);

    FILE *fp = fopen(last_filename, "wb");
    if(NULL == fp) {
        return RESULT_FAILED;
    }
    fwrite(file_buff, file_size, 1, fp);
    fclose(fp);    

    return RESULT_SUCCESS;
}

static result_t file_fetch_file(char *file_id, char **file_buff, int64_t *file_size) {
    FILE *fp;    
    char filename[256];
    sprintf(filename, "%s/%s", store_root, file_id);

    if ((fp = fopen(filename, "rb")) != NULL) {
        if(0 == fseek(fp, 0, SEEK_END) ) {
            *file_size = ftell(fp);
        }
        else {
            return RESULT_FAILED;
        }
        rewind(fp);        
        *file_buff = (char*)malloc(sizeof(char) * (*file_size) ); 
        fread(*file_buff, 1, *file_size, fp);
        fclose(fp);
        return RESULT_SUCCESS;
    }

    return RESULT_FAILED;
}

static result_t file_remove_file(char *file_id) {
    result_t result = RESULT_UNSET;
    //TODO:to be continue
    return result;
}


static result_t file_get_file_id(char *file_id) {
    result_t result = RESULT_UNSET;
    //TODO:to be continue
    return result;
}

static result_t file_get_file_info(char *file_id, file_info_t *fi) {
    struct stat buf;
    char filename[256];
    sprintf(filename, "%s/%s", store_root, file_id);
    if( 0 == stat(filename, &buf) ) {
        fi->create_timestamp = buf.st_mtime;
        return RESULT_SUCCESS;
    } 
    return RESULT_FAILED;
}

static result_t file_client_init(char *cfg) {    
    FILE *fp = fopen(cfg, "r");
    if( NULL != fp && NULL == fgets(store_root, 128, fp) ) {
        return RESULT_FAILED;
    }
    fclose(fp);

    return RESULT_SUCCESS;
}


store_plugin_t *file_store_init(char *cfg) {
    store_plugin_t *sp = store_plugin_init();
    sp->version           = LIGHTTPD_VERSION_ID;
    strcpy(sp->name, "file");

    //sp->init              = file_init;
    //sp->save_file         = file_save_file;
    sp->save_slave_file     = file_save_slave_file;
    sp->fetch_file          = file_fetch_file;
    sp->get_file_info       = file_get_file_info;
    //sp->remove_file       = file_remove_file;
    //sp->get_file_id       = file_get_file_id;


    sp->data                = NULL;

    if ( RESULT_FAILED == file_client_init(cfg) ) {
        return NULL;
    }

    return sp;
}

void *file_store_free(store_plugin_t *sp) {
    store_plugin_free(sp);
}





