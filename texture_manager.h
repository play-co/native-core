/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with the Game Closure SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "core/texture_2d.h"

#include <pthread.h>
#define MAX_TEXTURE_COUNT 256


typedef struct texture_manager_t {
	texture_2d *url_to_tex;
	int textures_to_load;
	long texture_bytes_used;
	long approx_bytes_to_load;
	long max_texture_bytes;
	int tex_count;
} texture_manager;


#ifdef __cplusplus
extern "C" {
#endif

void texture_manager_tick(texture_manager *manager);
texture_2d *texture_manager_new_texture(texture_manager *manager, int width, int height);
texture_2d *texture_manager_get_texture(texture_manager *manager, const char *url);
texture_2d *texture_manager_add_texture(texture_manager *manager, texture_2d *tex, bool is_canvas);
texture_2d *texture_manager_add_texture_from_image(texture_manager *manager, const char *url, int name, int width, int height, int original_width, int original_height);
texture_2d *texture_manager_load_texture(texture_manager *manager, const char *url);
texture_2d *texture_manager_load_texture_with_size(texture_manager *manager, const char *url, int width, int height);
void texture_manager_reload_canvases(texture_manager *manager);
void texture_manager_reload(texture_manager *manager);
void texture_manager_save(texture_manager *manager);
texture_manager *texture_manager_get();
void texture_manager_destroy(texture_manager *manager);
void texture_manager_clear_textures(texture_manager *manager, bool clear_all);
void texture_manager_free_texture(texture_manager *manager, texture_2d *tex);
void texture_manager_touch_texture(texture_manager *manager, const char *url);
void texture_manager_set_use_halfsized_textures();
texture_2d *texture_manager_update_texture(texture_manager *manager, const char *url, int name,
											int width, int height, int original_width, int original_height,
											int num_channels, int scale, bool is_text, long used);

void texture_manager_on_texture_loaded(texture_manager *manager, const char *url, int name,
										int width, int height, int original_width, int original_height,
										int num_channels, int scale, bool is_text);
void texture_manager_on_texture_failed_to_load(texture_manager *manager, const char *url);
void texture_manager_memory_warning(texture_manager *manager);
void texture_manager_set_max_memory(texture_manager *manager, int bytes); // Will only ratchet down

#ifdef __cplusplus
}
#endif

#endif
