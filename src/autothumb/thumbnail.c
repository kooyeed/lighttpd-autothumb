#include "thumbnail.h"
#include <wand/MagickWand.h>

#if 1
#define REPLACE_CHAR_COUNT  5 /** the number of characters that need to be replaced */
#define CROP_ITEM_COUNT     5 /** ?? */

#define IMAGE_FOR_MAX(X, Y)  ((X) > ( Y ) ? (X):(Y))
#define IMAGE_FOR_MIN(X, Y)  ((X) < ( Y ) ? (X):(Y))

#define ThrowWandException(wand) \
{ \
  char *description; \
 \
  ExceptionType severity; \
 \
  description = MagickGetException(wand,&severity); \
  printf("%s %s\n",GetMagickModule(),description); \
  description = (char *) MagickRelinquishMemory(description); \
}

unsigned char *covert_image(MagickWand *magick_wand, file_info_t *fi, int64_t *thumbnail_size);
int get_Crop_width_height(char *inner_format, long int* cw, long int *ch, size_t *x_offset, size_t *y_offset);
static size_t get_pixels_by_rate(size_t a, size_t b, size_t c);
static void get_Crop_offset_and_wh(long unsigned int cw, long unsigned int ch, long unsigned int *width, long unsigned int *height, size_t *x_offset, size_t *y_offset);

/**
* based on the conversion format specifier for cutting the designated area of 
* the parameter, such as 200x100 to +100
* the imagemagick Internal does not support this protocol
*/
int get_Crop_width_height(char *inner_format, long int* cw, long int *ch, size_t *x_offset, size_t *y_offset) {
	if (inner_format == NULL || cw == NULL || ch == NULL || x_offset == NULL || y_offset == NULL) {
	    return 0;
	}

	int j;
	*x_offset = 0;
	*y_offset = 0;
	*ch = 0;
	*cw = 0;
	char *str = strdup(inner_format);
	char delim[] = "x+ ";
	char *tmp = NULL;
	char *saveptr = NULL;
	char *tokens[CROP_ITEM_COUNT];
	char *token;
	for (j = 0, tmp = str;; j++, tmp = NULL) {
		token = strtok_r(tmp, delim, &saveptr);
		if (token == NULL || j >= CROP_ITEM_COUNT) {
		    break;
		}

		tokens[j] = token;
	}
	if (j < 2 || j > 4) {
		free(str);
		return 0;
	}
	*cw = (size_t) atol(tokens[0]);
	*ch = (size_t) atol(tokens[1]);
	if (j == 3) {
		*x_offset = (size_t) atol(tokens[2]);
	}
	else if (j >= 4) {
		*x_offset = (size_t) atol(tokens[2]);
		*y_offset = (size_t) atol(tokens[3]);
	}
	free(str);
	return 1;
}

/** data conversion of a unit  */
static size_t get_pixels_by_rate(size_t a, size_t b, size_t c) {
	if (c == 0) {
	    return 0;
	}
	if (a * b % c) {
		return a * b / c + 1;
	}

	return a * b / c;
}

/** get the real cutting parameters according to the format string */
static void get_Crop_offset_and_wh(long unsigned int cw, long unsigned int ch, long unsigned int *width, long unsigned int *height, size_t *x_offset, size_t *y_offset) {
	long unsigned int old_width = *width;
	long unsigned int old_height = *height;
	long unsigned int x = *x_offset;
	long unsigned int y = *y_offset;
	if (cw * old_height >= ch * old_width) {
		*width = cw;
		*x_offset = 0;
		*height = get_pixels_by_rate(cw, old_height, old_width);
		if (0 == x && 0 == y) {
			*y_offset = 0;
		} else if (0 == x) {
			*y_offset = *height * (100 - y) / 100;
			*y_offset = *y_offset - IMAGE_FOR_MIN(*y_offset, ch);
		} else if (0 == y) {
			*y_offset = *height * x / 100;
			*y_offset = IMAGE_FOR_MIN(*y_offset, (*height - ch));
		} else {
			*y_offset = (*height - ch) * x / (x + y);
		}

	} else {
		*height = ch;
		*y_offset = 0;

		*width = get_pixels_by_rate(ch, old_width, old_height);
		if (0 == x && 0 == y) {
			*x_offset = 0;
		} else if (0 == x) {
			*x_offset = *width * (100 - y) / 100;
			*x_offset = *x_offset - IMAGE_FOR_MIN(*x_offset, cw);
		} else if (0 == y) {
			*x_offset = *width * x / 100;
			*x_offset = IMAGE_FOR_MIN(*x_offset, (*width - cw));
		} else {
			*x_offset = (*width - cw) * x / (x + y);
		}

	}

}

