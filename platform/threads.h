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

#ifndef CORE_THREADS_H
#define CORE_THREADS_H

#include "core/util/detect.h"

// Prototype for a thread procedure
typedef void (*ThreadsThreadProc)(void *);

// Opaque handle for a thread
typedef void *ThreadsThread;

#define THREADS_INVALID_THREAD (ThreadsThread)( 0 )

// Start a thread
CEXPORT ThreadsThread threads_create_thread(ThreadsThreadProc proc, void *param);

// Join the thread (MUST call this at some point for each started thread)
CEXPORT void threads_join_thread(ThreadsThread *thread);

#endif
