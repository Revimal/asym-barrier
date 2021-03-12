/* SPDX-License-Identifier: MIT */

/**
 * @file symm_barrier.h
 * @brief The symmetric barrier
 *	which built with the 'asym-barrier' library.
 *
 * It was implemented by the mixing of the
 * 'asym-barrier' library and the 'ticket-lock' approach.
 *
 * This is not efficient compared to existing methods such as the 'pthread_barrier',
 * but maybe an example case showing how to use the 'asym-barrier'.
 *
 * It treats the updater and waiter equally by internally calls the check function
 * instead of the CPU spinning of the 'ticket-lock' algorithm.
 *
 * @author Hyeonho Seo a.k.a Revimal (seohho@gmail.com)
 */

#ifndef SYMM_BARRIER_H
#define SYMM_BARRIER_H

#include "asym_barrier.h"

struct symm_barrier_obj
{
	uint64_t workers;
	uint64_t waiting;
	uint64_t serving;
	asym_barrier_t asymb;
} ASYM_BARRIER_CACHE_ALIGNED;

typedef struct symm_barrier_obj ASYM_BARRIER_CACHE_ALIGNED symm_barrier_t;

/**
 * @brief Initialize a symmetric barrier object.
 *
 * @param symmb
 *	The pointer to a symmetric barrier object.
 * @param workters
 *	The number of total threads.
 */
static inline void symm_barrier_init(symm_barrier_t *symmb, uint64_t workers)
{
	if (ASYM_BARRIER_COMP_UNLIKELY(!symmb))
		return;

	atomic_store_explicit(&symmb->workers, workers, memory_order_relaxed);
	atomic_store_explicit(&symmb->waiting, 0, memory_order_relaxed);
	atomic_store_explicit(&symmb->serving, 0, memory_order_relaxed);

	if (ASYM_BARRIER_COMP_UNLIKELY(symmb->workers > 1))
		asym_barrier_init(&symmb->asymb, (symmb->workers - 1));

	return;
}

/**
 * @brief Update the period of a symmetric barrier object.
 *
 * @param symmb
 *	The pointer to a symmetric barrier object.
 *
 * @param synced
 *	If 0, the caller thread does not wait for others.
 */
static inline void symm_barrier_update(symm_barrier_t *symmb, unsigned synced)
{
	if (ASYM_BARRIER_COMP_LIKELY(!!symmb && symmb->workers > 1))
	{
		register uint64_t expect = atomic_fetch_add_explicit(
				&symmb->waiting, 1, memory_order_acquire);
		while (expect != atomic_load_explicit(&symmb->serving, memory_order_relaxed))
			asym_barrier_check(&symmb->asymb);

		asym_barrier_update(&symmb->asymb, synced);
	}

	return;
}

/**
 * @brief Commit the period of a symmetric barrier object.
 *
 * @param symmb
 *	The pointer to a symmetric barrier object.
 */
static inline void symm_barrier_commit(symm_barrier_t *symmb)
{
	if (ASYM_BARRIER_COMP_LIKELY(!!symmb && symmb->workers > 1))
	{
		asym_barrier_commit(&symmb->asymb);
		(void)atomic_fetch_add_explicit(&symmb->serving, 1, memory_order_release);
	}

	return;
}

/**
 * @brief Check the period of a symmetric barrier object.
 *
 * @param symmb
 *	The pointer to a symmetric barrier object.
 */
static inline void symm_barrier_check(symm_barrier_t *symmb)
{
	if (ASYM_BARRIER_COMP_LIKELY(!!symmb && symmb->workers > 1))
		asym_barrier_check(&symmb->asymb);

	return;
}

#endif /* SYMM_BARRIER_H */
