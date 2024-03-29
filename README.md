<!-- SPDX-License-Identifier:	MIT -->

# asym-barrier
[![Build Status](https://travis-ci.com/Revimal/asym-barrier.svg?branch=master)](https://travis-ci.com/Revimal/asym-barrier)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C11](https://img.shields.io/badge/Language-C11-blue.svg)](https://www.iso.org/standard/57853.html)
[![Header-only library](https://img.shields.io/badge/Header--only_library-green.svg)](https://en.wikipedia.org/wiki/Header-only)

## What is the 'asym-barrier'?
A simple header-only library that can replace the traditional reader-writer lock for the exceptional case when only one single writer thread exists.

Focused on reducing reader-side overheads; get the reader-lock can be replaced with single memory access to an atomic variable with the relaxed memory ordering.
On the other hand, its performance may be worse than the traditional reader-writer lock when in environments with frequent modifications.

I hope it helps people who have the same concern about reducing the reader-lock acquirement cost under high contention.

---

## Where can I use it?
* Synchronize per-core data to a modification that needs global visibility.
* Guarantee the MT-safety by pending all threads during a critical subroutine.
* Emulate the SPMC behavior on a data structure which not considered for the multiprocessing.
* Implement the thread barrier (Reference the example code: 'symm_barrier.h').
* And so on...

---

## How does it works?
1. All actions of the 'asym-barrier' are triggered by calling the 'update-function' in the updater thread; we call this 'the period has updated'.
2. If 'the period has updated', the updater increases the 'waiting-count' as the number of waiter threads.
3. The updater thread spins until the 'waiting-count' being 0.
4. Each waiter threads that confront a 'barrier-point' after the updater thread increased the 'waiting-count' decreases the 'waiting-count' by 1.
5. All waiter threads that performed the increment spins until the 'waiting-count' being 0.
6. When all waiter threads met their 'barrier-point', so the 'waiting-count' being 0, all threads are escaped from spinning; we call this 'the period has recognized'.
7. The updater thread continues its series of processing until the first 'commit-point' after the place where the 'update-function' is invoked.
8. Concurrently, each waiter threads increase the 'synced-count' by 1, then spinning until the 'synced-count' being 0.
9. The updater thread spins until the 'synced-count' equals the number of waiter threads when it reaches a 'commit-point'.
10. If the updater thread satisfied the condition to break the spinning, it sets the 'synced-count' as the 0.
11. The increment action of the 'statement 8' makes the updater thread advance from 'statement 9' to 'statement 10'.
12. The zeroing action of 'statement 10' makes the waiter thread escapes from spinning scribed in 'statement 8'.
13. By the above two interactions, all the threads are escaped from spinning.
14. At this point, both the 'waiting-count' and 'synced-count' is restored as 0, and all threads start continuing their control flow; we call this 'the period has committed'.

In the end, we can consider all threads have synchronized to modifications made by the updater thread during the time between 'the period has recognized' and 'the period has committed'.

#### Legend
* 'asym-barrier': 'asym_barrier_t'
* 'waiting-count': 'struct asym_barrier_obj :: uint64_t wcount'
* 'synced-count': 'struct asym_barrier_obj :: uint64_t synced'
* 'update-function': 'asym_barrier_update()'
* 'commit-point': 'asym_barrier_commit()'
* 'barrier-point': 'asym_barrier_check()'

---

## How to use it?
#### Main Function Example
```c
#include "asym_barrier.h"

#define WAITER_THREAD_NUM 4

int main(int argc, char *argv[])
{
	asym_barrier_t abobj;

	asym_barrier_init(&abobj, WAITER_THREAD_NUM);

	/* spawn threads... */

	return 0;
}
```

#### Updater Function Example
```c
#include "asym_barrier.h"

void updater_func(void *arg)
{
	asym_barrier_t *abptr = arg;

	while (1) {
		/* synchornized behavior (refer comments on 'asym_barrier_update()') */
		asym_barrier_update(abptr, 1);
		/* do somthing... */
		asym_barrier_commit(abptr);
	}

	return;
}
```

#### Waiter Function Example
```c
#include "asym_barrier.h"

void waiter_func(void *arg)
{
	asym_barrier_t *abprt = arg;

	while (1) {
		asym_barrier_check(abptr);
		/* do something... */
	}
}
```

---

## How many architectures are supported?
This library uses a C11 standard feature named 'ISO/IEC 9899:2011 Atomics' so most architectures are supported.

**DISCLAIMER: Never tested on 32-bit environments.**

#### List of architectures that support efficient CPU relaxing
* x86 (since Pentium4): define `ASYM_BARRIER_ARCH_X86` before you include the library header.
* ARM (since ARMv6K): define `ASYM_BARRIER_ARCH_ARM` before you include the library header.
* PPC: define `ASYM_BARRIER_ARCH_PPC` before you include the library header.
* MIPS: define `ASYM_BARRIER_ARCH_MIPS` before you include the library header.

---

## Is it works well?
If you work on a Linux distribution, you can test this library easily by executing the shell script: 'autotest-linux.sh'.

#### Benchmark result compared to existing methods
The following results were measured with the customized version of [rwl-bench](https://github.com/bloomberg/rwl-bench).

![Benchmark Result Plot](PERFORMANCE.png)

* CPU: Intel(R) Xeon(R) CPU E5-2680 v4 (14C, 28T, 2.40 GHz, 35MB Cache) x2 NUMA
* RAM: DDR-4 2400MT/s 8192MB x16
* SYS: Linux 4.15.0-101-generic (Ubuntu 18.04.4 LTS, AMD64, SMP)

---

## Can I contribute to this?
#### Did you find a bug?
* **Be sure the bug was not already reported on [Issues](https://github.com/Revimal/asym-barrier/issues).**
* If the issue seems not duplicated, **open a new one from [Here](https://github.com/Revimal/asym-barrier/issues/new).**
* Attaching additional information such as core-dump makes maintainers happier.

#### Did you write a patch?
* **Open a new [Pull Request](https://github.com/Revimal/asym-barrier/pulls).**

#### Are there any rewards for contributors?
* When we meet someday, I'll buy you a pint of chilling beer.
