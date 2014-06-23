/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the Mozilla Public License v. 2.0 as published by Mozilla.
 
 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Mozilla Public License v. 2.0 for more details.
 
 * You should have received a copy of the Mozilla Public License v. 2.0
 * along with the Game Closure SDK.  If not, see <http://mozilla.org/MPL/2.0/>.
 */

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

typedef void (*image_cache_cb)(struct image_data *);

// The callback provided to this function is when a url provided to image_cache_load has finished loading
// It is called as soon as possible if the image is cached on disk and called a second time if the cache 
// discovers a newer image on the server.
#if __cplusplus
extern "C" {
#endif

void image_cache_init(const char *path, image_cache_cb);
void image_cache_destroy();
void image_cache_remove(const char *url);
void image_cache_load(const char *url);

#if __cplusplus
} //extern C
#endif

#endif //IMAGE_CACHE_H_

