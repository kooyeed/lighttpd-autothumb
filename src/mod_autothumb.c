//#define FADTDFSMAGICK_DEBUG
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#define _GNU_SOURCE
#include <string.h>
#include <logger.h>
#include <time.h>

#include "base.h"
#include "log.h"
#include "buffer.h"
#include "plugin.h"
#include "array.h"

#include "autothumb/autothumb.h"

#define DEBUG_AUTOTHUMB 1

#ifdef DEBUG_AUTOTHUMB
long timer(int reset) {
	static long start = 0;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	/* return timediff */
	if (!reset) {
		long stop = ((long) tv.tv_sec) * 1000000 + tv.tv_usec ;
		return (stop - start);
	}

	/* reset timer */
	start = ((long) tv.tv_sec) *1000000  + tv.tv_usec  ;

	return 0;
}
#endif

/** defined private function */
handler_t _transfer_format(server *srv, connection *con, void *p_d, file_info_t *fi);

//-----------------private lighttpd interface-----------------------------------
/** defined in lighttpd conf file */
#define CONFIG_AUTOTHUMB_ENABLE                 "autothumb.enable"
#define CONFIG_AUTOTHUMB_CONVERT_IMG_TYPES      "autothumb.convert-filetypes"
#define CONFIG_AUTOTHUMB_CONVERT_FORMATS        "autothumb.convert-formats"
#define CONFIG_AUTOTHUMB_STORE_TYPE             "autothumb.store-type"
#define CONFIG_AUTOTHUMB_STORE_CONF             "autothumb.store-conf"

#define CONFIG_AUTOTHUMB_STYLE_BGCOLOR          "autothumb.style-bgcolor"
#define CONFIG_AUTOTHUMB_STYLE_WATER_IMAGE      "autothumb.style-water-image"
#define CONFIG_AUTOTHUMB_STYLE_WATER_TEXT       "autothumb.style-water-text"

typedef struct {
    unsigned short enable;  /** is enable this module */
    array *file_types;      /** support file types */
    array *file_formats;    /** support file formats */    
    buffer *store_type;     /** store type */
    buffer *store_conf;     /** store conf file */

    buffer *style_bgcolor;          
    buffer *style_water_image;      
    buffer *style_water_text;    
} plugin_config;

typedef struct {
    PLUGIN_DATA;
    plugin_config **config_storage;
    plugin_config conf;

} plugin_data;


#define PATCH(x) \
	p->conf.x = s->x;
static int _mod_autothumb_patch_connection(server *srv, connection *con, plugin_data *p) {
	size_t i, j;
	plugin_config *s = p->config_storage[0];
	PATCH(enable);
	PATCH(file_types);
    PATCH(file_formats);
	PATCH(store_type);
	PATCH(store_conf);
    PATCH(style_bgcolor);
    PATCH(style_water_image);
    PATCH(style_water_text);


    /* skip the first, the global context */
	for (i = 1; i < srv->config_context->used; i++) {
		data_config *dc = (data_config *) srv->config_context->data[i];
		s = p->config_storage[i];

		/* condition didn't match */
		if (!config_check_cond(srv, con, dc)) {
		    continue;
		}

        /* merge config */
		for (j = 0; j < dc->value->used; j++) {
			data_unset *du = dc->value->data[j];
			if (buffer_is_equal_string(du->key, CONST_STR_LEN(CONFIG_AUTOTHUMB_ENABLE))) {
			    PATCH(enable);
			}
			else if (buffer_is_equal_string(du->key, CONST_STR_LEN(CONFIG_AUTOTHUMB_CONVERT_IMG_TYPES))) {
			    PATCH(file_types);
			}
            else if (buffer_is_equal_string(du->key, CONST_STR_LEN(CONFIG_AUTOTHUMB_CONVERT_FORMATS))) {
                PATCH(file_formats);
            }
			else if (buffer_is_equal_string(du->key, CONST_STR_LEN(CONFIG_AUTOTHUMB_STORE_TYPE))) {
			    PATCH(store_type);
			}
			else if (buffer_is_equal_string(du->key, CONST_STR_LEN(CONFIG_AUTOTHUMB_STORE_CONF))) {
			    PATCH(store_conf);
			}
            else if (buffer_is_equal_string(du->key, CONST_STR_LEN(CONFIG_AUTOTHUMB_STYLE_BGCOLOR))) {
                PATCH(style_bgcolor);
            }
            else if (buffer_is_equal_string(du->key, CONST_STR_LEN(CONFIG_AUTOTHUMB_STYLE_WATER_IMAGE))) {
                PATCH(style_water_image);
            }
            else if (buffer_is_equal_string(du->key, CONST_STR_LEN(CONFIG_AUTOTHUMB_STYLE_WATER_TEXT))) {
                PATCH(style_water_text);
            }
		}
	}
	return 0;
}
#undef PATCH_OPTION

