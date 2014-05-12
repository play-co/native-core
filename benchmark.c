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
 * @file	 benchmark.c
 * @brief
 */
#include "core/benchmark.h"
#include "core/log.h"
#include <sys/time.h>

/**
 * @name	now
 * @brief	gets the current time in microseconts
 * @retval	double - current time in microseconds
 */
static double now() {
    LOG("enter now");
    struct timeval n;
    gettimeofday(&n, NULL);
    LOG("exit now");
    double ret = (n.tv_sec * 1000 * 1000) + n.tv_usec;
    LOG("returning %f from %li %li", ret, n.tv_sec, n.tv_usec);
    return ret;
}

/**
 * @name	get_bench
 * @brief	gets a pointer to a benchmark with the given name
 * @param	name - (const char*) name of the benchmark to get
 * @retval	bench* - benchmark addressed by the given name
 */
bench *get_bench(const char *name) {
    bench *b = (bench *)malloc(sizeof(bench));
    b->start_time = now();
    b->benches = NULL;
    b->name = strdup(name);
    return b;
}

/**
 * @name	destroy_bench
 * @brief	destroys the given benchmark
 * @param	b - (bench*) the benchmark to destroy
 * @retval	NONE
 */
void destroy_bench(bench *b) {
    bench_entry *bench = NULL;
    bench_entry *tmp = NULL;
    HASH_ITER(hh, b->benches, bench, tmp) {
        HASH_DELETE(hh, b->benches, bench);
        free((void *)bench->name);
        free(bench);
    }
    free((void *)b->name);
    free(b);
}

/**
 * @name	start_bench
 * @brief	starts a benchmark entry "name" from the given benchmark
 * @param	b - (bench*) benchmark holding the benchmark entries
 * @param	name - (char*) name of the benchmark entry to start
 * @retval	NONE
 */
void start_bench(bench *b, const char *name)  {
    LOG("starting bench %s on %s", name, b->name);
    bench_entry *entry = NULL;
    HASH_FIND(hh, b->benches, name, strlen(name), entry);

    if (!entry) {
        entry = (bench_entry *)malloc(sizeof(bench_entry));
        entry->name = strdup(name);
        entry->start_time = now();
        entry->end_time = 0;
        HASH_ADD_KEYPTR(hh, b->benches, entry->name, strlen(entry->name), entry);
    } else {
        LOG("ERROR! Tried to start bench %s twice", name);
    }

    LOG("ending bench %s on %s", name, b->name);
}

/**
 * @name	end_bench
 * @brief	ends a benchmark entry "name" from the given benchmark
 * @param	b - (bench*) benchmark holding the benchmark entriese
 * @param	name - (const char*) name of the benchmark entry to destroy
 * @retval	NONE
 */
void end_bench(bench *b, const char *name) {
    bench_entry *entry = NULL;
    HASH_FIND(hh, b->benches, name, strlen(name), entry);

    if (entry) {
        entry->end_time = now();
        LOG("ended bench %s at %f", entry->name, entry->end_time);
    } else {
        LOG("ERROR! Tried to stop nonexistant bench %s", name);
    }
}


/**
 * @name	print_benches
 * @brief	prints out information from the benchmark entries of the give benchmark
 * @param	b - (bench*) benchmark to print benchmark entry information from
 * @retval	NONE
 */
void print_benches(bench *b) {
    bench_entry *entry = NULL;
    bench_entry *tmp = NULL;
    LOG("==============%s===============", b->name);
    HASH_ITER(hh, b->benches, entry, tmp) {
        if (entry->end_time != 0) {
            float start_time = (entry->start_time - b->start_time) / 1000.0;
            float end_time = (entry->end_time - b->start_time) / 1000.0;
            LOG("%s: start: %.2fms end: %.2fms elapsed: %.2f ms",
                entry->name, start_time, end_time,
                end_time - start_time);
        }
    }
    LOG("====================================");
}
