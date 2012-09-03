#include "../common.h"
#include "fastdfs_store.h"

#define FDFS_MOD_REPONSE_MODE_PROXY	'P'
#define FDFS_MOD_REPONSE_MODE_REDIRECT	'R'
#define FDFS_MOD_REPONSE_MODE_CLIENT 'C'

TrackerServerInfo *ptracker_server;


static result_t fastdfs_init(char *cfg) {
    result_t result = RESULT_UNSET;

    //TODO:to be continue

    return result;
}


static result_t fastdfs_save_file(char *file_buff, void *cfg) {
    result_t result = RESULT_UNSET;

    //TODO:to be continue

    return result;
}

static result_t fastdfs_save_slave_file(char *file_buff, int64_t file_size, char *orig_filename, char *suffix, char *file_extension, char *last_filename) {
    int result = 0;
    result = storage_upload_slave_by_filebuff1(ptracker_server, NULL, file_buff, file_size,\
                                            orig_filename, suffix, file_extension, NULL, 0, last_filename);
    if(result == 0) {
        return RESULT_SUCCESS;
    }

    result = RESULT_FAILED;
}

static result_t fastdfs_fetch_file(char *file_id, char **file_buff, int64_t *file_size) {
    int result = 0;

    result = storage_download_file1(ptracker_server, NULL, file_id, file_buff, file_size);
    if(result == 0) {
        return RESULT_SUCCESS;
    }

    result = RESULT_FAILED;
}



static result_t fastdfs_remove_file(char *file_id) {
    result_t result = RESULT_UNSET;
    //TODO:to be continue
    return result;
}

static result_t fastdfs_get_file_id(char *file_id) {
    result_t result = RESULT_UNSET;
    //TODO:to be continue
    return result;
}

static result_t fastdfs_get_file_info(char *file_id, file_info_t *fi) {
    FDFSFileInfo file_info;
    int res = fdfs_get_file_info1(file_id, &file_info);
    if(res == 0) {
        fi->create_timestamp = file_info.create_timestamp;
        return RESULT_SUCCESS;
    }

    return RESULT_FAILED;
}


store_plugin_t *fastdfs_store_init(char *cfg) {
    store_plugin_t *sp = store_plugin_init();
    sp->version           = LIGHTTPD_VERSION_ID;
    strcpy(sp->name, "fastdfs");

    //sp->init              = fastdfs_init;
    //sp->save_file         = fastdfs_save_file;
    sp->save_slave_file     = fastdfs_save_slave_file;
    sp->fetch_file          = fastdfs_fetch_file;
    sp->get_file_info       = fastdfs_get_file_info;
    //sp->remove_file       = fastdfs_remove_file;
    //sp->get_file_id       = fastdfs_get_file_id;


    sp->data                = NULL;

    //log_init();
	//g_log_context.log_level = LOG_ERR;

    int result;
    if ( (result = fdfs_client_init(cfg) ) != 0) {
        return NULL;
    }
    ptracker_server = tracker_get_connection();
    if(ptracker_server == NULL) {
        return NULL;
    }

    return sp;
}

void *fastdfs_store_free(store_plugin_t *sp) {
    store_plugin_free(sp);

    tracker_close_all_connections();
	fdfs_client_destroy();

	//log_destroy();
}