plugin_data *_mod_autothumb_init() {
	plugin_data *p;

	p = calloc(1, sizeof(*p));

    /** init MagickWand handle */
	init_thumbnail();

	return p;
}

handler_t _mod_autothumb_set_defaults(server *srv, void *p_d) {
	plugin_data *p = p_d;
	size_t i = 0;

	config_values_t cv[] = {
		{ CONFIG_AUTOTHUMB_ENABLE, NULL, T_CONFIG_BOOLEAN, T_CONFIG_SCOPE_CONNECTION },

		{ CONFIG_AUTOTHUMB_CONVERT_IMG_TYPES, NULL, T_CONFIG_ARRAY, T_CONFIG_SCOPE_CONNECTION },
		{ CONFIG_AUTOTHUMB_CONVERT_FORMATS, NULL, T_CONFIG_ARRAY, T_CONFIG_SCOPE_CONNECTION },

		{ CONFIG_AUTOTHUMB_STORE_TYPE, NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION },
		{ CONFIG_AUTOTHUMB_STORE_CONF, NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION },        

        { CONFIG_AUTOTHUMB_STYLE_BGCOLOR, NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION },
        { CONFIG_AUTOTHUMB_STYLE_WATER_IMAGE, NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION },
        { CONFIG_AUTOTHUMB_STYLE_WATER_TEXT, NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION },

		{ NULL, NULL, T_CONFIG_UNSET, T_CONFIG_SCOPE_UNSET }
	};

	if (!p) {
	    return HANDLER_ERROR;
	}

	p->config_storage = calloc(1, srv->config_context->used * sizeof(specific_config *) );
	for (i = 0; i < srv->config_context->used; i++) {
		plugin_config *s;
		s = calloc(1, sizeof(plugin_config));
		s->enable = 1;
		s->file_types = array_init();
		s->file_formats = array_init();        
		s->store_type = buffer_init();
		s->store_conf = buffer_init();        

        s->style_bgcolor = buffer_init();
        s->style_water_image = buffer_init();
        s->style_water_text = buffer_init();

		cv[0].destination = &(s->enable);
		cv[1].destination = s->file_types;
		cv[2].destination = s->file_formats;
		cv[3].destination = s->store_type;
		cv[4].destination = s->store_conf;

        cv[5].destination = s->style_bgcolor;
        cv[6].destination = s->style_water_image;
        cv[7].destination = s->style_water_text;

		p->config_storage[i] = s;
		if (0 != config_insert_values_global(srv, ((data_config *) srv->config_context->data[i])->value, cv)) {
			return HANDLER_ERROR;
		}
	}

	if ( buffer_is_empty(p->config_storage[0]->store_type) || buffer_is_empty(p->config_storage[0]->store_conf) ) {
		return HANDLER_ERROR;
	}

	return HANDLER_GO_ON;
}

handler_t _mod_autothumb_physical_handler(server *srv, connection *con, void *p_d) {
    plugin_data *p = p_d;
	if (con->http_status != 0
        || con->uri.path->used == 0
        || con->physical.path->used == 0
        || con->mode != DIRECT
        || con->file_finished) {
        return HANDLER_GO_ON;
	}

	_mod_autothumb_patch_connection(srv, con, p);
	con->mode = p->id;

	return HANDLER_GO_ON;
}


