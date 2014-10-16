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

#include "core/texture_manager.h"
#include "core/texture_2d.h"
#include "core/deps/uthash/uthash.h"
#include "core/core.h"
#include "core/log.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "core/image-cache/include/image_cache.h"
#include "core/config.h"
#include "platform/resource_loader.h"
#include "core/list.h"
#include "platform/gl.h"
#include "core/events.h"
#include "platform/native.h"
#include "core/deps/jansson/jansson.h"
#include "core/platform/threads.h"

// This mode allows the texture manager to opportunistically free canvases
// if they have not been accessed lately
#define FREE_CANVASES

// This mode notifies immediately on losing the canvas, otherwise it notifies
// when the canvas is used
//#define FREE_CANVASES_NOTIFY_IMMEDIATELY

// 1024 x 1024 with 4 channels
// this is not necassarily the size of every sheet
#define MAX_SHEET_BYTES (1024*1024*4)
#define DEFAULT_SHEET_DIMENSION 64

// Constants for Android phones and starting-points for iOS
#define MAX_BYTES_FOR_HALFSIZED_TEXTURES 500000000 	/* 30 MB */
#define MAX_BYTES_FOR_FULLSIZED_TEXTURES 500000000	/* 100 MB */

// Does not use fullsized textures under this memory value
#define MIN_BYTES_FOR_FULLSIZE_TEXTURES 30000000	/* 30 MB */

// Does not ratchet below this value
#define MIN_BYTES_FOR_TEXTURES 15000000             /* 15 MB */

// Amount to drop below the high water mark for recent frames in reaction to a low memory warning
// NOTE: This is tuned for the iPad 1 where this sort of issue actually happens consistently
#define LOW_MEM_DROP_RATE 10000000					/* 10 MB */

#define CONTACTPHOTO_URL_PREFIX "@CONTACTPICTURE"
#define CONTACTPHOTO_URL_PREFIX_LEN strlen("@CONTACTPICTURE")
#define DEFAULT_CONTACTPHOTO_SIZE 64
#define DEFAULT_REMOTE_RESOURCE_SIZE 64

// Global halfsized textures flag
int use_halfsized_textures = 0;

static bool m_running = false; // Flag indicating that the background texture loader thread should continue
static texture_manager *m_instance = NULL;
static bool m_instance_ready = false; // Flag indicating that the instance is ready
static bool m_memory_warning = false; // Flag indicating that a memory warning occurred

static ThreadsThread m_load_thread = THREADS_INVALID_THREAD;
static pthread_mutex_t mutex     = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_var   = PTHREAD_COND_INITIALIZER;

static texture_2d *tex_load_list = NULL;
static json_t *spritesheet_map_root = NULL;
static json_t *fontsheet_map_root = NULL;
static int m_frame_epoch = 1;
static long m_frame_used_bytes = 0;

#define EPOCH_USED_BINS 64 /* must be power of two */
#define EPOCH_USED_MASK (EPOCH_USED_BINS - 1)
static long m_epoch_used[EPOCH_USED_BINS] = {0};

// TODO: Optimize the mutex lock holding times

//#define TEXMAN_VERBOSE
//#define TEXMAN_EXTRA_VERBOSE
#if defined(TEXMAN_VERBOSE)
#define TEXLOG(fmt, ...) LOG("{tex} " fmt, ##__VA_ARGS__)
#else
#define TEXLOG(fmt, ...)
#endif

bool is_remote_resource(const char *url) {
    //TODO: until ios implements simulate and stores images from http
    //on disk, need this temporary ifdef
    bool is_remote = false;
#ifndef ANDROID
    is_remote = config_get_remote_loading();
#endif

    if (!url || strlen(url) == 0) {
        is_remote = false;
    } else if (*url == '@') {
        is_remote = true;
    } else if (!strncmp("http", url, 4) || !strncmp("//", url, 2)) {
        is_remote = true;
    }

    return is_remote;
}

texture_2d *texture_manager_new_texture_from_data(texture_manager *manager, int width, int height, const void *data) {
    texture_2d *tex = texture_2d_new_from_data(width, height, data);
    texture_manager_add_texture(manager, tex, false);
    return tex;
}

texture_2d *texture_manager_new_texture(texture_manager *manager, int width, int height) {
    LOGFN("texture_manager_new_texture");
    texture_2d *tex = texture_2d_new_from_dimensions(width, height);
    texture_manager_add_texture(manager, tex, true);
    return tex;
}

