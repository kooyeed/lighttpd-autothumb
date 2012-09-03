#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
//#define _GNU_SOURCE
//#define HAVE_SOCKLEN_T 1
#include <string.h>


#define HTTPD_MAX_PARAMS        32  /** the maximum number of URI's parameters */

#define FI_FILE_EXTENSION_SIZE  5   /** the maximum length  of file extension */
#define FI_FULL_FILENAME_SIZE   128 /** the maximum length  of full filename */
#define FI_ORIG_FILENAME_SIZE   128 /** the maximum length  of original filename */
#define FI_OUTER_FORMAT_SIZE    16  /** the maximum length  of format string */
#define FI_INNER_FORMAT_SIZE    32  /** the maximum length  of inner format string */
#define FI_SUFFIX_SIZE          32  /** the maximum length  of salve file suffix */
#define FI_FILESIZE_SIZE        10  /** the maximum length  of file size */
#define MAX_SUFFIX_SIZE         12  


#define LIGHTTPD_VERSION_ID (1 << 16 | 4 << 8 | 28)

typedef struct {
    char *enable;
}config_t;

/** pitcure full information */
typedef struct {
    size_t rotate_degree;
    size_t quality;       

    char *file_extension;   
    char *full_filename;   
    char *orig_filename;   
    char *outer_format;         /** not include first char '_'  */
    char *inner_format;         /** imagemagick format string   */
    char *suffix;               /** file suffix                 */
    int  convert_type;          /** 1: resize, 2: crop          */

    time_t create_timestamp;    /** file create time            */
} file_info_t;

/** file store type */
typedef enum {
    LOCAL_FILE,
    FASTDFS,
    REDIS,
    MONGODB_GRIDFS,
    STORE_TYPE_SIZE
} store_type;

typedef enum {
    RESULT_UNSET,
    RESULT_SUCCESS,
    RESULT_FAILED,
    RESULT_EXCEPTION,
    RESULT_T_SIZE
} result_t;


typedef struct {
	char *key;
	char *value;
} key_value_pair_t;


/**
* file store plugin
* each plug-in are required to implement these virtual functions
*/
typedef struct {
    size_t version;
    char *name;


    //result_t (* init)              (char *cfg);
	//result_t (* save_file)         (char *file_buff, void *cfg);
	result_t (* save_slave_file)   (char *file_buff, int64_t file_size, char *orig_filename, char *suffix, char *file_extension, char *last_filename);
	result_t (* fetch_file)        (char *file_id, char **file_buff, int64_t *file_size);
	//result_t (* remove_file)       (char *file_id);
	//result_t (* get_file_id)       (char *file_id);
	result_t (* get_file_info)     (char *file_id, file_info_t *fi);

	void *data;
} store_plugin_t;

//void file_info_init(file_info_t *fi);
file_info_t *file_info_init();
void file_info_free(file_info_t *fi);

store_plugin_t *store_plugin_init();
void store_plugin_free(store_plugin_t *sp);

/** resolve the URI address, and access to basic information */
int parse_uri(char *uri, file_info_t *fi);

/** 
* The following two functions is defined in FDFS actually,
* but release coupling,redefine here
*/
int parse_uri_params(char *url, key_value_pair_t *params, const int max_count);
char *get_uri_param(const char *param_name, key_value_pair_t *params, const int param_count);

#endif // COMMON_H_INCLUDED
