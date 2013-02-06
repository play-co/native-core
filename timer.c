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

/**
 * @file	 timer.c
 * @brief
 */
#include "core/timer.h"
#include <stdlib.h>
#include "core/log.h"
#include "util/detect.h"

static core_timer *timer_head = NULL;

static int timer_id = 0;

/**
 * @name	core_timer_schedule
 * @brief	adds the given timer to the timer list
 * @param	timer - (core_timer *) timer to add
 * @retval	NONE
 */
CEXPORT void core_timer_schedule(core_timer *timer) {
	if (!timer_head) {
		timer_head = timer;
	} else {
		core_timer *current = timer_head;

		while (current->next && current->time_left <= timer->time_left) {
			current = current->next;
		}

		if (current->prev) {
			current->prev->next = timer;
		} else {
			timer_head = timer; //it must be the head
		}

		timer->prev = current->prev;
		current->prev = timer;
		timer->next = current;
	}
}

/**
 * @name	timer_unschedule
 * @brief	sets the clear flag on the timer, to have it unlinked in tick
 * @param	timer - (core_timer *) timer to unschedule
 * @retval	NONE
 */
CEXPORT static void timer_unschedule(core_timer *timer) {
	timer->cleared = true;
}

/**
 * @name	timer_unlink
 * @brief	removes the timer from the timer list
 * @param	timer - (core_timer *) timer to remove
 * @retval	NONE
 */
CEXPORT void timer_unlink(core_timer *timer) {
	if (timer->prev) {
		timer->prev->next = timer->next;
	} else {
		timer_head = timer->next;
	}

	if (timer->next) {
		timer->next->prev = timer->prev;
	}

	if (timer == timer_head) {
		timer_head = NULL;
	}

	js_timer_unlink(timer);
	free(timer->js_data);
	free(timer);
}

/**
 * @name	timer_fire
 * @brief	fire's the given timer to js
 * @param	timer - (core_timer *) timer to fire
 * @retval	NONE
 */
CEXPORT static void timer_fire(core_timer *timer) {
	js_timer_fire(timer);

	if (!timer->repeat) {
		timer_unschedule(timer);
	} else {
		timer->time_left = timer->duration;
	}
}

/**
 * @name	core_get_timer
 * @brief	creates and returns a timer using the given params
 * @param	js_data - (void *) javascript data to attach to the timer
 * @param	time - (int) how long the timer should be fore
 * @param	repeat - (bool) whether the timer should be repeating or not
 * @retval	core_timer* - pointer to the created timer
 */
CEXPORT core_timer *core_get_timer(void *js_data, int time, bool repeat) {
	core_timer *timer = (core_timer *)malloc(sizeof(core_timer));
	timer->time_left = time;
	timer->duration = time;
	timer->id = timer_id++;
	timer->next = NULL;
	timer->prev = NULL;
	timer->repeat = repeat;
	timer->cleared = false;
	timer->js_data = js_data;
	return timer;
}

/**
 * @name	core_timer_clear
 * @brief	unschedules timers with the given id
 * @param	id - (int) id to unschedule timers with
 * @retval	NONE
 */
CEXPORT void core_timer_clear(int id) {
	core_timer *timer = timer_head;

	while (timer) {
		if (timer->id == id) {
			timer_unschedule(timer);
			return;
		}

		timer = timer->next;
	}

	LOG("{timer} Tried to clear timer %i when it didn't exist", id);
}

/**
 * @name	core_timer_clear_all
 * @brief	unlinks (removes) all timers in the timer list
 * @retval	NONE
 */
CEXPORT void core_timer_clear_all() {
	core_timer *timer = timer_head;

	while (timer) {
		core_timer *next = timer->next;
		timer_unlink(timer);
		timer = next;
	}

	timer_head = NULL;
}

/**
 * @name	core_timer_tick
 * @brief	ticks all of the timers, decrementing time, firing and unlinking as needed
 * @param	dt - (int) elapsed time since last tick
 * @retval	NONE
 */
CEXPORT void core_timer_tick(int dt) {
	if (dt < 0) {
		return;
	}

	core_timer *timer = timer_head;

	while (timer) {
		core_timer *next = timer->next;

		if (!timer->cleared) {
			timer->time_left -= dt;

			if (timer->time_left <= 0) {
				timer_fire(timer);
			}
		} else {
			timer_unlink(timer);
		}

		timer = next;
	}
}