#ifdef FREE_CANVASES

static void notify_canvas_death(const char *url) {
    char *event_str;
    int event_len;

    char *dynamic_str = 0;
    char stack_str[512];

    // Generate event string
    {
        int url_len = (int)strlen(url);

        if (url_len > 300) {
            event_len = url_len + 212;
            dynamic_str = (char*)malloc(event_len);
            event_str = dynamic_str;
        } else {
            event_len = 512;
            event_str = stack_str;
        }
    }

    event_len = snprintf(event_str, event_len,
                         "{\"url\":\"%s\",\"name\":\"canvasFreed\",\"priority\":0}",
                         url);

    event_str[event_len] = '\0';

    core_dispatch_event(event_str);

    if (dynamic_str) {
        free(dynamic_str);
    }
}

#endif

texture_2d *texture_manager_get_texture(texture_manager *manager, const char *url) {
    LOGFN("texture_manager_get_texture");
    size_t len = strlen(url);
    texture_2d *tex = NULL;
    HASH_FIND(url_hash, manager->url_to_tex, url, len, tex);

    if (tex) {
        if (!tex->failed) {
            time(&tex->last_accessed);
        }

        // If we haven't accumulated this texture yet,
        if (tex->frame_epoch != m_frame_epoch) {
            tex->frame_epoch = m_frame_epoch;
            m_frame_used_bytes += tex->used_texture_bytes;
        }
#ifdef FREE_CANVASES
#ifndef FREE_CANVASES_NOTIFY_IMMEDIATELY
    } else {
        // If it was a canvas,
        if (url[0] == '_' && url[1] == '_' &&
                url[2] == 'c' && url[3] == 'a' &&
                url[4] == 'n' && url[5] == 'v' &&
                url[6] == 'a' && url[7] == 's' &&
                url[8] == '_' && url[9] == '_') {
            notify_canvas_death(url);
        }
#endif
#endif
    }

    return tex;
}

void texture_manager_get_sheet_size(char *url, int *width, int *height) {
    LOGFN("texture_manager_get_sheet_size");
    if (!spritesheet_map_root) {
        //load map.json
        char * map_str = resource_loader_string_from_url("spritesheets/spritesheetSizeMap.json");
        char * font_str = resource_loader_string_from_url("resources/fonts/fontsheetSizeMap.json");
        json_error_t error;
        spritesheet_map_root = json_loads(map_str, 0, &error);
        fontsheet_map_root = json_loads(font_str, 0, &error);
        free(map_str);
        free(font_str);
    }

    if (spritesheet_map_root) {
        json_t *sheet_obj = json_object_get(spritesheet_map_root, url);
        if (json_is_object(sheet_obj)) {
            json_t *width_obj = json_object_get(sheet_obj, "w");
            json_t *height_obj = json_object_get(sheet_obj, "h");
            if (json_is_integer(width_obj) && json_is_integer(height_obj)) {
                *width = (int)json_integer_value(width_obj);
                *height = (int)json_integer_value(height_obj);
                return;
            }
        }
    }

    if (fontsheet_map_root) {
        json_t *sheet_obj = json_object_get(fontsheet_map_root, url);
        if (json_is_object(sheet_obj)) {
            json_t *width_obj = json_object_get(sheet_obj, "w");
            json_t *height_obj = json_object_get(sheet_obj, "h");
            if (json_is_integer(width_obj) && json_is_integer(height_obj)) {
                *width = (int)json_integer_value(width_obj);
                *height = (int)json_integer_value(height_obj);
                return;
            }
        }
    }

    //default
    *width = DEFAULT_SHEET_DIMENSION;
    *height = DEFAULT_SHEET_DIMENSION;
}

