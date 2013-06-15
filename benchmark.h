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

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "uthash/uthash.h"

typedef struct bench_entry_t {
	const char *name;
	double start_time;
	double end_time;
	UT_hash_handle hh;
} bench_entry;

typedef struct bench_t {
	bench_entry *benches;
	double start_time;
	const char *name;
} bench;

bench *get_bench(const char *name);
void start_bench(bench *b, const char *name);
void end_bench(bench *b, const char *name);
void print_benches(bench *b);


#endif //BENCHMARK_H

