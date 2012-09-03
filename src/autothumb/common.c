#include "common.h"

file_info_t *file_info_init() {
    file_info_t *fi;
    fi = (file_info_t*)calloc(1, sizeof(file_info_t));

    fi->rotate_degree = 0;
    fi->quality = 0;

    fi->file_extension = (char*)calloc(FI_FILE_EXTENSION_SIZE, sizeof(char) );
    fi->full_filename = (char*)calloc(FI_FULL_FILENAME_SIZE, sizeof(char) );
    fi->orig_filename = (char*)calloc(FI_ORIG_FILENAME_SIZE, sizeof(char) );
    fi->outer_format = (char*)calloc(FI_OUTER_FORMAT_SIZE, sizeof(char) );
    fi->inner_format = (char*)calloc(FI_INNER_FORMAT_SIZE, sizeof(char) );
    fi->suffix = (char*)calloc(FI_SUFFIX_SIZE, sizeof(char) );          

    return fi;
}

void file_info_free(file_info_t *fi) {
    if (!fi) return;
    free(fi->file_extension);
    free(fi->full_filename);
    free(fi->orig_filename);
    free(fi->outer_format);
    free(fi->inner_format);
    free(fi->suffix);
    free(fi);
}

store_plugin_t *store_plugin_init() {
    store_plugin_t *sp;
    sp = (store_plugin_t*)calloc(1, sizeof(store_plugin_t));

    sp->version = 0;
    sp->name = (char*)calloc(32, sizeof(char) );
    sp->data = NULL;

    return sp;
}

void store_plugin_free(store_plugin_t *sp) {
    if (!sp) return;
    free(sp->name);
    free(sp);
}


int parse_uri(char *uri, file_info_t *fi) {

    char *p1;    
    char arr1[MAX_SUFFIX_SIZE],arr2[MAX_SUFFIX_SIZE];    

    if (*uri == '/') {
        uri++;
    }

    /** full filename */
    p1 = strrchr(uri, '.');
    if(p1 != NULL) {
        strncpy(fi->full_filename, uri, p1 - uri);
    }
    else {
        return -1;
    }

    /** original filename, original format string*/
    p1 = strchr(fi->full_filename, '=');

    if(p1 == NULL) {//not resize,standard output        
        *(fi->outer_format) = '\0';
        *(fi->suffix) = '\0';
        strcpy(fi->orig_filename, fi->full_filename);
    }
    else {//fetch format params
        strncpy(fi->orig_filename, fi->full_filename, p1 - fi->full_filename);
        p1++;
        if(strlen(p1) > MAX_SUFFIX_SIZE) {
            return -2;
        }
        strcpy(arr1, p1);

        /** split out format */
        //TODO
        p1 = strchr(arr1, '_');
        if(p1 == NULL) {
            strcpy(fi->outer_format, arr1);
        }
        else {
            strncpy(fi->outer_format, arr1, p1 - arr1);

            p1 = strrchr(arr1, 'q');
            if(p1 != NULL) {
                p1++;                
                strncpy(arr2, p1, 1);                
                fi->quality = atoi(arr2);           
            }
            p1 = strrchr(arr1, 'r');
            if(p1 != NULL) {
                p1++;
                strncpy(arr2, p1, 2);                
                fi->rotate_degree = atoi(arr2);           
            }            
        }
    }


    /** file extension   */
    p1 = strrchr(uri, '.');
    p1++;
    strcpy(fi->file_extension, p1);
    
    /** the file extension for the unique identifier of the file */
    if( *(fi->outer_format) != '\0' ) {
        *arr1 = '\0';
        *arr2 = '\0';        

        if(fi->quality > 0) {
            sprintf(arr1, "_q%d", fi->quality);
            fi->quality *= 10;
        }

        if(fi->rotate_degree > 0) {
            sprintf(arr2, "_r%d", fi->rotate_degree);
            fi->rotate_degree *= 10;
        }

        sprintf(fi->suffix, "_%s%s%s", fi->outer_format, arr1, arr2);
    }

    //force to lower char:TODO
    //fi->file_extension = tolower(fi->file_extension);
    //fi->outer_format = tolower(fi->outer_format);

    /** TODO:can actually use a solution, but fastdfs only 16 length */
    /*
    if( *(fi->suffix) == '\0' ) {
        sprintf(fi->full_filename, "%s.%s",fi->orig_filename, fi->file_extension);
    }
    else {
        sprintf(fi->full_filename, "%s%s.%s",fi->orig_filename, fi->suffix, fi->file_extension);
    }
    */
    sprintf(fi->full_filename, "%s%s.%s",fi->orig_filename, fi->suffix, fi->file_extension);
    sprintf(fi->orig_filename, "%s.%s",fi->orig_filename, fi->file_extension);   

    return 0;
}



