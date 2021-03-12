/* SPDX-License-Identifier:	MIT */

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <pthread.h>

#include "asym_barrier.h"

#define TEST_BARRIER_THREADS 3
#define TEST_BARRIER_TESTCNT 10000000

static unsigned unsafe_exit_flag;
static size_t unsafe_update_count;
static size_t unsafe_result_count;

static unsigned synced_exit_flag;
static size_t synced_update_count;
static size_t synced_result_count;

static void * unsafe_update_fn(__attribute__((unused)) void *arg)
{
	while (unsafe_update_count < TEST_BARRIER_TESTCNT)
	{
		ASYM_BARRIER_ARCH_RELAX();
		unsafe_update_count++;
		ASYM_BARRIER_ARCH_RELAX();
	}

	unsafe_exit_flag = 1;

	return NULL;
}

static void * unsafe_wait_fn(__attribute__((unused)) void *arg)
{
	uint64_t local_count = 0;

	while (!unsafe_exit_flag)
	{
		ASYM_BARRIER_ARCH_RELAX();
		if (local_count < unsafe_update_count)
		{
			local_count++;
			ASYM_BARRIER_ARCH_RELAX();
			if (local_count < (unsafe_update_count - 1))
			{
				local_count++;
				(void)atomic_fetch_add_explicit(
						&unsafe_result_count, 1, memory_order_seq_cst);
			}
		}
	}

	return NULL;
}

static void * synced_update_fn(void *arg)
{
	asym_barrier_t *barrier = (asym_barrier_t *)arg;

	while (synced_update_count < TEST_BARRIER_TESTCNT)
	{
		asym_barrier_update(barrier, 1);
		synced_update_count++;
		asym_barrier_commit(barrier);
	}

	synced_exit_flag = 1;

	return NULL;
}

static void * synced_wait_fn(void *arg)
{
	asym_barrier_t *barrier = (asym_barrier_t *)arg;
	uint64_t local_count = 0;

	while (!synced_exit_flag)
	{
		asym_barrier_check(barrier);
		if (local_count < synced_update_count)
		{
			local_count++;
			ASYM_BARRIER_ARCH_RELAX();
			if (local_count < (synced_update_count - 1))
			{
				local_count++;
				(void)atomic_fetch_add_explicit(
						&synced_result_count, 1, memory_order_seq_cst);
			}
		}
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t unsafe_updater;
	pthread_t unsafe_waiters[TEST_BARRIER_THREADS];

	pthread_t synced_updater;
	pthread_t synced_waiters[TEST_BARRIER_THREADS];
	asym_barrier_t asym_barrier;

	size_t thread_idx;
	void *tmp_var;

	if (pthread_create(&unsafe_updater, NULL, &unsafe_update_fn, NULL) < 0)
	{
		perror("Unsafe updater create:");
		exit(-1);
	}

	for (thread_idx = 0; thread_idx < TEST_BARRIER_THREADS; ++thread_idx)
	{
		if (pthread_create(&unsafe_waiters[thread_idx],
					NULL, &unsafe_wait_fn, NULL) < 0)
		{
			perror("Unsafe waiter create:");
			exit(-1);
		}
	}

	if (pthread_join(unsafe_updater, &tmp_var) < 0)
	{
		perror("Unsafe updater join:");
		exit(-1);
	}

	for (thread_idx = 0; thread_idx < TEST_BARRIER_THREADS; ++thread_idx)
	{
		if (pthread_join(unsafe_waiters[thread_idx], &tmp_var) < 0)
		{
			perror("Unsafe waiter join:");
			exit(-1);
		}
	}

	asym_barrier_init(&asym_barrier, TEST_BARRIER_THREADS);

	if (pthread_create(&synced_updater, NULL, &synced_update_fn, &asym_barrier) < 0)
	{
		perror("Synced updater create:");
		exit(-1);
	}

	for (thread_idx = 0; thread_idx < TEST_BARRIER_THREADS; ++thread_idx)
	{
		if (pthread_create(&synced_waiters[thread_idx],
					NULL, &synced_wait_fn, &asym_barrier) < 0)
		{
			perror("Synced waiter create:");
			exit(-1);
		}
	}

	if (pthread_join(synced_updater, &tmp_var) < 0)
	{
		perror("Synced updater join:");
		exit(-1);
	}

	for (thread_idx = 0; thread_idx < TEST_BARRIER_THREADS; ++thread_idx)
	{
		if (pthread_join(synced_waiters[thread_idx], &tmp_var) < 0)
		{
			perror("Synced waiter join:");
			exit(-1);
		}
	}

	printf("RESULT: %20s/%20s/%20s\n",
			"FAILED", "TESTED", "TSTNUM");
	printf("UNSAFE: %20zu/%20zu/%20zu\n",
			unsafe_result_count,
			(unsafe_update_count * TEST_BARRIER_THREADS),
			(size_t)(TEST_BARRIER_TESTCNT * TEST_BARRIER_THREADS));
	printf("SYNCED: %20zu/%20zu/%20zu\n",
			synced_result_count,
			(synced_update_count * TEST_BARRIER_THREADS),
			(size_t)(TEST_BARRIER_TESTCNT * TEST_BARRIER_THREADS));

	return 0;
}
