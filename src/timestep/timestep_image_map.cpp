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

#include <stdlib.h>

#include "timestep_image_map.h"
#include "core/log.h"

timestep_image_map *timestep_image_map_init() {
    timestep_image_map *map = (timestep_image_map *) malloc(sizeof(timestep_image_map));
#if defined(DEBUG)
    map->canary = CANARY_GOOD;
#endif
    map->url = 0;
    return map;
}

void timestep_image_delete(timestep_image_map *map) {
    if (map->url) {
        free(map->url);
    }

#if defined(DEBUG)
    map->canary = 0;
#endif
    free(map);
}

