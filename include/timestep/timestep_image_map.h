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

#ifndef TIMESTEP_IMAGE_MAP_H
#define TIMESTEP_IMAGE_MAP_H

typedef struct timestep_image_map_t {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	int32_t margin_top;
	int32_t margin_right;
	int32_t margin_bottom;
	int32_t margin_left;
	int32_t sheet_width;
	int32_t sheet_height;
#if defined(DEBUG)
	unsigned int canary;
#endif
	char *url;
} timestep_image_map;

#if defined(DEBUG)
#define CANARY_GOOD 0xdeaddeed
#endif

typedef struct timestep_sprite_t {
	timestep_image_map **frames;
	unsigned int fps;
} timestep_sprite;

timestep_image_map *timestep_image_map_init();
void timestep_image_delete(timestep_image_map *map);

#endif
