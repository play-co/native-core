#ifndef IMAGE_CACHE_H_
#define IMAGE_CACHE_H_

#include "uthash/uthash.h"

struct image_data {
	char *bytes;
	size_t size;
	char *etag;
	char *url;
	UT_hash_handle hh;
};


void image_cache_init(const char *path);
void image_cache_destroy();
struct image_data *image_cache_get_image(const char *url);

#endif //IMAGE_CACHE_H_
