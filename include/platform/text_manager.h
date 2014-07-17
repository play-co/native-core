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

#ifndef TEXT_MANAGER_H
#define TEXT_MANAGER_H

#include "core/texture_2d.h"
#include "core/rgba.h"

enum TEXT_STYLE {
	TEXT_STYLE_FILL,
	TEXT_STYLE_STROKE
};

#ifdef __cplusplus
extern "C" {
#endif

texture_2d *text_manager_get_text(const char *font_name, int size, const char *text, rgba *color, int max_width, int text_style, float stroke_width);
texture_2d *text_manager_get_filled_text(const char *font_name, int size, const char *text, rgba *color, int max_width);
texture_2d *text_manager_get_stroked_text(const char *font_name, int size, const char *text, rgba *color, int max_width, float stroke_width);
int text_manager_measure_text(const char *font_name, int size, const char *text);
	
#ifdef __cplusplus
}
#endif
	
#endif
