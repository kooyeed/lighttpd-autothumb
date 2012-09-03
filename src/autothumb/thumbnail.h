#ifndef THUMBNAIL_H_INCLUDED
#define THUMBNAIL_H_INCLUDED

#include "common.h"
#include "../base.h"

/** convert file data (images stored in the file) */
unsigned char * get_transition_image(char *full_filename, int64_t *thumbnail_size, file_info_t *fi);

/** conversion-memory data (the images are stored in memory) */
unsigned char *get_transition_image_blob(char *file_buf, int buf_size, int64_t *thumbnail_size, file_info_t *fi);

/** free all resource */
void free_thumbnail();

void init_thumbnail();

#endif // THUMBNAIL_H_INCLUDED
