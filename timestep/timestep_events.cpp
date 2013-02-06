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

#include "timestep_events.h"
#include "core/log.h"

static unsigned int count = 0;
static const unsigned int buffer_size = 32;
static input_event events[buffer_size];

CEXPORT void timestep_events_push(int id, int type, int x, int y) {

	LOGFN("timestep_events_push");
	if (count < buffer_size) {
		input_event *t = &events[count];
		t->id = id;
		t->type = type;
		t->x = x;
		t->y = y;

		++count;
	}
	LOGFN("end timestep_events_push");
}

// WARNING: caller must immediately make a copy since the data
// will be destroyed
CEXPORT input_event_list timestep_events_get() {
	LOGFN("timestep_events_get");
	int current_count = count;

	count = 0;

	LOGFN("end timestep_events_get");
	return (input_event_list_t) {
		events,
		current_count
	};
}

CEXPORT void timestep_events_shutdown() {
	count = 0;
}