/** convert images */
unsigned char *covert_image(MagickWand *magick_wand, file_info_t *fi, int64_t *thumbnail_size) {    
    //log_error_write(srv, __FILE__, __LINE__, "s:s", "[image]inner_format:", fi->inner_format);

	unsigned char *image_data = NULL;
    MagickBooleanType status;
	MagickWand *tmp_magick_wand = NULL;
	PixelWand *background = NULL;
	long unsigned int height = MagickGetImageHeight(magick_wand);
	long unsigned int old_height = height;
	long unsigned int width = MagickGetImageWidth(magick_wand);
	long unsigned int old_width = width;
	int is_crop = 0;
	int is_Crop = 0;
	int is_thumbnail = 0;
	long int i = 0, j = 0;
	char is_gif_flag = 0;
	char is_jpeg_flag = 0;
	int do_quality = 0;
	char *fileformat = NULL;
	size_t cw = 0; //crop weight
	size_t ch = 0; //crop height

	size_t x_offset = 0;
	size_t y_offset = 0;

    status = MagickFalse;

	if(NULL == (fileformat = MagickGetImageFormat(magick_wand)) ) {
		return NULL;
	}

    /** TODO
	if (0 == strcasecmp(fileformat, image_format[GIFEXT])) {
		is_gif_flag = 1;
	} else if (0 == strcasecmp(fileformat, image_format[JPEGEXT]) || 0 == strcasecmp(fileformat, image_format[JPGEXT]) ) {
		is_jpeg_flag = 1;
	}
    */
	fileformat = (char *) MagickRelinquishMemory(fileformat);

	if ('c' == fi->inner_format[0]) {
		is_crop = 1;
	} else if ('C' == fi->inner_format[0]) {
		is_Crop = 1;
	} else {
		is_thumbnail = 1;
	}

	if (is_crop) {        
		ParseMetaGeometry(fi->inner_format + 1, &i, &j, &width, &height);
	} else if (is_thumbnail) {
	    char fmt[32];
	    strcpy(fmt, fi->inner_format);
		ParseMetaGeometry(fmt, &i, &j, &width, &height);
		if (old_width == width && height == old_height) {
		    is_thumbnail = 0;
		}
	} else if (is_Crop) {
		if (0 >= get_Crop_width_height(fi->inner_format + 1, (long int*)&cw, (long int*)&ch, &x_offset, &y_offset) ) {
            printf("%s%d:Crop  %s error\n", __FILE__, __LINE__, fi->inner_format + 1 );

			return NULL;
		}
        #if 0
        		if(cw > width || ch > height) {
        			image_data = MagickGetImagesBlob(magick_wand, thumbnail_size);
        			magick_wand = DestroyMagickWand(magick_wand);
        			return image_data;
        		}
        #endif
		get_Crop_offset_and_wh(cw, ch, &width, &height, &x_offset, &y_offset);
	}

	if ( old_width == width && height == old_height && (is_Crop == 0) && (fi->rotate_degree == 0) && (fi->quality == 0) ) {
		image_data = MagickGetImagesBlob(magick_wand, (size_t *)thumbnail_size);
	} else if (width <= 0 || height <= 0) {
		printf("%s%d:Geometry  %s error\n", __FILE__, __LINE__, fi->inner_format );
	} else {
		if (is_gif_flag) {
			tmp_magick_wand = magick_wand;
			magick_wand = MagickCoalesceImages(tmp_magick_wand);
			tmp_magick_wand = magick_wand;
			magick_wand = MagickOptimizeImageLayers(tmp_magick_wand);
			tmp_magick_wand = DestroyMagickWand(tmp_magick_wand);
		}

        //TODO
		if ((old_width < 800) && (old_height < 600) && is_jpeg_flag && is_crop != 1 && (fi->quality == 0)) {
			do_quality = 1;
		}
		background = NewPixelWand();
		status = PixelSetColor(background, "#000000");

		MagickResetIterator(magick_wand);
		while (MagickNextImage(magick_wand) != MagickFalse) {
			if (do_quality) {
				MagickSetImageCompressionQuality(magick_wand, 100);
				MagickStripImage(magick_wand);
			}
			if (is_thumbnail == 1) {                
				MagickThumbnailImage(magick_wand, width, height);
			} else if (is_crop == 1) {
				MagickCropImage(magick_wand, width, height, i, j);
			} else if (is_Crop == 1) {
				MagickThumbnailImage(magick_wand, width, height);
				MagickCropImage(magick_wand, cw, ch, x_offset, y_offset);
				if(is_gif_flag){
				    // gif should thumbnail again
					MagickThumbnailImage(magick_wand, cw, ch);
				}
			}

			if (fi->rotate_degree > 0) {
				MagickRotateImage(magick_wand, background, (double) (fi->rotate_degree) );
			}
			if (fi->quality > 0) {
				MagickSetImageCompressionQuality(magick_wand, fi->quality);
			}
			MagickStripImage(magick_wand);
		}

		background = DestroyPixelWand(background);
		image_data = MagickGetImagesBlob(magick_wand, (size_t *)thumbnail_size);

		if (is_gif_flag) {
			magick_wand = DestroyMagickWand(magick_wand);
		}
	}

	return image_data;
}

