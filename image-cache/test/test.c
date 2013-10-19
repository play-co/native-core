#include "image_cache.h"
#include <stdio.h>

static int m_got_response = 0;

void on_image_loaded(struct image_data *data) {
	if (data) {
		const char *url = data->url;
		if (!url) {
			url = "(null)";
		}

		if (data->bytes) {
			printf("Image loaded: %s with data bytes provided, and length = %d\n", url, (int)data->size);
		} else {
			printf("Image loaded: %s with (null) data bytes, and length = %d\n", url, (int)data->size);
		}
	} else {
		printf("(null) passed to on_image_loaded callback\n");
	}

	m_got_response = 1;
}

int main(void) {
	m_got_response = 0;

	image_cache_init("test", on_image_loaded);

	image_cache_load("http://s.wee.cat/users/62e03d3d9f1044d0ba52424c1ae19e3e/image.png");

	while (!m_got_response) {
		usleep(1000);
	}

	image_cache_destroy();
	
	return 0;
}

