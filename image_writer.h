#ifndef IMAGE_WRITER_H
#define IMAGE_WRITER_H

#ifdef UNITY
#include "Unity/png.h"
#else
#include "core/deps/png/png.h"
#include "core/deps/png/pngstruct.h"
#endif

#include "core/log.h"
#include "core/types.h"

enum IMAGE_TYPES {IMAGE_TYPE_JPEG, IMAGE_TYPE_PNG};

#ifdef __cplusplus
extern "C" {
#endif

bool write_image_to_file(const char *path, const char *name, unsigned char * data, int width, int height, int channels); 
bool write_png_to_file(const char *path, const char *name, unsigned char * data, int width, int height, int channels);
bool write_jpeg_to_file(const char *path, const char *name, unsigned char * data, int width, int height, int channels); 

char *write_image_to_base64(const char *image_type, unsigned char *data, int width, int height, int channels); 
char *write_png_to_base64(unsigned char * data, int width, int height, int channels);
char *write_jpeg_to_base64(unsigned char * data, int width, int height, int channels); 

#ifdef __cplusplus
}
#endif

#endif