/** set http header and output it */
void _subrequest_output(server *srv, connection *con, char *file_buff, int64_t file_size, file_info_t *fi) {
    data_string *ds = NULL;
    buffer *mtime;
    buffer *content_type = buffer_init();

    /** Content-Type */
	for (size_t k = 0; k < con->conf.mimetypes->used; k++) {
		ds = (data_string *) con->conf.mimetypes->data[k];
		buffer *type = ds->key;

		if (type->used == 0 || type->used > con->physical.path->used) {
		    continue;
		}

        char *tmp = con->physical.path->ptr + con->physical.path->used - type->used;
		if (0 == strncasecmp(tmp, type->ptr, type->used - 1) ) {
			buffer_copy_string_buffer(content_type, ds->value);
			break;
		}
	}

	if ( NULL == array_get_element(con->response.headers, "Content-Type") ) {
        if ( buffer_is_empty(content_type) ) {
            buffer_copy_string(content_type, "application/octet-stream");            
        }
        
        response_header_overwrite(srv, con,  CONST_STR_LEN("Content-Type"), CONST_BUF_LEN(content_type) ) ;        
    }
    buffer_free(content_type);


    /** Last-Modified */
    if (NULL == (ds = (data_string *) array_get_element(con->request.headers, "If-Modified-Since")) ) {
		mtime = strftime_cache_get(srv, fi->create_timestamp);
	}
	else {
		mtime = ds->value;
	}

    
    //int response_header_append(server *srv, connection *con, const char *key, size_t keylen, const char *value, size_t vallen) {
    response_header_append(srv, con, CONST_STR_LEN("Last-Modified"), CONST_BUF_LEN(mtime) );    

    buffer *expire = buffer_init();
    time_t timestamp = fi->create_timestamp + 864000;
    int lens = strftime(expire->ptr, 32 , "%a, %d %b %Y %H:%M:%S GMT", gmtime(&(timestamp)) );
    expire->used = lens + 1;
    //HTTP/1.0
    response_header_append(srv, con, CONST_STR_LEN("Expires"),  CONST_BUF_LEN(expire));
    //HTTP/1.1
    buffer_copy_string_len(expire, CONST_STR_LEN("max-age="));
    buffer_append_long(expire, timestamp - srv->cur_ts);    
    response_header_append(srv, con, CONST_STR_LEN("Cache-Control"), CONST_BUF_LEN(expire) );
    buffer_free(expire);

	/** ETag,not support etag, use "last-modify + cache-control"*/
	/*
	if (NULL == array_get_element(con->response.headers, "ETag"))
	{
		buffer *file_etag = buffer_init();
		buffer_append_long(file_etag, file_size);
		buffer_append_string_buffer(file_etag, mtime);
		etag_mutate(con->physical.etag, file_etag);
		buffer_free(file_etag);
		response_header_overwrite(srv, con, CONST_STR_LEN("ETag"), CONST_BUF_LEN(con->physical.etag) ) ;
	}
	*/
	//log_error_write(srv, __FILE__, __LINE__, "s:s", "mtime:", mtime->ptr);

    /** step: cachable,If-Range,output content */
    //(server *srv, connection *con, buffer *mtime)
	//if (HANDLER_FINISHED != http_response_handle_cachable(srv, con, mtime) ) {
	if(1) {
        //step:设置If-Range
        //if (con->request.http_range && con->conf.range_requests) {

        //}
        
        //step:output content
        buffer *content = buffer_init();
        buffer_copy_memory(content, file_buff, file_size);
        
        buffer_append_string(content, "\xD9");
        chunkqueue_append_buffer(con->write_queue, content);
        
        buffer_free(content);
    }
}

