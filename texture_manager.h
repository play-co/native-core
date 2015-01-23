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

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "core/texture_2d.h"
#include "core/image-cache/include/image_cache.h"

#include <pthread.h>
#define MAX_TEXTURE_COUNT 256


typedef struct texture_manager_t {
	texture_2d *url_to_tex;
	int textures_to_load;
	size_t texture_bytes_used;
	size_t approx_bytes_to_load;
	size_t max_texture_bytes;
	int tex_count;
} texture_manager;


#ifdef __cplusplus
extern "C" {
#endif

void texture_manager_tick(texture_manager *manager);
texture_2d *texture_manager_new_texture_from_data(texture_manager *manager, int width, int height, const void *data);
texture_2d *texture_manager_new_texture(texture_manager *manager, int width, int height);
texture_2d *texture_manager_get_texture(texture_manager *manager, const char *url);
texture_2d *texture_manager_add_texture(texture_manager *manager, texture_2d *tex, bool is_canvas);
texture_2d *texture_manager_add_texture_from_image(texture_manager *manager, const char *url, int name, int width, int height, int original_width, int original_height);
texture_2d *texture_manager_add_texture_loaded(texture_manager *manager, texture_2d *tex);
texture_2d *texture_manager_load_texture(texture_manager *manager, const char *url);
texture_2d *texture_manager_load_texture_with_size(texture_manager *manager, const char *url, int width, int height);
void texture_manager_reload_canvases(texture_manager *manager);
void texture_manager_reload(texture_manager *manager);
texture_2d *texture_manager_resize_texture(texture_manager *manager, texture_2d *tex, int width, int height);
void texture_manager_save(texture_manager *manager);
texture_manager *texture_manager_get();
void texture_manager_destroy(texture_manager *manager);
void texture_manager_clear_textures(texture_manager *manager, bool clear_all);
void texture_manager_free_texture(texture_manager *manager, texture_2d *tex);
void texture_manager_touch_texture(texture_manager *manager, const char *url);
void texture_manager_set_use_halfsized_textures(bool use_halfsized);
texture_2d *texture_manager_update_texture(texture_manager *manager, const char *url, int name,
											int width, int height, int original_width, int original_height,
											int num_channels, int scale, bool is_text, long used);

bool texture_manager_on_texture_loaded(texture_manager *manager,
                                       const char *url,
                                       int name,
                                       int width,
                                       int height,
                                       int original_width,
                                       int original_height,
                                       int num_channels,
                                       int scale,
                                       bool is_text,
                                       long size,
                                       int compression_type);
void texture_manager_on_texture_failed_to_load(texture_manager *manager, const char *url);
void texture_manager_memory_warning();
void texture_manager_set_max_memory(texture_manager *manager, long bytes); // Will only ratchet down
void image_cache_load_callback(struct image_data *data);
texture_manager *texture_manager_acquire();
void texture_manager_release();

#ifdef __cplusplus
}
#endif

#endif
