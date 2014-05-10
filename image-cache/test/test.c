#include "image_cache.h"
#include <stdio.h>
#include <unistd.h> // usleep

static int m_got_response = 0;
#define NUM_RESPONSES 10
#define EXTRA_RESPONSES 6

void on_image_loaded(struct image_data *data) {
    if (data) {
        const char *url = data->url;
        if (!url) {
            url = "(null)";
        }

        if (data->bytes) {
            printf("Image loaded: %s with data bytes provided, and length = %d\n", url, (int)data->size);
        } else {
            printf("Image could not be loaded: %s with (null) data bytes, and length = %d\n", url, (int)data->size);
        }
    } else {
        printf("(null) passed to on_image_loaded callback\n");
    }

    m_got_response++;
}

int main(void) {
    m_got_response = 0;

    // Use test folder for cache
    image_cache_init("test", on_image_loaded);

    image_cache_load("http://unresolvableserverkfsdjghsdkj.com/imageneverloaded.png");
    image_cache_load("https://unresolvableserverkfsdjghsdkj.com/imageneverloaded.png");
    image_cache_load("http://s3.amazonaws.com/theoatmeal-img/comics/columbus_day/1.png");
    image_cache_load("https://s3.amazonaws.com/s.wee.cat/avatars/weeby_avatar_0002.png");
    image_cache_load("http://s3.amazonaws.com/theoatmeal-img/comics/columbus_day/filenotheresfasf.png");
    image_cache_load("https://s3.amazonaws.com/s.wee.cat/avatars/filenotherebvlah.png");

    for (int ii = 0; ii < NUM_RESPONSES; ++ii) {
        char image_url[512];
        sprintf(image_url, "https://graph.facebook.com/%d/picture?type=large", ii + 1000);
        image_cache_load(image_url);
    }

    while (m_got_response < NUM_RESPONSES + EXTRA_RESPONSES) {
        usleep(100);
    }

    image_cache_destroy();

    return 0;
}