handler_t _mod_autothumb_subrequest(server *srv, connection *con, void *p_d) {
    plugin_data *p = p_d;

    //fileter some request
    if (con->mode != DIRECT && con->mode != p->id
        || con->http_status != 0
        || con->uri.path->used == 0
        || con->physical.path->used == 0
        || con->file_finished == 1
        || con->request.http_method != HTTP_METHOD_GET) {
        con->mode = DIRECT;
        return HANDLER_GO_ON;
    }


    #ifdef DEBUG_AUTOTHUMB
    static int cnt = 0;
    log_error_write(srv, __FILE__, __LINE__, "s:d", "------------------------------", cnt);
    log_error_write(srv, __FILE__, __LINE__, "s:s", "store_type:", p->conf.store_type->ptr);
    log_error_write(srv, __FILE__, __LINE__, "s:s", "store_conf:", p->conf.store_conf->ptr);
    cnt++;
    if(cnt > 1000000) {
        cnt = 0;
    }
    timer(1);
    #endif

    result_t res;
    handler_t result = HANDLER_FINISHED;

    char *file_data1 = NULL;
    int64_t file_size1 = 0;
    char *file_data2 = NULL;
    int64_t file_size2 = 0;

    char last_filename[128];

    file_info_t *fi = file_info_init();

    //TODO:....
    store_plugin_t *sp = NULL;
    if( buffer_is_equal_string(p->conf.store_type, "fastdfs", 7) ) {
        sp = STORE_INIT(fastdfs, p->conf.store_conf->ptr);
    }
    else if( buffer_is_equal_string(p->conf.store_type, "file", 4) ) {
        sp = STORE_INIT(file, p->conf.store_conf->ptr);
    }

    do {
        //validate params
        if ( 0 == p->conf.enable ) {
            result = HANDLER_GO_ON;
            break;
        }
        #ifdef DEBUG_AUTOTHUMB
        log_error_write(srv, __FILE__, __LINE__, "s", "init sp and fi object.");
        #endif
        if(NULL == sp || NULL == fi) {
            result = HANDLER_ERROR;
            con->http_status = 500;
            break;
        }
        #ifdef DEBUG_AUTOTHUMB
        log_error_write(srv, __FILE__, __LINE__, "s:s:s:s", "parse uri:", con->uri.path->ptr, ",query:", con->uri.query->ptr);
        #endif
        //parse uri params
        parse_uri(con->uri.path->ptr, fi);

        //transfer format to inner
        if( _transfer_format(srv, con, p_d, fi) == RESULT_FAILED ) {
            #ifdef DEBUG_AUTOTHUMB
            log_error_write(srv, __FILE__, __LINE__, "s", "transfer format failed:");
            #endif
            con->http_status = 404;
            break;
        }        

        #ifdef DEBUG_AUTOTHUMB
        log_error_write(srv, __FILE__, __LINE__, "s:s", "file_extension:", fi->file_extension);
        log_error_write(srv, __FILE__, __LINE__, "s:s", "full_filename:", fi->full_filename);
        log_error_write(srv, __FILE__, __LINE__, "s:s", "orig_filename:", fi->orig_filename);
        log_error_write(srv, __FILE__, __LINE__, "s:s", "outer_format:", fi->outer_format);
        log_error_write(srv, __FILE__, __LINE__, "s:s", "inner_format:", fi->inner_format);
        log_error_write(srv, __FILE__, __LINE__, "s:d", "quality:", fi->quality);
        log_error_write(srv, __FILE__, __LINE__, "s:d", "rotate_degree:", fi->rotate_degree);
        log_error_write(srv, __FILE__, __LINE__, "s:s", "suffix:", fi->suffix);        

        log_error_write(srv, __FILE__, __LINE__, "s", "validate request type.");
        #endif
        //validate request according to file suffix and size
        if (NULL == array_get_element(p->conf.file_types, fi->file_extension) ) {                    
            con->http_status = 404;
            break;
        }

        //fetch request file from store
        #ifdef DEBUG_AUTOTHUMB
        log_error_write(srv, __FILE__, __LINE__, "s:s", "download current file:", fi->full_filename);
        #endif
        res = sp->fetch_file(fi->full_filename, &file_data1, &file_size1);        
        if(res == RESULT_SUCCESS) {
            sp->get_file_info(fi->full_filename, fi);
            break;
        }
        else {
            log_error_write(srv, __FILE__, __LINE__, "s:s", "can not download current file:", fi->full_filename);
        }

        //fetch old file 
        #ifdef DEBUG_AUTOTHUMB
        log_error_write(srv, __FILE__, __LINE__, "s:s", "download old file:", fi->orig_filename);
        #endif
        res = sp->fetch_file(fi->orig_filename, &file_data2, &file_size2);
        if(res != RESULT_SUCCESS) {
            log_error_write(srv, __FILE__, __LINE__, "s:s:d", "can not download old file:", fi->orig_filename, file_size2);
            con->http_status = 404;
            break;
        }

        #ifdef DEBUG_AUTOTHUMB
        log_error_write(srv, __FILE__, __LINE__, "s", "auto create new thumbnail.");
        #endif
        //generate thumbnails
        file_data1 = get_transition_image_blob(file_data2, file_size2, &file_size1, fi);
        if(file_data1 == NULL) {
            log_error_write(srv, __FILE__, __LINE__, "s:s", "can not auto create new thumbnail:", fi->full_filename);
            con->http_status = 500;
            break;
        }
        free(file_data2);

        #ifdef DEBUG_AUTOTHUMB
        log_error_write(srv, __FILE__, __LINE__, "s:s", "upload slave file,suffix:", fi->suffix);
        #endif
        //save slave file
        res = sp->save_slave_file(file_data1, file_size1, fi->orig_filename, fi->suffix, fi->file_extension, last_filename);
        if( res != RESULT_SUCCESS) {
            log_error_write(srv, __FILE__, __LINE__, "s:s:s", "can not upload slave file:", fi->orig_filename, fi->suffix);
            con->http_status = 500;
            break;
        }
        #ifdef DEBUG_AUTOTHUMB
        log_error_write(srv, __FILE__, __LINE__, "s:s", "upload slave file ok, last_filename:", last_filename);
        #endif
        sp->get_file_info(last_filename, fi);
    }while(0);

    #ifdef DEBUG_AUTOTHUMB
    log_error_write(srv, __FILE__, __LINE__, "s:d", "output file size:", file_size1);
    #endif
    if(file_size1 > 0) {
        _subrequest_output(srv, con, file_data1, file_size1, fi);
        free(file_data1);
        con->file_finished = 1;
    }
    con->mode = DIRECT;

    /** free resource  */
    file_info_free(fi);
    //TODO:....
    if( buffer_is_equal_string(p->conf.store_type, "fastdfs", 7) ) {
        STORE_FREE(fastdfs, sp);
    }
    else if( buffer_is_equal_string(p->conf.store_type, "file", 4) ) {
        STORE_FREE(file, sp);
    }

    #ifdef DEBUG_AUTOTHUMB
    log_error_write(srv, __FILE__, __LINE__, "s:d", "the elapsed time(us):", timer(0) );
    #endif

    return (handler_t)result;
}

