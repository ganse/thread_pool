
/*
 * workq.h
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

#pragma once

#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H 1

#include <sys/types.h>

#define WORKQ_MAX_SIZE (2048)

#define WORKQ_LOWEST_PRIO (10)

/** Opaque handle to a work queue object. */
typedef void * WorkQ_t;

/** Work queue message object. */
typedef struct {
	long type; /**< Message type, analogous to SysV msgq type. */
	unsigned char data[WORKQ_MAX_SIZE]; /**< Message payload. */
} workq_msg_t;

/**
 * @brief Create and initialize a work queue object.
 *
 * @param keyfile a filename to generate a key from, much like SysV ftok()
 * @param subsystem_id subsystem (for use with multiple queues)
 *
 * return a work queue object
 */
WorkQ_t workq_init(const char *keyfile, int subsystem_id);

/**
 * @brief Clean up a work queue object.
 *
 * @param work_queue the work queue object to destroy
 *
 * return zero on success, something else on error
 */
int workq_destroy(WorkQ_t work_queue);

/**
 * @brief Get a work queue packet.
 * 
 * Note this will retrieve higher priority packets first.
 *
 * @param work_queue the work queue to retrieve from
 * @param msg object to be filled in with the next work packet
 *
 * return the size of the work queue packet
 */
ssize_t workq_get(WorkQ_t work_queue, workq_msg_t *msg);

/**
 * @brief Add a work packet to a work queue.
 *
 * @param buffer the work queue packet
 * @param size the size of the work queue packet
 * @param work_queue the work queue to add to
 * @param prio the priority of the work queue packet
 *
 * return zero on success, anything else is failure
 */
int workq_add(const unsigned char *buffer, size_t size, WorkQ_t work_queue, long prio);

#endif /* WORK_QUEUE_H */
