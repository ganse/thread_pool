
/*
 * thread_pool.c
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
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h> /* for memcpy() */

#include "thread_pool.h"

#ifdef THREAD_POOL_DEBUG

#define THREAD_DEBUG_PRINTF(...) do { fprintf(stderr, "%s:%d :%s(): ThreadPoolDebug: ", __FILE__, __LINE__, __FUNCTION__); fprintf(stderr, __VA_ARGS__); } while(0)

#else /* THREAD_POOL_DEBUG */

#ifdef __GNUC__
inline
#endif /* __GNUC__ */
void no_op(void)
{
}
#define THREAD_DEBUG_PRINTF(...) no_op()

#endif /* THREAD_POOL_DEBUG */

#define THREAD_POOL_MAGIC (0x54687264)
#define THREAD_POOL_MAGIC_DELETED (0x46726565)

/* Internal only pool type. */
typedef struct thread_pool_t
{
   unsigned int magic;
   pthread_mutex_t pool_lock;
   unsigned int desired_threads;
   unsigned int running_threads;
   pthread_t *thread_table;
   Thread_t run_function;
   void *arg;
} thread_pool_t;

/** Internal only thread argument type.
 * Contains the passed argument.
 */
typedef struct pool_thread_arg_t
{
   thread_pool_t *pool;
   /* Room for expansion */
} pool_thread_arg_t;

void *thread_wrap_function(void *arg)
{
   pool_thread_arg_t *thread_arg = (pool_thread_arg_t*)arg;
   void * return_value = NULL;
   void *run_arg;
   Thread_t function;

   while(1) {

      /* Retrieve the thread parameters. */
      pthread_mutex_lock(&thread_arg->pool->pool_lock);
      run_arg = thread_arg->pool->arg;
      function = thread_arg->pool->run_function;
      pthread_mutex_unlock(&thread_arg->pool->pool_lock);

      /* Execute the thread function. */
      return_value = function(run_arg);

      pthread_mutex_lock(&thread_arg->pool->pool_lock);

      if(thread_arg->pool->magic != THREAD_POOL_MAGIC) {
         /* Pool object deleted or corrupted, bail out. */
         pthread_mutex_unlock(&thread_arg->pool->pool_lock);
         pthread_exit(return_value);
      }

      if(thread_arg->pool->running_threads > thread_arg->pool->desired_threads) {
         thread_arg->pool->running_threads--;
         pthread_mutex_unlock(&thread_arg->pool->pool_lock);
         free(thread_arg);
         pthread_exit(return_value);
      }
      pthread_mutex_unlock(&thread_arg->pool->pool_lock);
   }

   THREAD_DEBUG_PRINTF("Unexpected exit from run loop.\n");

   pthread_exit(return_value);
}

unsigned int thread_pool_set_function(ThreadPool_t pool, Thread_t thread_function, void *arg)
{
   thread_pool_t *_pool = (thread_pool_t*)pool;

   pthread_mutex_lock(&_pool->pool_lock);

   if(_pool->magic != THREAD_POOL_MAGIC) {
      pthread_mutex_unlock(&_pool->pool_lock);
      return(BOOLEAN_FALSE);
   }

	_pool->run_function = thread_function;
   pthread_mutex_unlock(&_pool->pool_lock);

   THREAD_DEBUG_PRINTF("Updated run function.\n");

	return(1);
}

/* WARNING: Only called from locked context.
 * Adds threads to pool.
 */
static void _add_threads_from_locked_context(thread_pool_t *_pool)
{
   unsigned int x = _pool->running_threads;
   BOOLEAN *fail_table;
   unsigned int failed = 0;

   /* This shouldn't be needed, since it's static and only called from init and add operations. */
   if(_pool->running_threads < _pool->desired_threads) {
      THREAD_DEBUG_PRINTF("Got called with running threads %u and desired threads %u.\n", _pool->running_threads, _pool->desired_threads);
      return;
   }

   fail_table = calloc(_pool->desired_threads - _pool->running_threads, sizeof(BOOLEAN));

	for( ; x < _pool->desired_threads; ++x) {

      /* The thread function's cleanup frees the argument. */
      pool_thread_arg_t *thread_arg = calloc(1, sizeof(*thread_arg));

      thread_arg->pool = _pool;
		if(!pthread_create(&_pool->thread_table[x], NULL, thread_wrap_function, thread_arg)) {
         _pool->running_threads++;
		} else {
         failed = BOOLEAN_TRUE;
         fail_table[x] = BOOLEAN_TRUE;
      }
	}

   if(failed) {
      pthread_t *thread_table = calloc(_pool->running_threads, sizeof(*_pool->thread_table));
      unsigned int y = 0;

      /* Copy loop without failed threads. Note the y++ inside the loop! */
      for(x = 0; x < _pool->desired_threads; ++x) {
         if(!fail_table[x]) {
            thread_table[y++] = _pool->thread_table[x];
         }
      }
   }
   free(fail_table);
}