/** convert file data (images stored in the file) */
unsigned char * get_transition_image(char *full_filename, int64_t * thumbnail_size, file_info_t *fi) {
	unsigned char *image_data = NULL;

	if (full_filename == NULL ||fi == NULL) {
	    return NULL;
	}
	if ((0 == fi->rotate_degree) && ('\0' == fi->inner_format[0]) && (0 == fi->quality) ) {
        return NULL;
	}

	MagickBooleanType status;
	MagickWand *magick_wand = NULL;
	magick_wand = NewMagickWand();
	status = MagickReadImage(magick_wand, full_filename);
	if (status == MagickFalse) {
		ThrowWandException(magick_wand);
		return NULL;
	}
	image_data = covert_image(magick_wand, fi, thumbnail_size);
	magick_wand = DestroyMagickWand(magick_wand);
	return image_data;
}

/** conversion-memory data (the images are stored in memory) */
unsigned char *get_transition_image_blob(char *file_buf, int buf_size, int64_t *thumbnail_size, file_info_t *fi) {
	unsigned char *image_data = NULL;

	if (file_buf == NULL || fi == NULL || ( 0 == fi->rotate_degree && '\0' == fi->inner_format[0] && 0 == fi->quality) ) {
	    return NULL;
	}

	MagickBooleanType status;
	MagickWand *magick_wand = NULL;
	magick_wand = NewMagickWand();
	status = MagickReadImageBlob(magick_wand, file_buf, buf_size);
	if (status == MagickFalse) {
		ThrowWandException(magick_wand);
		return NULL;
	}

	image_data = covert_image(magick_wand, fi, thumbnail_size);
	magick_wand = DestroyMagickWand(magick_wand);
	return image_data;

}

void init_thumbnail() {
    MagickWandGenesis();
}

void free_thumbnail() {
    MagickWandTerminus();
}

