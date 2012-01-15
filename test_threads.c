
/*
 * test_threads.c
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
#include "thread_pool.h"

#define ADD_OR_DIE(_s, _q, _p) \
	if(workq_add((const unsigned char *)_s, strlen(_s) + 1, _q, _p)) { \
		printf("Error adding \"%s\" priority %d to queue %p: %s (%d)\n", _s, _p, _q, strerror(errno), errno); \
		exit(EXIT_FAILURE); \
	} \
	printf("Added \"%s\"\tto queue %p priority %d\n", _s, _q, _p)

WorkQ_t work_queue;

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
const char *more = "More";

void kill_q(void) {
	workq_destroy(work_queue);
}

void *print_msg(void *arg) {
	workq_msg_t msg;
	printf("Hello from thread %ld\n", pthread_self());
	while(1) {
		printf("Thread %ld trying to get a message...\n", pthread_self());
		workq_get(work_queue, &msg);
		printf("Thread %ld: Got message \"%s\" priority %ld\n", pthread_self(), msg.data, msg.type);
	}
	return(NULL);
}

int main(void) {
	int num_threads = 1;
	int x;
   ThreadPool_t pool;
	work_queue = workq_init(NULL, 0);

	if(!work_queue) {
		printf("Can't initialize work queue.\n");
		exit(EXIT_FAILURE);
	}

	atexit(kill_q);

	printf("Preloading queue...\n");
	ADD_OR_DIE(one,   work_queue, 1);
	ADD_OR_DIE(two,   work_queue, 2);
	ADD_OR_DIE(three, work_queue, 3);
	ADD_OR_DIE(four,  work_queue, 4);
	ADD_OR_DIE(five,  work_queue, 5);
	ADD_OR_DIE(six,   work_queue, 6);
	ADD_OR_DIE(seven, work_queue, 7);
	ADD_OR_DIE(eight, work_queue, 8);
	ADD_OR_DIE(nine,  work_queue, 9);
	ADD_OR_DIE(ten,   work_queue, 10);

	printf("Kickstarting thread pool (%d threads)...\n", num_threads);
	pool = thread_pool_create(num_threads, print_msg, NULL);

   if(!pool) {
      printf("Pool could not be created: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

	printf("main(): adding more...\n");
	for(x = 0; x < 1000; ++x) {
		ADD_OR_DIE(more, work_queue, 10);
	}

	printf("Now waiting for output.\n");

	for(x = 0; x < 10000000; ++x) {
		sched_yield();
	}

   printf("Waiting on thread pool to die.\n");
   thread_pool_delete(pool);

	printf("Waited for them to finish, now we're done.\n");

	exit(EXIT_SUCCESS);
}
