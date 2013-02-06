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

#ifndef RGBA_H
#define RGBA_H

#define RGBA_MAX_STR_LEN 60

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"

	typedef struct rgba_t {
		float r, g, b, a;
	} rgba;

	void rgba_init();
	void rgba_parse(rgba *color, const char *src);
	void rgba_print(rgba *color);
	bool rgba_equals(rgba *a, rgba *b);
	// returns length of string
	// TODO: Should pass in buf_bytes here
	int rgba_to_string(rgba *color, char *buf);


#ifdef __cplusplus
}
#endif

#endif // RGBA_H