#if 0
int main(int argc, char **argv) {    
    size_t len = 0;
    unsigned char *data = NULL;
    char filename[50];
    //strcpy(filename, argv[1]);
    //strcpy(filename, "test=c300x300+275.jpg");
    strcpy(filename, "/home/tony/app/fastdfs-3.0.6/data2/mm.jpg");

    printf("\n\nstart :%s \n", filename);
    

    MagickWandGenesis();

    FILE *fp;
    char *file_buff;
    int buf_size;
    int64_t thumbnail_size;
    file_info_t *fi;

    if ((fp = fopen(filename, "rb")) != NULL) {
        if(0 == fseek(fp, 0, SEEK_END) ) {
            buf_size = ftell(fp);
        }
        else {
            printf("ERROR.");
            return 0;
        }
        rewind(fp);        
        file_buff = (char*)malloc(sizeof(char) * buf_size ); 
        fread(file_buff, 1, buf_size, fp);
        fclose(fp);
    }
    char *file_buff2 = (char*)malloc(sizeof(char) * buf_size ); 
    fi = file_info_init();
    strcpy(fi->file_extension, "jpg");
    strcpy(fi->full_filename, "mm_a1.jpg");
    strcpy(fi->orig_filename, "mm.jpg");
    strcpy(fi->outer_format, "a1");
    strcpy(fi->inner_format, "101x100");
    strcpy(fi->suffix, "_a1");
    file_buff2 = get_transition_image_blob(file_buff, buf_size, &thumbnail_size, fi);

    printf("thumbnail_size:[%d].\n", thumbnail_size);

    fp = fopen("/home/tony/app/fastdfs-3.0.6/data2/bxx2.jpg", "wb");
    fwrite(file_buff2, thumbnail_size, 1, fp);
    fclose(fp);

    printf("make ok.\n");

    MagickWandTerminus();

    printf("OK.\n\n");

    return 0;
}
#endif

#endif
//==============================================================================
    
//==============================================================================
#if 0
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <magick/api.h>
/** 由于gm的wand版本太低，我们需要自己实现一些方法，暂时考虑放弃。 */
uint convert_image_by_gm(char *src_data, size_t src_size, unsigned char **dest_data, size_t *dest_size, file_info_t *fi) {
    int result = 0;
    size_t status;
    //MagickWand *wand;
    //MagickWand *tmp_wand = NULL;
    char *fileformat = NULL;
    size_t height = 0,old_height,width = 0,old_width;
    //PixelWand *background = NULL;
    ssize_t i = 0, j = 0;

    ExceptionInfo exception;
    Image *image;
    ImageInfo *image_info;
 

    //step:init
    InitializeMagick("tony");
    GetExceptionInfo(&exception);

    do {
        //step:load data
        image_info = CloneImageInfo((ImageInfo *) NULL);                        
        image_info->blob = (void *) src_data;
        image_info->length = src_size;

        image = ReadImage(image_info, &exception);
        if (NULL == image ) {
            result = 1;
            break;
        }
        

        //if(MagickFalse == IsGeometry(fi->inner_format)) {
        //    result = 2;
        //    break;
        //}

        //step:validate file type
        fileformat = image->magick;
        height = image->rows;
        width = image->columns;
        old_width = width;
        old_height = height;

        ParseGeometry(fi->inner_format, &i, &j, &width, &height);

        

        
        if (old_width != width || height != old_height || fi->quality != 0 || fi->rotate_degree != 0) {
            if(fileformat == "GIF") {
                tmp_wand = wand;
                wand = MagickCoalesceImages(tmp_wand);
                tmp_wand = wand;
                wand = MagickOptimizeImageLayers(tmp_wand);
                tmp_wand = DestroyMagickWand(tmp_wand);
            }

            if (fi->rotate_degree > 0) {
                background = NewPixelWand();
                status = PixelSetColor(background, "#000000");
            }

            MagickResetIterator(wand);
            while (MagickNextImage(wand) != MagickFalse) {
                if (fi->convert_type == 1) {//resize          
                    MagickThumbnailImage(wand, width, height);
                } else if (fi->convert_type == 2) {//crop,not support gif                    
                    MagickCropImage(wand, width, height, i, j);
                }

                if (fi->rotate_degree > 0) {
                    MagickRotateImage(wand, background, (double) (fi->rotate_degree) );
                }
                if (fi->quality > 0) {
                    MagickSetImageCompressionQuality(wand, fi->quality);
                }
                MagickStripImage(wand);
            }

            if (fi->rotate_degree > 0) {
                background = DestroyPixelWand(background);                    
            }
        }    
        *dest_data = MagickGetImagesBlob(wand, (size_t *)dest_size);

       

        
    } while(0);

    //step:free 
    DestroyImageInfo(image_info);
    DestroyExceptionInfo(&exception);
    DestroyMagick();

    return (uint)result;
}
#endif

