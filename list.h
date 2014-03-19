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

#define LIST_ADD(head, node) do {                \
		if (!*head) {                                \
			*head = node;                            \
			node->prev = *head;                      \
			node->next = *head;                      \
		} else {                                     \
			node->next = *head;                      \
			node->prev = (*head)->prev;              \
			(*head)->prev->next = node;              \
			(*head)->prev = node;                    \
		}                                            \
	} while (0)

#define LIST_REMOVE(head, node) do {             \
		if (node->next == node) {                    \
			if (*head == node) {                     \
				*head = NULL;                        \
			}                                        \
		} else {                                     \
			node->prev->next = node->next;           \
			node->next->prev = node->prev;           \
			if (node == *head) {                     \
				*head = node->next;                  \
			}                                        \
        }                                            \
        node->prev = NULL;                           \
        node->next = NULL;                           \
} while (0)

//DO NOT REMOVE FROM LIST BEFORE an ITERATE
//You can safely remove after an iterate by
//saving a reference to the item
#define LIST_ITERATE(head, current) do {         \
		if (*head) {                                 \
			if (current->next == *head) {            \
				current = NULL;                      \
			} else {                                 \
				current = current->next;             \
			}                                        \
		} else {                                     \
			current = NULL;                          \
		}                                            \
	} while (0)

#define LIST_IN_LIST(head, current) ((current)->prev && (current)->next)

#define DEBUG_TEST_LIST_IN(head, node) do {          \
		void *item = (void *) node;                      \
		int seen_head = 0;                               \
		if (*head) {                                     \
			while (node) {                               \
				if ((void *) node->next == item) {       \
					node = node->next;                   \
					break;                               \
				}                                        \
				\
				if (node->next == *head) { seen_head++; } \
				if (seen_head == 2) {                    \
					node = NULL;                         \
					break;                               \
				}                                        \
				\
				node = node->next;                       \
			}                                            \
		} else {                                         \
			node = NULL;                                 \
		}                                                \
	} while (0)
