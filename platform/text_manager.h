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