texture_2d *texture_manager_load_texture(texture_manager *manager, const char *url) {
    LOGFN("texture_manager_load_texture");
    texture_2d *tex = texture_manager_get_texture(manager, url);

    if (tex) {
        return tex;
    }

    char *permanent_url = strdup(url);
    tex = texture_2d_new_from_url(permanent_url);

    bool remote_resource = is_remote_resource(permanent_url);
    bool is_contact_photo = (strncmp(url, CONTACTPHOTO_URL_PREFIX, CONTACTPHOTO_URL_PREFIX_LEN) == 0);

    if (remote_resource) {
        tex->originalWidth = tex->width = DEFAULT_REMOTE_RESOURCE_SIZE;
        tex->originalHeight = tex->height = DEFAULT_REMOTE_RESOURCE_SIZE;
    } else if (is_contact_photo) {
        tex->originalWidth = tex->width = DEFAULT_CONTACTPHOTO_SIZE;
        tex->originalHeight = tex->height = DEFAULT_CONTACTPHOTO_SIZE;
    } else {
        int width = DEFAULT_SHEET_DIMENSION, height = DEFAULT_SHEET_DIMENSION;
        texture_manager_get_sheet_size(permanent_url, &width, &height);
        tex->originalWidth = tex->width = width;
        tex->originalHeight = tex->height = height;
    }

    //add texture to texture manager
    texture_manager_add_texture(manager, tex, false);
    // Initialize pixel data to NULL to indicate it is not loaded

    // If URL represents a remote resource, or is a special resource
    if (remote_resource) {
        if (!strncmp("http", url, 4) || !strncmp("//", url, 2)) {
            image_cache_load(url);
        } else {
            launch_remote_texture_load(permanent_url);
        }
    } else {
        //lock, add to the pool and signal something has been added
        pthread_mutex_lock(&mutex);
        LIST_ADD(&tex_load_list, tex);
        pthread_cond_signal(&cond_var); //signal there is a texture to load
        pthread_mutex_unlock(&mutex);
    }

    return tex;
}

void texture_manager_on_texture_loaded(texture_manager *manager, const char *url, int name,
                                       int width, int height, int original_width, int original_height,
                                       int num_channels, int scale, bool is_text, long size, int compression_type) {
    //add the amount of bytes being used by this texture to the amount of texture bytes being used
    //scale = 1, texture stays at its regular size
    //scale = 2, texture is being halfsized as is needed for lower memory footprint
    //scale > 2, not currently used
    long used = size;
    // If no texture size was given then compute the number of bytes using the number of channels
    // and the dimensions of the image
    if (!used) {
        used = width * height * num_channels;
    }
    if (scale > 1) {
        used /= 4;
    }

    manager->texture_bytes_used += used;
    const int epoch = (unsigned)m_frame_epoch & EPOCH_USED_MASK;
    if (m_epoch_used[epoch] < manager->texture_bytes_used) {
        m_epoch_used[epoch] = manager->texture_bytes_used;
    }

    TEXLOG("Texture loaded: %s!  TOLOAD=%d USED=%d", url, (int)manager->textures_to_load, (int)manager->texture_bytes_used);

    texture_2d *tex = texture_manager_get_texture(manager, (char *)url);

    if (!tex) {
        char *permanent_url = strdup(url);
        tex = texture_2d_new_from_url(permanent_url);
    }

    tex->used_texture_bytes = used;
    manager->approx_bytes_to_load -= tex->assumed_texture_bytes;
    tex->name = name;
    tex->original_name = name;
    tex->is_text = is_text;
    tex->loaded = true;
    tex->width = width;
    tex->height = height;
    tex->scale = scale;
    tex->originalWidth = original_width;
    tex->originalHeight = original_height;
}

void texture_manager_on_texture_failed_to_load(texture_manager *manager, const char *url) {

    pthread_mutex_lock(&mutex);
    texture_2d *tex = texture_manager_get_texture(manager, url);
    if (tex) {
        tex->loaded = true;
        //texture_manager_free_texture(manager, to_be_destroyed);
        manager->approx_bytes_to_load -= tex->assumed_texture_bytes;
    }

    pthread_mutex_unlock(&mutex);
}

texture_2d *texture_manager_add_texture(texture_manager *manager, texture_2d *tex, bool is_canvas) {
    LOGFN("texture_manager_add_texture");

    texture_manager_clear_textures(manager, false);

    time(&tex->last_accessed);

    if (tex->url) {
        HASH_ADD_KEYPTR(url_hash, manager->url_to_tex, tex->url, strlen(tex->url), tex);
    }

    manager->tex_count++;

    // Approximate because it doesn't round up to the next power-of-two, etc
    long assumed_texture_bytes = tex->width * tex->height * tex->num_channels;
    if (!is_canvas) {
        if (use_halfsized_textures) {
            assumed_texture_bytes /= 4;
        }
        manager->approx_bytes_to_load += assumed_texture_bytes;
    } else {
        manager->texture_bytes_used += assumed_texture_bytes;
        const int epoch = (unsigned)m_frame_epoch & EPOCH_USED_MASK;
        if (m_epoch_used[epoch] < manager->texture_bytes_used) {
            m_epoch_used[epoch] = manager->texture_bytes_used;
        }
        tex->used_texture_bytes = assumed_texture_bytes;
    }
    tex->assumed_texture_bytes = assumed_texture_bytes;
    tex->loaded = is_canvas;

    TEXLOG("Texture added: %s!  COUNT=%d, TOLOAD=%d", tex->url, (int)manager->tex_count, (int)manager->textures_to_load);
    return tex;
}

