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

#ifndef LOCAL_STORAGE_H
#define LOCAL_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

void local_storage_set_data(const char *key, const char *data);
const char *local_storage_get_data(const char *key);
void local_storage_remove_data(const char *key);
void local_storage_clear();

#ifdef __cplusplus
}
#endif

#endif
