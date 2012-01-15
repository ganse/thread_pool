
/*
 * test_workq.c
 * This file is part of thread_pool - Thread Pool server
 *
 * Copyright (C) 2012 - Ian Ganse
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; specifically version 2.x of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "workq.h"

#define ADD_OR_DIE(_s, _q, _p) \
	if(workq_add((const unsigned char *)_s, strlen(_s) + 1, _q, _p)) { \
		printf("Error adding \"%s\" priority %d to queue %p: %s (%d)\n", _s, _p, _q, strerror(errno), errno); \
		exit(EXIT_FAILURE); \
	} \
	printf("Added \"%s\"\tto queue %p priority %d\n", _s, _q, _p)

const char *one = "One";
const char *two = "Two";
const char *three = "Three";
const char *four = "Four";
const char *five = "Five";
const char *six = "Six";
const char *seven = "Seven";
const char *eight = "Eight";
const char *nine = "Nine";
const char *ten = "Ten";

WorkQ_t work_queue = NULL;

void kill_q(void) {
	workq_destroy(work_queue);
}

int main(void) {
	workq_msg_t msg;
	int x;
	int size = 0;

	work_queue = workq_init(NULL, 0);

	if(work_queue < 0) {
		printf("Failed to initialize a private work queue: error %s (%d)\n", strerror(errno), errno);
		exit(EXIT_FAILURE);
	}

	atexit(kill_q);

	printf("Adding 10 items to the queue in order...\n");

	ADD_OR_DIE(one, work_queue, 1);
	ADD_OR_DIE(two, work_queue, 2);
	ADD_OR_DIE(three, work_queue, 3);
	ADD_OR_DIE(four, work_queue, 4);
	ADD_OR_DIE(five, work_queue, 5);
	ADD_OR_DIE(six, work_queue, 6);
	ADD_OR_DIE(seven, work_queue, 7);
	ADD_OR_DIE(eight, work_queue, 8);
	ADD_OR_DIE(nine, work_queue, 9);
	ADD_OR_DIE(ten, work_queue, 10);

	printf("Removing from queue...\n");
	for(x = 1; x <= 10; ++x) {
		size = workq_get(work_queue, &msg);
		if(size < 0) {
			printf("Error pulling from queue: %s (%d)\n", strerror(errno), errno);
			exit(EXIT_FAILURE);
		} else if(size == 0) {
			printf("No messages left...\n");
		} else {
			printf("Got message \"%s\" priority %ld\n", msg.data, msg.type);
		}
	}

	printf("Adding 10 items to the queue in reverse order...\n");

	ADD_OR_DIE(ten, work_queue, 10);
	ADD_OR_DIE(nine, work_queue, 9);
	ADD_OR_DIE(eight, work_queue, 8);
	ADD_OR_DIE(seven, work_queue, 7);
	ADD_OR_DIE(six, work_queue, 6);
	ADD_OR_DIE(five, work_queue, 5);
	ADD_OR_DIE(four, work_queue, 4);
	ADD_OR_DIE(three, work_queue, 3);
	ADD_OR_DIE(two, work_queue, 2);
	ADD_OR_DIE(one, work_queue, 1);

	printf("Removing from queue...\n");
	for(x = 1; x <= 10; ++x) {
		size = workq_get(work_queue, &msg);
		if(size < 0) {
			printf("Error pulling from queue: %s (%d)\n", strerror(errno), errno);
			exit(EXIT_FAILURE);
		} else if(size == 0) {
			printf("No messages left...\n");
		} else {
			printf("Got message \"%s\" priority %ld\n", msg.data, msg.type);
		}
	}

	printf("Tests passed.\n");

	exit(EXIT_SUCCESS);
}
