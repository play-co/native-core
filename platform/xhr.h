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

#ifndef XHR_H
#define XHR_H

#include "core/deps/uthash/uthash.h"

typedef struct request_header_t {
	char *header;
	char *value;
	UT_hash_handle hh;
} request_header;

typedef struct xhr_t {
	int id;
	const char *method;
	const char *url;
	const char *data;
	request_header *request_headers;
	bool async;
	int state;
} xhr;

#ifdef __cplusplus
extern "C" {
#endif

void xhr_send(xhr *req);
	
#ifdef __cplusplus
}
#endif

#endif
