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

#define OPT_MERGE_EVENTS

static const int BUFFER_SIZE = 32;
static input_event m_events[BUFFER_SIZE];
static int m_count = 0;

CEXPORT void timestep_events_push(int id, int type, int x, int y) {
    int ii;

    LOGFN("timestep_events_push");

#ifdef OPT_MERGE_EVENTS
    for (ii = 0; ii < m_count; ++ii) {
        input_event *t = &m_events[ii];

        if (t->id == id && t->type == type) {
            t->x = x;
            t->y = y;
            return;
        }
    }
#endif

    if (m_count < BUFFER_SIZE) {
        input_event *t = &m_events[m_count++];

        t->id = id;
        t->type = type;
        t->x = x;
        t->y = y;
    }

    LOGFN("end timestep_events_push");
}

// WARNING: caller must immediately make a copy since the data
// will be destroyed
// TODO that seems like a bad api
CEXPORT input_event_list timestep_events_get() {
    unsigned current_count = m_count;

    LOGFN("timestep_events_get");

    m_count = 0;

    LOGFN("end timestep_events_get");

    return (input_event_list_t) {
        m_events,
        current_count
    };
}

CEXPORT void timestep_events_shutdown() {
    m_count = 0;
}

