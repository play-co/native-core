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

#ifndef TEALEAF_SHADER_H
#define TEALEAF_SHADER_H
#include "core/types.h"

enum SHADERS { PRIMARY_SHADER, DRAWING_SHADER, FILL_RECT_SHADER, LINEAR_ADD_SHADER, NUM_SHADERS };
bool use_single_shader;
typedef struct shader_t {
	int program;

	union {
		// primary shader
		struct {
			int tex_coords;
		};

		// drawing shader
		struct {
			int point_size;
		};

		// fill rect shader
		// (nothing special)

	};

	int proj_matrix;
	int vertex_coords;
	int draw_color;
	int tex_sampler;
	int add_color;
	unsigned int last_width;
	unsigned int last_height;

} tealeaf_shader;

tealeaf_shader global_shaders[NUM_SHADERS];
unsigned int current_shader;

void tealeaf_shaders_init();
void tealeaf_shaders_bind(unsigned int shader_type);

#endif // TEALEAF_SHADER_H
