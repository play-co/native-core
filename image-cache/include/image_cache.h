#ifndef IMAGE_CACHE_H_
#define IMAGE_CACHE_H_

#include "uthash/uthash.h"

struct image_data {
	char *bytes;
	size_t size;
	char *url;
};

struct etag_data {
	char *url;
	char *etag;
	UT_hash_handle hh;
};


void image_cache_init(const char *path, void (*load_callback)(struct image_data*));
void image_cache_destroy();
struct image_data *image_cache_get_image(const char *url);
void image_cache_load(const char *url);

#endif //IMAGE_CACHE_H_
