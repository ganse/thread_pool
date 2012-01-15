
/*
 * test_threads.h
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

#ifndef THREAD_POOL_H
#define THREAD_POOL_H 1

#include <pthread.h>

#define BOOLEAN unsigned int
#define BOOLEAN_FALSE (1)
#define BOOLEAN_TRUE (!(BOOLEAN_FALSE))

/** Opaque type representing a thread pool. */
typedef void *ThreadPool_t;

/** Thread function type. */
typedef void *(*Thread_t)(void*);

/**
 * @brief Create and return a thread pool object.
 *
 * Spawns threads with the given function.
 *
 * @param num_threads number of threads in the pool
 * @param run_function function for each thread to run
 * @param arg argument to each thread
 *
 * return a thread pool object, or NULL on failure (errno is set)
 */
ThreadPool_t thread_pool_create(int num_threads, Thread_t run_function, void *arg);

/**
 * @brief Set a new thread function.
 *
 * Existing threads will complete, then execute the new function.
 *
 * @param pool the pool object
 * @param thread_function the new thread function
 * @param arg the thread argument
 *
 * return BOOLEAN_TRUE on success, BOOLEAN_FALSE on failure
 */
unsigned int thread_pool_set_function(ThreadPool_t pool, Thread_t thread_function, void *arg);

/**
 * @brief Delete a thread pool object.
 * 
 * Will block until all threads have completed.
 *
 * @param pool the thread pool to delete
 */
void thread_pool_delete(ThreadPool_t pool);

/**
 * @brief Trim a thread pool's size.
 *
 * Threads will exit on completion of the thread pool
 * function until the desired size is reached. The thread
 * pool size will shrink until it reaches the desired size.
 * 
 * Note the pool will not shrink immediately.
 *
 * @param pool the pool to trim
 * @param num_to_cut the number of threads to remove
 */
void thread_pool_trim(ThreadPool_t pool, unsigned int num_to_cut);

/**
 * @brief Add threads to a thread pool.
 * 
 * Immediately spawns new threads to the pool.
 *
 * If arg is NULL the previous value will be used.
 *
 * @param pool the pool to be added to
 * @param num_to_add how many threads to add
 * @param arg the thread function argument
 *
 * return BOOLEAN_TRUE on success, BOOLEAN_FALSE on failure
 */
BOOLEAN thread_pool_add(ThreadPool_t pool, unsigned int num_to_add, void *arg);

/**
 * @brief Get the number of active threads in the pool.
 *
 * This will be greater than or equal to the number of desired threads.
 * The number of threads can vary if the pool is in a shrink operation
 * and has not yet reached the desired size.
 *
 * @param pool the pool to be queried
 *
 * return the number of active threads in the pool
 */
unsigned int thread_pool_get_pool_size(ThreadPool_t pool);

#endif // THREAD_POOL_H
