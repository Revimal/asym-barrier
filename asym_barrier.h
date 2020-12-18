/* SPDX-License-Identifier:	MIT */

/**
 * @file asym_barrier.h
 * @brief The 'asym-barrier' library.
 *
 * @author Hyeonho Seo a.k.a Revimal (seohho@gmail.com)
 */

#ifndef ASYM_BARRIER_H
#define ASYM_BARRIER_H

#include <stdint.h>
#include <stdatomic.h>

#define ASYM_BARRIER_CACHE_SIZE 64

#if defined(_MSC_VER)
	#define ASYM_BARRIER_COMP_LIKELY(x)
	#define ASYM_BARRIER_COMP_UNLIKELY(x)
	#define ASYM_BARRIER_ARCH_RELAX() YieldProcessor()
	#define ASYM_BARRIER_CACHE_ALIGNED __declspec(align(ASYM_BARRIER_CACHE_SIZE))
#elif (defined(__GNUC__) || defined(__clang__))
	#if defined(__GNUC__)
		#define ASYM_BARRIER_COMP_LIKELY(x) __builtin_expect(!!(x), 1)
		#define ASYM_BARRIER_COMP_UNLIKELY(x) __builtin_expect(!!(x), 0)
	#else
		#define ASYM_BARRIER_COMP_LIKELY(x)
		#define ASYM_BARRIER_COMP_UNLIKELY(x)
	#endif
	#if (defined(ASYM_BARRIER_ARCH_X86) || defined(ASYM_BARRIER_ARCH_MIPS))
		#define ASYM_BARRIER_ARCH_RELAX() __asm__ __volatile__ ("pause" ::: "memory")
	#elif defined(ASYM_BARRIER_ARCH_ARM)
		#define ASYM_BARRIER_ARCH_RELAX() __asm__ __volatile__ ("yield" ::: "memory")
	#elif defined(ASYM_BARRIER_ARCH_PPC)
		#define ASYM_BARRIER_ARCH_RELAX() __asm__ __volatile__ ("or 27,27,27" ::: "memory")
	#else
		#define ASYM_BARRIER_ARCH_RELAX() __asm__ __volatile__ ("")
	#endif
	#define ASYM_BARRIER_CACHE_ALIGNED __attribute__((aligned(ASYM_BARRIER_CACHE_SIZE)))
#else
	#define ASYM_BARRIER_COMP_LIKELY(x)
	#define ASYM_BARRIER_COMP_UNLIKELY(x)
	#define ASYM_BARRIER_ARCH_RELAX() ((void)0) /* HEY, COMPILER. DO NOT OPTIMIZE THIS! */
	#define ASYM_BARRIER_CACHE_ALIGNED
#endif

struct asym_barrier_obj
{
	uint64_t refcnt;
	uint64_t wcount;
	uint64_t synced;
} ASYM_BARRIER_CACHE_ALIGNED;

typedef struct asym_barrier_obj ASYM_BARRIER_CACHE_ALIGNED asym_barrier_t;

/**
 * @brief Initialize an asymmetric barrier object.
 *
 * @param asymb
 *	The pointer to an asymmetric barrier object.
 * @param waiters
 *	The number of waiter threads.
 */
static inline void asym_barrier_init(asym_barrier_t *asymb, uint64_t waiters)
{
	if (ASYM_BARRIER_COMP_UNLIKELY(!asymb))
		return;

	atomic_store_explicit(&asymb->refcnt, waiters, memory_order_relaxed);
	atomic_store_explicit(&asymb->wcount, 0, memory_order_relaxed);
	atomic_store_explicit(&asymb->synced, 0, memory_order_relaxed);

	return;
}

/**
 * @brief Update the period of an asymmetric barrier object.
 *
 * @note
 *	@b UPDATER-API: This function must be called in the update thread.
 *
 * @param asymb
 *	The pointer to an asymmetric barrier object.
 * @param synced
 *	If 0, the updater thread does not wait for 'the period has recognized'.
 */
static inline void asym_barrier_update(asym_barrier_t *asymb, unsigned synced)
{
	if (ASYM_BARRIER_COMP_UNLIKELY(!asymb))
		return;

	atomic_store_explicit(&asymb->wcount, asymb->refcnt, memory_order_seq_cst);

	while (!!synced && !!atomic_load_explicit(&asymb->wcount, memory_order_relaxed))
		ASYM_BARRIER_ARCH_RELAX();

	return;
}

/**
 * @brief Commit the period of an asymmetric barrier object.
 *
 * @note
 *	@b UPDATER-API: This function must be called in the update thread.
 *
 * @param asymb
 *	The pointer to an asymmetric barrier object.
 */
static inline void asym_barrier_commit(asym_barrier_t *asymb)
{
	if (ASYM_BARRIER_COMP_UNLIKELY(!asymb))
		return;

	while (atomic_load_explicit(&asymb->synced, memory_order_relaxed) !=
			atomic_load_explicit(&asymb->refcnt, memory_order_relaxed))
		ASYM_BARRIER_ARCH_RELAX();

	atomic_store_explicit(&asymb->synced, 0, memory_order_seq_cst);

	return;
}

/**
 * @brief Check the period of an asymmetric barrier object.
 *
 * @note
 * 	@b WAITER-API: This function must be called in a waiter thread.
 *
 * @param asymb
 *	The pointer to an asymmetric barrier object.
 */
static inline void asym_barrier_check(asym_barrier_t *asymb)
{
	if (ASYM_BARRIER_COMP_UNLIKELY(!asymb))
		return;

	if (ASYM_BARRIER_COMP_UNLIKELY(
				atomic_load_explicit(&asymb->wcount, memory_order_relaxed)))
	{
		(void)atomic_fetch_sub_explicit(&asymb->wcount, 1, memory_order_acquire);

		while (!!atomic_load_explicit(&asymb->wcount, memory_order_relaxed))
			ASYM_BARRIER_ARCH_RELAX();

		(void)atomic_fetch_add_explicit(&asymb->synced, 1, memory_order_acq_rel);

		while (!!atomic_load_explicit(&asymb->synced, memory_order_relaxed))
			ASYM_BARRIER_ARCH_RELAX();
	}

	return;
}

#endif /* ASYM_BARRIER_H */
