#include "image_cache.h"
#include <stdio.h>

int main(void) {
	image_cache_init("./cache");
	struct image_data *data = image_cache_get_image("http://s.wee.cat/users/62e03d3d9f1044d0ba52424c1ae19e3e/image.png");
	printf("image data size %li\n", data->size);
	printf("image data etag %s\n", data->etag);
	image_cache_destroy();
	
  return 0;
}