ThreadPool_t thread_pool_create(int num_threads, Thread_t run_function, void *arg)
{
   thread_pool_t *_pool = NULL;

	if(!run_function) {
		errno = ENODEV;

      THREAD_DEBUG_PRINTF("No run function was supplied.\n");

		return(0);
	}

   _pool = calloc(1, sizeof(*_pool));
	if(!_pool) {
      /* calloc() sets errno for us */

      THREAD_DEBUG_PRINTF("calloc(): failed.\n");

		return(0);
	}

   _pool->magic = THREAD_POOL_MAGIC;
   _pool->arg = arg;

   _pool->thread_table = calloc(num_threads, sizeof(*_pool->thread_table));
	if(!_pool->thread_table) {
      /* calloc() sets errno for us */

      THREAD_DEBUG_PRINTF("calloc(): failed for thread table.\n");

      free(_pool);
		return(0);
	}


   THREAD_DEBUG_PRINTF("Created the pool object.\n");

   pthread_mutex_init(&_pool->pool_lock, NULL);
	_pool->run_function = run_function;
   _pool->desired_threads = num_threads;

   /* Note that a lock must take place here, since the threads will really be starting and the loop should finish first. */
   pthread_mutex_lock(&_pool->pool_lock);
   _add_threads_from_locked_context(_pool);
   pthread_mutex_unlock(&_pool->pool_lock);

	return(_pool);
}

void thread_pool_delete(ThreadPool_t pool)
{
   unsigned int x;
   unsigned int threads;
   pthread_t *thread_table;
   thread_pool_t *_pool = (ThreadPool_t)pool;

   if(!_pool) {
      return;
   }

   if(_pool->magic != THREAD_POOL_MAGIC) {
      /* Already deleted? Not a thread pool? Bail out. */
      return;
   }

   pthread_mutex_lock(&_pool->pool_lock);
   threads = _pool->running_threads;
   thread_table = calloc(threads, sizeof(*thread_table));
   memcpy(thread_table, _pool->thread_table, sizeof(*thread_table) * threads);

   /* Let the cleanup do its job. */
   _pool->desired_threads = 0;

   pthread_mutex_unlock(&_pool->pool_lock);

   /* Wait for all the threads to exit. */
   for(x = threads; x > 0; --x) {
      pthread_join(thread_table[x], NULL);
   }

   free(thread_table);

   /* Leave thread object in a locked state before free(). */
   pthread_mutex_lock(&_pool->pool_lock);

   /* Set deleted marker. */
   _pool->magic = THREAD_POOL_MAGIC_DELETED;

   _pool->running_threads = 0;
   free(_pool->thread_table);
   free(_pool);
}

void thread_pool_trim(ThreadPool_t pool, unsigned int num_to_cut)
{
   thread_pool_t *_pool = (ThreadPool_t)pool;
   pthread_mutex_lock(&_pool->pool_lock);

   if(_pool->magic != THREAD_POOL_MAGIC) {
      pthread_mutex_unlock(&_pool->pool_lock);
      return;
   }

   /* Safety: Only trim if the size can handle it, otherwise, drop pool to zero. */
   if(_pool->desired_threads >= num_to_cut) {
      _pool->desired_threads -= num_to_cut;
   } else {
      /* Not really sure why you'd drop all threads and not delete the pool object,
       * but just in case the operation is supported.
       */
      _pool->desired_threads = 0;
   }

   pthread_mutex_unlock(&_pool->pool_lock);
}

BOOLEAN thread_pool_add(ThreadPool_t pool, unsigned int num_to_add, void *arg)
{
   thread_pool_t *_pool = (ThreadPool_t)pool;
   pthread_t *thread_table;

   pthread_mutex_lock(&_pool->pool_lock);

   if(_pool->magic != THREAD_POOL_MAGIC) {
      pthread_mutex_unlock(&_pool->pool_lock);
      return(BOOLEAN_FALSE);
   }

   if(arg) {
      _pool->arg = arg;
   }

   thread_table = realloc(_pool->thread_table, (_pool->desired_threads * sizeof(*_pool->thread_table)));

   if(!thread_table) {
      pthread_mutex_unlock(&_pool->pool_lock);
      return(BOOLEAN_FALSE);
   }

   free(_pool->thread_table);
   _pool->thread_table = thread_table;
   _pool->desired_threads = _pool->running_threads + num_to_add;
   _add_threads_from_locked_context(_pool);
   pthread_mutex_unlock(&_pool->pool_lock);

   return(BOOLEAN_TRUE);
}

unsigned int thread_pool_get_pool_size(ThreadPool_t pool)
{
   unsigned int size = 0;
   thread_pool_t *_pool = (ThreadPool_t)pool;

   pthread_mutex_lock(&_pool->pool_lock);
   if(_pool->magic == THREAD_POOL_MAGIC) {
      size = _pool->running_threads;
   }
   pthread_mutex_unlock(&_pool->pool_lock);

   return(size);
}