texture_2d *texture_manager_add_texture_from_image(texture_manager *manager, const char *url, int name, int width, int height, int original_width, int original_height) {
    LOGFN("texture_manager_add_texture_from_image");
    char *permanent_url = strdup(url);
    texture_2d *tex = texture_2d_new_from_image(permanent_url, name, width, height, original_width, original_height);
    texture_manager_add_texture(manager, tex, false);
    return tex;
}

texture_2d *texture_manager_add_texture_loaded(texture_manager *manager, texture_2d *tex) {
    tex->loaded = true;
    HASH_ADD_KEYPTR(url_hash, manager->url_to_tex, tex->url, strlen(tex->url), tex);
    manager->tex_count++;
    //TODO handle the accounting stuff
    return tex;
}

static int last_accessed_compare(texture_2d *a, texture_2d *b) {
    return a->last_accessed - b->last_accessed;
}

void texture_manager_clear_textures(texture_manager *manager, bool clear_all) {
    long adjusted_max_texture_bytes = manager->max_texture_bytes - manager->approx_bytes_to_load;

    // If needs to clear textures,
    if (clear_all || manager->texture_bytes_used > adjusted_max_texture_bytes) {
#if !defined(RELEASE)
        int old_tex_count = manager->tex_count;
        int old_bytes_used = manager->texture_bytes_used;
#endif

#if defined(TEXMAN_EXTRA_VERBOSE)
        {
            texture_2d *tex = NULL;
            texture_2d *tmp = NULL;
            HASH_ITER(url_hash, manager->url_to_tex, tex, tmp) {
                TEXLOG("Before: %s canvas=%d access=%d tex-epoch: %d frame-epoch: %d", tex->url, tex->is_canvas, (int)tex->last_accessed, tex->frame_epoch, m_frame_epoch);
            }
        }
#endif

        HASH_SRT(url_hash, manager->url_to_tex, last_accessed_compare);
        texture_2d *tex = NULL;
        texture_2d *tmp = NULL;
        HASH_ITER(url_hash, manager->url_to_tex, tex, tmp) {
#ifdef FREE_CANVASES
            // do not clear a texture which has not yet been loaded!
            if (!tex->loaded) {
#else
            // keep canvases always, they have data we can't recreate
            // also do not clear a texture which has not yet been loaded!
            if (tex->is_canvas || !tex->loaded) {
#endif
                continue;
            }

            // If texture is already half-sized (or canvas) and it was used last frame,
            if ((tex->scale > 1 || tex->is_canvas) && tex->frame_epoch == m_frame_epoch) {
                // Do not reload this one (and maybe go over the memory limit)
                continue;
            }

            // we need to keep clearing until we can load the upcoming textures fine
            // if we've cleared enough that we are under max texture bytes to use,
            // then we are done
            if (!clear_all && manager->texture_bytes_used <= adjusted_max_texture_bytes) {
                break;
            }

            texture_2d *to_be_destroyed = tex;
            texture_manager_free_texture(manager, to_be_destroyed);
        }

#if !defined(RELEASE)
        LOG("{tex} Unloaded %d stale textures. Now: Texture count = %d. Bytes used = %d -> %d / %d", (int)(old_tex_count - manager->tex_count), (int)manager->tex_count, (int)old_bytes_used, (int)manager->texture_bytes_used, (int)adjusted_max_texture_bytes);
#endif

#if defined(TEXMAN_EXTRA_VERBOSE)
        {
            texture_2d *tex = NULL;
            texture_2d *tmp = NULL;
            HASH_ITER(url_hash, manager->url_to_tex, tex, tmp) {
                TEXLOG("After: %s canvas=%d access=%d", tex->url, tex->is_canvas, (int)tex->last_accessed);
            }
        }
#endif
    }
}

void texture_manager_reload_canvases(texture_manager *manager) {
    texture_2d *tex = NULL;
    texture_2d *tmp = NULL;
    HASH_ITER(url_hash, manager->url_to_tex, tex, tmp) {
        if (tex->is_canvas || tex->is_text) {
            texture_2d_reload(tex);
        }
    }
};

void texture_manager_reload(texture_manager *manager) {
    //TODO automatically reload the textures we clear instead
    //of just clearing them
    LOG("{tex} Reloading %i textures", manager->tex_count);

    pthread_mutex_lock(&mutex);
    texture_2d *cur_tex = tex_load_list;

    //remove anything waiting to be loaded from the hash
    while (cur_tex) {
        HASH_DELETE(url_hash, manager->url_to_tex, cur_tex);
        LIST_ITERATE(&tex_load_list, cur_tex);
    }

    //add offscreen canvases to a canvas list to be reloaded
    //after all the normal textures have been freed
    texture_2d *tex = NULL;
    texture_2d *tmp = NULL;
    texture_2d *canvas_list = NULL;
    HASH_ITER(url_hash, manager->url_to_tex, tex, tmp) {
        if (tex->is_canvas || tex->is_text) {
            LIST_ADD(&canvas_list, tex);
        } else {
            texture_2d *to_be_destroyed = tex;
            texture_manager_free_texture(manager, to_be_destroyed);
        }
    }

    //reload all the canvases
    cur_tex = canvas_list;
    tmp = NULL;
    while (cur_tex) {
        texture_2d_reload(cur_tex);
        tmp = cur_tex;
        LIST_ITERATE(&canvas_list, cur_tex);
        LIST_REMOVE(&canvas_list, tmp);
    }

    //re-add the previously removed textures awaiting loading
    cur_tex = tex_load_list;
    while (cur_tex) {
        HASH_ADD_KEYPTR(url_hash, manager->url_to_tex, cur_tex->url, strlen(cur_tex->url), cur_tex);
        LIST_ITERATE(&tex_load_list, cur_tex);
    }

    pthread_mutex_unlock(&mutex);
}

/**
 * @brief tries to resize texture in place without changing gl texture.  returns pointer to original texture if successful, or returns pointer to new texture if change in gl texture occurs
 */
texture_2d *texture_manager_resize_texture(texture_manager *manager, texture_2d *tex, int width, int height) {
    LOGFN("texture_manager_resize_texture");
    if (texture_2d_can_resize(tex, width, height)) {
        texture_2d_resize_unsafe(tex, width, height);
        return tex;
    } else {
        texture_2d *new_tex = texture_2d_new_from_dimensions(width, height);
        texture_manager_free_texture(manager, tex);
        texture_manager_add_texture(manager, new_tex, true);
        return new_tex;
    }
}

void texture_manager_save(texture_manager *manager) {
    LOGFN("texture_manager_save");
    texture_2d *tex = NULL;
    texture_2d *tmp = NULL;
    HASH_ITER(url_hash, manager->url_to_tex, tex, tmp) {
        if (tex->is_canvas || tex->is_text) {
            texture_2d_save(tex);
        }
    }
}

void texture_manager_free_texture(texture_manager *manager, texture_2d *tex) {
    LOGFN("texture_manager_free_texture");

    if (tex) {
        //need to subtract off the texture bytes being used as the texture is freed
        manager->texture_bytes_used -= tex->used_texture_bytes;
        HASH_DELETE(url_hash, manager->url_to_tex, tex);
        manager->tex_count--;

        if (!tex->loaded) {
            manager->approx_bytes_to_load -= tex->assumed_texture_bytes;
        }

#ifdef FREE_CANVASES_NOTIFY_IMMEDIATELY
        if (tex->is_canvas) {
            notify_canvas_death(tex->url);
        }
#endif

        TEXLOG("Texture freed: %s!  COUNT=%d, USED=%d", tex->url, (int)manager->tex_count, (int)manager->texture_bytes_used);
        texture_2d_destroy(tex);
    }
}

void texture_manager_background_texture_loader(void *dummy) {
    pthread_mutex_lock(&mutex);

    while (m_running) {
        texture_2d *cur_tex = tex_load_list;

        while (cur_tex) {
            texture_2d *old_cur = NULL;

            if (cur_tex->pixel_data == NULL && cur_tex->url != NULL && !cur_tex->failed) {
                TEXLOG("Passing to load_image_with_c: %s", cur_tex->url);

                if (!resource_loader_load_image_with_c(cur_tex)) { //if not loading from C remove from list
                    old_cur = cur_tex;
                }
            }

            LIST_ITERATE(&tex_load_list, cur_tex);

            if (old_cur) {
                LIST_REMOVE(&tex_load_list, old_cur);
                old_cur = NULL;
            }
        }

        pthread_cond_wait(&cond_var, &mutex);
    }

    pthread_mutex_unlock(&mutex);
}

CEXPORT void image_cache_load_callback(struct image_data *data) {
    int num_channels, width, height, originalWidth, originalHeight, scale, compression_type;
    long size;
    unsigned char *bytes  = texture_2d_load_texture_raw(data->url, data->bytes, data->size, &num_channels, &width, &height, &originalWidth, &originalHeight, &scale, &size, &compression_type);
    bool failed = (bytes == NULL);

    TEXLOG("image_cache_background_loader loaded %s, status: %i", data->url, failed);

    texture_manager *manager = texture_manager_get();

    pthread_mutex_lock(&mutex);
    texture_2d *tex = texture_manager_get_texture(manager, data->url);
    if (tex != NULL) {
        tex->num_channels = num_channels;
        tex->width = width;
        tex->height = height;
        tex->originalWidth = originalWidth;
        tex->originalHeight = originalHeight;
        tex->scale = scale;
        tex->failed = failed;
        tex->pixel_data = bytes;
        tex->compression_type = compression_type;
        tex->used_texture_bytes = size;
        LIST_ADD(&tex_load_list, tex);
    }

    pthread_mutex_unlock(&mutex);
}

void texture_manager_set_use_halfsized_textures() {
#ifdef LOWER_TEX_LIMIT_ON_HALFSIZE
    texture_manager *texman = texture_manager_get();
    long new_limit = use_halfsized_textures ? MAX_BYTES_FOR_HALFSIZED_TEXTURES : MAX_BYTES_FOR_FULLSIZED_TEXTURES;

    // Ratchet it down only
    if (texman->max_texture_bytes > new_limit) {
        texman->max_texture_bytes = new_limit;
    }
#endif

    if (use_halfsized_textures) {
        LOG("{tex} Using half-sized textures");
    }
}

texture_manager *texture_manager_acquire() {
    texture_manager *manager = texture_manager_get();
    pthread_mutex_lock(&mutex);
    return manager;
}

void texture_manager_release() {
    pthread_mutex_unlock(&mutex);
}

texture_manager *texture_manager_get() {
    LOGFN("texture_manager_get");

    // If it looks like the instance hasn't been created yet,
    if (!m_instance_ready) {
        // Lock mutex while racing to create the instance
        pthread_mutex_lock(&mutex);

        // If we won the race to create the instance,
        if (!m_instance_ready) {
            m_instance = (texture_manager *)malloc(sizeof(texture_manager));
            m_instance->url_to_tex = NULL;
            m_instance->tex_count = 0;
            m_instance->texture_bytes_used = 0;
            m_instance->textures_to_load = 0;
            m_instance->approx_bytes_to_load = 0;
            //default to fullsized textures
            m_instance->max_texture_bytes = MAX_BYTES_FOR_FULLSIZED_TEXTURES;
            // Start the background texture loader thread
            m_running = true;

            // Launch the loader thread
            m_load_thread = threads_create_thread(texture_manager_background_texture_loader, m_instance);

            // Mark the instance as being ready
            m_instance_ready = true;
        }

        pthread_mutex_unlock(&mutex);
    }

    return m_instance;
}

// DANGER: This function is not thread-safe.  Make sure nothing else is running
// texture_manager_get() et al while destroying an instance.
void texture_manager_destroy(texture_manager *manager) {
    LOGFN("texture_manager_destroy");
    pthread_mutex_lock(&mutex);
    m_running = false;              // Flag texture loading thread to stop
    pthread_cond_signal(&cond_var); // Signal thread to wake up and terminate
    pthread_mutex_unlock(&mutex);

    LOG("{tex} Goodnight");

    threads_join_thread(&m_load_thread);

    texture_2d *tex = NULL;
    texture_2d *tmp = NULL;
    HASH_ITER(url_hash, manager->url_to_tex, tex, tmp) {
        texture_2d_destroy(tex);
    }
    HASH_CLEAR(url_hash, manager->url_to_tex);
    free(manager);
    // Clear the texture load list
    tex_load_list = NULL;

    // If manager is the singleton instance, clear it also
    if (manager == m_instance) {
        m_instance = NULL;
        m_instance_ready = false;
    }

    // Clear memory usage profile
    m_memory_warning = false;
    m_frame_epoch = 1;
    m_frame_used_bytes = 0;
}

void texture_manager_touch_texture(texture_manager *manager, const char *url) {
    LOGFN("texture_manager_touch_texture");
    size_t len = strlen(url);
    texture_2d *tex = NULL;
    HASH_FIND(url_hash, manager->url_to_tex, url, len, tex);

    if (tex) {
        time(&tex->last_accessed);

        // If we haven't accumulated this texture yet,
        if (tex->frame_epoch != m_frame_epoch) {
            tex->frame_epoch = m_frame_epoch;
            m_frame_used_bytes += tex->used_texture_bytes;
        }
#ifdef FREE_CANVASES
#ifndef FREE_CANVASES_NOTIFY_IMMEDIATELY
    } else {
        // If it was a canvas,
        if (url[0] == '_' && url[1] == '_' &&
                url[2] == 'c' && url[3] == 'a' &&
                url[4] == 'n' && url[5] == 'v' &&
                url[6] == 'a' && url[7] == 's' &&
                url[8] == '_' && url[9] == '_') {
            notify_canvas_death(url);
        }
#endif
#endif
    }
}

static long get_epoch_used_max() {
    long highest = m_epoch_used[0];
    int i;

    for (i = 1; i < EPOCH_USED_BINS; ++i) {
        if (highest < m_epoch_used[i]) {
            highest = m_epoch_used[i];
        }
    }

    return highest;
}

void texture_manager_tick(texture_manager *manager) {
    LOGFN("texture_manager_tick");
    pthread_mutex_lock(&mutex);

    // If memory warning encountered,
    if (m_memory_warning) {
        m_memory_warning = false;

        // Chip some MB off the high water mark in the last X frames and use that as the new limit
        long highest = get_epoch_used_max();

        // Only ratchet memory limit downward
        if (highest > manager->max_texture_bytes) {
            highest = manager->max_texture_bytes;
        }

        long new_limit = highest - LOW_MEM_DROP_RATE;

        // If limit drops under half-sizing threshold,
        if (!use_halfsized_textures) {
            if (new_limit < MIN_BYTES_FOR_FULLSIZE_TEXTURES) {
                new_limit = MIN_BYTES_FOR_FULLSIZE_TEXTURES;

                // Start using half-sized textures
                use_halfsized_textures = true;
                set_halfsized_textures(true);

                LOG("{tex} WARNING: Detected very low available memory! Reacting by half-sizing textures from here on out");
            }
        }

        // Do not allow it to go too low
        if (new_limit < MIN_BYTES_FOR_TEXTURES) {
            new_limit = MIN_BYTES_FOR_TEXTURES;
        }

        // Update the max texture bytes limit
        manager->max_texture_bytes = new_limit;

        // Zero the epoch used bins since it will never get that high again
        memset(m_epoch_used, 0, sizeof(m_epoch_used));

        LOG("{tex} WARNING: Low memory warning! Texture memory limit now %ld", manager->max_texture_bytes);

        // And run clear textures now to get back under the new limit:
    }

    // If not half-sizing yet,
    if (!use_halfsized_textures) {
        long adjusted_max_texture_bytes = manager->max_texture_bytes - manager->approx_bytes_to_load;

        // If bytes used this frame exceed the max,
        if (m_frame_used_bytes > adjusted_max_texture_bytes) {
            // Start using half-sized textures
            use_halfsized_textures = true;
            set_halfsized_textures(true);

            LOG("{tex} WARNING: Detected a texture load loop condition! Reacting by half-sizing textures from here on out");
        }
    }

    //clear uneeded textures and make space for ones about to be loaded
    texture_manager_clear_textures(manager, false);

    // Invalidate earlier frame epochs tagged on textures
    m_frame_epoch++;
    m_frame_used_bytes = 0;

    // Initialize the high water mark for this frame
    const int epoch = (unsigned)m_frame_epoch & EPOCH_USED_MASK;
    m_epoch_used[epoch] = manager->texture_bytes_used;

#if defined(TEXMAN_VERBOSE)
    if (epoch == 0) {
        long highest = get_epoch_used_max();
        LOG("{tex} Frame texture memory usage for interval = %ld", highest);
    }
#endif

    // Load new textures
    texture_2d *cur_tex = tex_load_list;
    while (cur_tex) {
        //skip this if texture is not ready to load
        if (!cur_tex->failed && (cur_tex->pixel_data == NULL || cur_tex->url == NULL)) {
            LIST_ITERATE(&tex_load_list, cur_tex);
            continue;
        }

        GLuint texture = 0;
        if (!cur_tex->failed) {
            GLTRACE(glGenTextures(1, &texture));
            GLTRACE(glBindTexture(GL_TEXTURE_2D, texture));
            GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
            GLTRACE(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

            //create the texture
            int channels = cur_tex->num_channels;
            int width = cur_tex->width >> (cur_tex->scale - 1);
            int height = cur_tex->height >> (cur_tex->scale - 1);

            if (cur_tex->compression_type) {
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, cur_tex->compression_type, width, height, 0, cur_tex->used_texture_bytes, cur_tex->pixel_data);
            } else {
                // Select the right internal and input format based on the number of channels
                GLint format;
                switch (channels) {
                case 1:
                    format = GL_LUMINANCE;
                    break;
                case 3:
                    format = GL_RGB;
                    break;
                default:
                case 4:
                    format = GL_RGBA;
                    break;
                }
                GLTRACE(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, cur_tex->pixel_data));
            }
            core_check_gl_error();
            texture_manager_on_texture_loaded(manager, cur_tex->url, texture,
                                              cur_tex->width, cur_tex->height, cur_tex->originalWidth, cur_tex->originalHeight,
                                              cur_tex->num_channels, cur_tex->scale, false, cur_tex->used_texture_bytes, cur_tex->compression_type);
        }

        char *event_str;
        int event_len;

        char *dynamic_str = 0;
        char stack_str[512];

        // Generate event string
        {
            int url_len = (int)strlen(cur_tex->url);

            if (url_len > 300) {
                event_len = url_len + 212;
                dynamic_str = (char*)malloc(event_len);
                event_str = dynamic_str;
            } else {
                event_len = 512;
                event_str = stack_str;
            }
        }

        if (cur_tex->failed) {
            event_len = snprintf(event_str, event_len,
                                 "{\"url\":\"%s\",\"name\":\"imageError\",\"priority\":0}",
                                 cur_tex->url);
        } else {
            //create json event string
            event_len = snprintf(event_str, event_len,
                                 "{\"url\":\"%s\",\"height\":%d,\"originalHeight\":%d,\"originalWidth\":%d" \
                                 ",\"glName\":%d,\"width\":%d,\"name\":\"imageLoaded\",\"priority\":0}",
                                 cur_tex->url, (int)cur_tex->height,
                                 (int)cur_tex->originalHeight, (int)cur_tex->originalWidth,
                                 (int)texture, (int)cur_tex->width);
        }

        event_str[event_len] = '\0';

        //dispatch the event
        pthread_mutex_unlock(&mutex);
        // TODO: Is this a thread safety problem?  Does the tex_load_list get modified from another thread?
        core_dispatch_event(event_str);

        // If dynamically allocated the string,
        if (dynamic_str) {
            free(dynamic_str);
        }

        pthread_mutex_lock(&mutex);

        free(cur_tex->pixel_data);
        cur_tex->pixel_data = NULL;

        texture_2d *old_cur = cur_tex;
        LIST_ITERATE(&tex_load_list, cur_tex);
        LIST_REMOVE(&tex_load_list, old_cur);
    }

    pthread_mutex_unlock(&mutex);
}