#if 0
uint convert_image_by_magickwand(char *src_data, size_t src_size, unsigned char **dest_data, size_t *dest_size, file_info_t *fi) {
    int result = 0;
    MagickBooleanType status;
    MagickWand *wand;
    MagickWand *tmp_wand = NULL;
    char *fileformat = NULL;
    size_t height = 0,old_height,width = 0,old_width;
    PixelWand *background = NULL;
    ssize_t i = 0, j = 0;
 

    do {
        //step:init
        MagickWandGenesis();

        //step:load data
        wand = NewMagickWand();        
        if (MagickFalse == (status = MagickReadImageBlob(wand, src_data, src_size)) ) {
            result = 1;
            break;
        }

        if(MagickFalse == IsGeometry(fi->inner_format)) {
            result = 2;
            break;
        }

        //step:validate file type
        if(MagickFalse == (fileformat = MagickGetImageFormat(wand)) ) {            
            result = 3;
            break;
        }                
        height = MagickGetImageHeight(wand);
        width = MagickGetImageWidth(wand);        
        old_width = width;
        old_height = height;

        ParseMetaGeometry(fi->inner_format, &i, &j, &width, &height);
        if (old_width != width || height != old_height || fi->quality != 0 || fi->rotate_degree != 0) {
            if(fileformat == "GIF") {
                tmp_wand = wand;
                wand = MagickCoalesceImages(tmp_wand);
                tmp_wand = wand;
                wand = MagickOptimizeImageLayers(tmp_wand);
                tmp_wand = DestroyMagickWand(tmp_wand);
            }

            if (fi->rotate_degree > 0) {
                background = NewPixelWand();
                status = PixelSetColor(background, "#000000");
            }

            MagickResetIterator(wand);
            while (MagickNextImage(wand) != MagickFalse) {
                if (fi->convert_type == 1) {//resize          
                    MagickThumbnailImage(wand, width, height);
                } else if (fi->convert_type == 2) {//crop,not support gif                    
                    MagickCropImage(wand, width, height, i, j);
                }

                if (fi->rotate_degree > 0) {
                    MagickRotateImage(wand, background, (double) (fi->rotate_degree) );
                }
                if (fi->quality > 0) {
                    MagickSetImageCompressionQuality(wand, fi->quality);
                }
                MagickStripImage(wand);
            }

            if (fi->rotate_degree > 0) {
                background = DestroyPixelWand(background);                    
            }
        }    
        *dest_data = MagickGetImagesBlob(wand, (size_t *)dest_size);

        //step:free 
        wand = DestroyMagickWand(wand);
        MagickWandTerminus();
    } while(0);

    return (uint)result;
}
#endif

#if 0
int main() {
    unsigned char *src_data, *dest_data;
    size_t src_size,dest_size;
    file_info_t *fi;

    fi = file_info_init();
    strcpy(fi->file_extension, "gif");
    strcpy(fi->full_filename, "mm.jpg");
    strcpy(fi->orig_filename, "mm.jpg");
    strcpy(fi->outer_format, "a1");
    strcpy(fi->inner_format, "100x100");
    strcpy(fi->suffix, "_a1");
    fi->convert_type = 2;
    fi->rotate_degree = 0;
    fi->quality = 0;


    FILE *fp;

    //if ((fp = fopen("/home/tony/app/fastdfs-3.0.6/data2/test3.gif", "rb")) != NULL) {
    if ((fp = fopen("/home/tony/app/fastdfs-3.0.6/data2/mm.jpg", "rb")) != NULL) {
        if(0 == fseek(fp, 0, SEEK_END) ) {
            src_size = ftell(fp);
        }
        else {
            printf("ERROR.");
            return 0;
        }
        rewind(fp);        
        src_data = (char*)malloc(sizeof(char) * src_size ); 
        (void)fread(src_data, 1, src_size, fp);
        fclose(fp);
    }
    uint res = convert_image_by_magickwand(src_data, src_size, &dest_data, &dest_size, fi);    
    //uint res = convert_image_by_gm(src_data, src_size, &dest_data, &dest_size, fi);        
    printf("res:[%d], dest_size:[%d].\n", res, dest_size);    



    fp = fopen("/home/tony/app/fastdfs-3.0.6/data2/bxx.jpg", "wb");
    fwrite(dest_data, dest_size, 1, fp);

    free(dest_data);    
    file_info_free(fi);

    printf("OVER.\n");


    
    return 0;
}
#endif

