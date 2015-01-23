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
 * @file	 object_pool.c
 * @brief
 */
#include "object_pool.h"
#include <stdlib.h>
#include <stdio.h>

#include "log.h"

/**
 * @name	object_pool_init
 * @brief	initilizes the object pool with the given size and item size
 * @param	initial_size - (unsigned int) initial size the pull should be (of items)
 * @param	item_size - (unsigned int) the size of each item in the pool
 * @retval	object_pool* - the object pool created after being initilized
 */
object_pool *object_pool_init(unsigned int initial_size, size_t item_size) {
    LOGFN("object_pool_init");
    object_pool *pool = (object_pool *) malloc(sizeof(object_pool));
    pool->max_size = initial_size;
    pool->avail_count = 0;
    pool->items = (void **) malloc(sizeof(void *) * initial_size);
    pool->item_size = item_size + sizeof(void *);
    unsigned int i;

    for (i = 0; i < initial_size; ++i) {
        pool->items[i] = malloc(pool->item_size);
        ++pool->avail_count;
    }

    LOGFN("end object_pool_init");
    return pool;
}

/**
 * @name	object_pool_put
 * @brief	puts the given object into the object pool
 * @param	obj - (void *) object to be put into the object pool
 * @retval	NONE
 */
void object_pool_put(void *obj) {
    LOGFN("object_pool_put");
    object_pool *pool = (object_pool *)((void **) obj)[-1];

    if (pool->avail_count == pool->max_size) {
        pool->max_size = pool->max_size == 0 ? 1 : pool->max_size * 2;
        pool->items = (void **)realloc(pool->items, sizeof(void *) * pool->max_size);
    }

    pool->items[pool->avail_count] = (void **)obj - 1;
    pool->avail_count++;
    LOGFN("end object_pool_put");
}

/**
 * @name	object_pool_get
 * @brief	___
 * @param	pool - (object_pool *)
 * @retval	void* -
 */
void *object_pool_get(object_pool *pool) {
    LOGFN("object_pool_get");
    void **obj;

    if (pool->avail_count) {
        --pool->avail_count;
        obj = (void **)pool->items[pool->avail_count];
        //LOG("object_pool: reuse of size %d, %p", pool->item_size, (obj + 1));
    } else {
        obj = (void **) malloc(pool->item_size);
        //LOG("object_pool: malloc of size %d, %p", pool->item_size, (obj + 1));
    }

    obj[0] = (void *) pool;
    LOGFN("end object_pool_get");
    return (void *)(obj + 1);
}

/**
 * @name	object_pool_destroy
 * @brief	destroys the given object pool
 * @param	pool - (object_pool *) the object pool to destroy
 * @retval	NONE
 */
void object_pool_destroy(object_pool *pool) {
    LOGFN("object_pool_destroy");

    while (pool->avail_count) {
        --pool->avail_count;
        free(pool->items[pool->avail_count]);
    }

    free(pool->items);
    free(pool);
    LOGFN("end object_pool_destroy");
}
