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

/**
 * @file	 url_loader.c
 * @brief
 */
#include "platform/resource_loader.h"

/**
 * @name	core_load_url
 * @brief	loads and returns a string from a given url / filename
 * @param	url - (const char *) url / filename to load from
 * @retval	char* - contents found in the file
 */
char *core_load_url(const char *url) {
    return resource_loader_string_from_url(url);
}
