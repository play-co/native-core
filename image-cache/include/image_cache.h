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
	UT_hash_handle hhr;
};


// The callback provided to this function is when a url provided to image_cache_load has finished loading
// It is called as soon as possible if the image is cached on disk and called a second time if the cache 
// discovers a newer image on the server.
#if __cplusplus
extern "C" {
#endif

void image_cache_init(const char *path, void (*load_callback)(struct image_data*));
void image_cache_destroy();
void image_cache_remove(const char *url);
void image_cache_load(const char *url);

#if __cplusplus
} //extern C
#endif
#endif //IMAGE_CACHE_H_