handler_t _mod_autothumb_free(server *srv, void *p_d) {
	plugin_data *p = p_d;

	UNUSED(srv);

	if (!p) return HANDLER_GO_ON;

	if (p->config_storage) {
		size_t i;

		for (i = 0; i < srv->config_context->used; i++) {
			plugin_config *s = p->config_storage[i];
			array_free(s->file_types);
            array_free(s->file_formats);
            buffer_free(s->store_conf);
            buffer_free(s->store_type);

            buffer_free(s->style_bgcolor);
            buffer_free(s->style_water_image);
            buffer_free(s->style_water_text);
			free(s);
		}
		free(p->config_storage);
	}

    free_thumbnail();

	free(p);

	return HANDLER_GO_ON;
}

/** convert formatted string for the internal format */
handler_t _transfer_format(server *srv, connection *con, void *p_d, file_info_t *fi) {
    plugin_data *p = p_d;    
    data_string *ds;
    handler_t result = RESULT_FAILED;

	if( *(fi->outer_format) == '\0' ) {//no suffix format
		result = RESULT_SUCCESS;
	} 
    else if (NULL != (ds = (data_string *) array_get_element(p->conf.file_formats, fi->outer_format)) ) {
        memcpy(fi->inner_format, ds->value->ptr, ds->value->used);
        result = RESULT_SUCCESS;  
    }

    return result;
}

//---------------------lighttpd plugin standard interface-----------------------
INIT_FUNC(mod_autothumb_init) {
	return _mod_autothumb_init();
}

SETDEFAULTS_FUNC(mod_autothumb_set_defaults) {
	return _mod_autothumb_set_defaults(srv, p_d);
}

PHYSICALPATH_FUNC(mod_autothumb_physical_handler) {
    return _mod_autothumb_physical_handler(srv, con, p_d);
}

SUBREQUEST_FUNC(mod_autothumb_subrequest) {
    return _mod_autothumb_subrequest(srv, con, p_d);
}

FREE_FUNC(mod_autothumb_free) {
	return _mod_autothumb_free(srv, p_d);
}

int mod_autothumb_plugin_init(plugin *p) {
    p->version           = LIGHTTPD_VERSION_ID;
	p->name              = buffer_init_string("autothumb");

	p->init              = mod_autothumb_init;
	p->set_defaults      = mod_autothumb_set_defaults;
	p->handle_physical   = mod_autothumb_physical_handler;
	p->handle_subrequest = mod_autothumb_subrequest;
	p->cleanup           = mod_autothumb_free;

	p->data              = NULL;

	return 0;
}







