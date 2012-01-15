
/*
 * workq.c
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

/*
 * You'll notice this is pretty much just a wrapper around
 * SysV message queues. This is deliberate, as they already work
 * pretty much the way I want them to work, especially the priority
 * part, which I think is nifty.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "workq.h"

#define WORKQ_MAGIC (0x57726b51)

typedef struct wq_t {
	uint32_t magic;
	int id;
	key_t key;
	pthread_mutex_t mutex;
	pthread_mutex_t send_mutex;
} wq_t;

WorkQ_t workq_init(const char *keyfile, int subsystem_id) {
	wq_t *q;

	q = calloc(1, sizeof(wq_t));
	if(!q) {
		return(NULL);
	}

	q->magic = WORKQ_MAGIC;

	pthread_mutex_init(&(q->mutex), NULL);
	pthread_mutex_init(&(q->send_mutex), NULL);

	if(keyfile) {
		q->key = ftok(keyfile, subsystem_id);
	} else {
		q->key = IPC_PRIVATE;
	}

	q->id = msgget(q->key, 0660);

	/* Try to attach to existing queue first */
	if(q->id > 0) {
		return((WorkQ_t)q);
	}

	/* Try to create a queue */
	if(msgget(q->key, IPC_CREAT | 0660) > 0) {
		return((WorkQ_t)q);
	}

	q->magic = 0;
	free(q);
	return(NULL);
}

int workq_destroy(WorkQ_t work_queue) {
	int rv = -1;
	wq_t *q = (wq_t*)work_queue;

	if(!q || q->magic != WORKQ_MAGIC) {
		errno = ENODEV;
		return(-1);
	}

	rv = msgctl(q->id, IPC_RMID, NULL);
	q->magic = 0;
	pthread_mutex_destroy(&(q->mutex));

	return(rv);
}

ssize_t workq_get(WorkQ_t work_queue, workq_msg_t *msg) {
	long x;
	int rcv_size = 0;
	wq_t *q = (wq_t*)work_queue;

	if(!q || q->magic != WORKQ_MAGIC) {
		errno = ENODEV;
		return(-1);
	}

	memset(msg, 0, sizeof(msg));

	pthread_mutex_lock(&(q->mutex));

	/* Get higher priority messages first */
	for(x = 1L; x <= WORKQ_LOWEST_PRIO; ++x) {
		if((rcv_size = msgrcv(q->id, msg, sizeof(*msg), x, IPC_NOWAIT)) > 0) {
			return(rcv_size);
		}
	}

	/* Wait for next message */
	rcv_size = msgrcv(q->id, msg, sizeof(*msg), x, 0);

	pthread_mutex_unlock(&(q->mutex));

	return(rcv_size);
}

/* TODO: Change to zero copy */
int workq_add(const unsigned char *buffer, size_t size, WorkQ_t work_queue, long prio) {
	int rv;
	workq_msg_t msg;
	wq_t *q = (wq_t*)work_queue;

	if(!q || q->magic != WORKQ_MAGIC) {
		errno = ENODEV;
		return(-1);
	}

	if(size > WORKQ_MAX_SIZE) {
		errno = ENOSPC;
		return(-1);
	}

	memset(&msg, 0, sizeof(msg));

	msg.type = prio;
	memcpy(msg.data, buffer, size);

	pthread_mutex_lock(&(q->send_mutex));

	rv = msgsnd(q->id, &msg, size + sizeof(msg.type), 0);

	pthread_mutex_unlock(&(q->send_mutex));

	return(rv);
}
