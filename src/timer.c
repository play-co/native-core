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
 * @file	 timer.c
 * @brief
 */
#include "core/timer.h"
#include <stdlib.h>
#include "core/log.h"
#include "util/detect.h"

static core_timer *timer_head = NULL;
static core_timer *m_insert_head = NULL;

static int timer_id = 0;

#define MAX_TIMERS_PER_TICK 400

core_timer* core_get_timers() {
  return timer_head;
}

core_timer* core_get_queued_timers() {
  return m_insert_head;
}

static void queue_insert(core_timer *timer) {
    timer->prev = 0;
    timer->next = m_insert_head;
    m_insert_head = timer;
}

static void insert_timer(core_timer *timer) {
    if (!timer_head) {
        timer_head = timer;
        timer->prev = 0;
        timer->next = 0;
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

static void insert_queued_timers() {
    core_timer *timer = m_insert_head;
    core_timer *next;

    for (; timer; timer = next) {
        next = timer->next;
        insert_timer(timer);
    }

    m_insert_head = NULL;
}

// Return non-zero if timer was cleared
static int unschedule_queued_timer(int id) {
    core_timer *timer = m_insert_head;
    core_timer *next;

    for (; timer; timer = next) {
        next = timer->next;

        if (timer->id == id) {
            timer->cleared = true;
            return 1;
        }
    }

    return 0;
}

/**
 * @name	core_timer_schedule
 * @brief	adds the given timer to the timer list
 * @param	timer - (core_timer *) timer to add
 * @retval	NONE
 */
CEXPORT void core_timer_schedule(core_timer *timer) {
    queue_insert(timer);
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

    // It may be in the queue still
    if (unschedule_queued_timer(id)) {
        return;
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

    LOG("{CAT} CLEARING ALL TIMERS");

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
CEXPORT void core_timer_tick(long dt) {
    insert_queued_timers();

    if (dt < 0) {
        return;
    }

    core_timer *timer = timer_head;
    core_timer *last;
    int max_ticks = MAX_TIMERS_PER_TICK;

    while (timer) {
        last = timer;

        if (--max_ticks <= 0) {
            LOG("{timer} WARNING: More than %d timer callbacks in one tick.  Waiting for next tick", MAX_TIMERS_PER_TICK);
            break;
        }

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
        if (timer == last) {
            LOG("{timer} WARNING: Infinite loop detected.  Dumping all timers!");
            core_timer_clear_all();
            break;
        }
    }
}