/*
 * Every memory warning should be taken very seriously.
 *
 * We should measure the high water mark for texture memory usage and use that
 * as an indicator of what is "too much."  We then ratchet down the memory
 * limit to a safe-feeling value less than "too much."
 *
 * We then expunge textures until we are under that new limit.
 *
 * We should measure the high water mark for texture memory usage per-frame.
 * If the memory limit is below this, then we will enter a texture load loop
 * that will kill performance.  So in this case we should switch on half-sized
 * textures.  The texture load loop may happen, but it will resolve itself
 * quickly by reloading textures half-sized.
 */
void texture_manager_memory_warning(texture_manager *manager) {
    LOGFN("texture_manager_memory_warning");

    m_memory_warning = true;
}


/*
 * For devices with known low memory limits, we can start with a lower memory limit
 * to avoid depending on low memory warnings.  This fixes issues where we cannot
 * handle the warning fast enough on larger games.
 */
void texture_manager_set_max_memory(texture_manager *manager, int bytes) {
    LOGFN("texture_manager_set_max_memory");

    // Do not allow falling below the minimum
    if (bytes < MIN_BYTES_FOR_TEXTURES) {
        bytes = MIN_BYTES_FOR_TEXTURES;
    }

    if (manager->max_texture_bytes > bytes) {
        manager->max_texture_bytes = bytes;
    }
}