int parse_uri_params(char *url, key_value_pair_t *params, const int max_count) {
	key_value_pair_t *pCurrent;
	key_value_pair_t *pEnd;
	char *pParamStart;
	char *p;
	char *pStrEnd;
	int value_len;

	pParamStart = strchr(url, '?');
	if (pParamStart == NULL) {
		return 0;
	}

	*pParamStart = '\0';

	pEnd = params + max_count;
	pCurrent = params;
	p = pParamStart + 1;
	while (p != NULL && *p != '\0') {
		if (pCurrent >= pEnd) {
			return pCurrent - params;
		}

		pCurrent->key = p;
		pStrEnd = strchr(p, '&');
		if (pStrEnd == NULL) {
			p = NULL;
		} else {
			*pStrEnd = '\0';
			p = pStrEnd + 1;
		}

		pStrEnd = strchr(pCurrent->key, '=');
		if (pStrEnd == NULL) {
			continue;
		}

		*pStrEnd = '\0';
		pCurrent->value = pStrEnd + 1;
		if (*pCurrent->key == '\0') {
			continue;
		}

        //TODO:urldecode还是依赖fastdfs，哎
		urldecode(pCurrent->value, strlen(pCurrent->value), pCurrent->value, &value_len);
		pCurrent++;
	}

	return pCurrent - params;
}


char *get_uri_param(const char *param_name, key_value_pair_t *params, const int param_count) {
	key_value_pair_t *pCurrent;
	key_value_pair_t *pEnd;

	pEnd = params + param_count;
	for (pCurrent = params; pCurrent < pEnd; pCurrent++) {
		if (strcmp(pCurrent->key, param_name) == 0) {
			return pCurrent->value;
		}
	}

	return NULL;
}


/*
int main(int argc, char **argv) {    
    
    char uri[128];
    //strcpy(uri, "group3/M00/00/71/toMRCk8K-nwIAAAAAAIjt5FB4t4AAAAFgCdh6MAAiPP422.jpg");
    //strcpy(uri, "group1/M00/01/98/wKgUy08OWUS01YlEAACCF4QfuD8488+100x100gb.jpg");
    strcpy(uri, "group1/M00/00/00/wKgjBk-OZkf3sBi4AAHbKkdVTuo444=q800x600_qaa_r_30.jpg");
    strcpy(uri, "group1/M00/00/00/wKgjBk-WS_udiMw7AAHbKkdVTuo111=c_q30.jpg");
    printf("URI:%s\n", uri);    

    file_info_t *fi = file_info_init();
    parse_uri(uri, fi);

    printf("file_extension:%s\n", fi->file_extension);
    printf("full_filename:%s\n", fi->full_filename);
    printf("orig_filename:%s\n", fi->orig_filename);
    printf("outer_format:%s\n", fi->outer_format);
    printf("inner_format:%s\n", fi->inner_format);
    printf("quality:%d\n", fi->quality);
    printf("rotate_degree:%d\n", fi->rotate_degree);
    printf("suffix:%s, len:%d\n", fi->suffix, strlen(fi->suffix));    
    
    return 0;
}
*/









