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

#ifndef TIMESTEP_TEXT_DATA_H
#define TIMESTEP_TEXT_DATA_H
#include "core/types.h"
#include "core/rgba.h"

typedef struct timestep_text_data_t {
	rgba color;
	rgba background_color;

	double horizontal_padding;
	double vertical_padding;
	double line_height;

	char *text_align;
	char *vertical_align;
	
	bool multiline;

	int font_size;
	char *font_family;
	char *font_weight;
	char *stroke_style;

	double line_width;

	char *text;

	bool shadow;
	rgba shadow_color;
} timestep_text_data;

timestep_text_data *timestep_text_data_init();
void timestep_text_data_delete(timestep_text_data *text_data);

#endif
