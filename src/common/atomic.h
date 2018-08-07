/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_ATOMIC_H
#define __NX_ATOMIC_H


#ifdef NX_NOATOMIC
#include <apr_thread_mutex.h>

// These macros are mainly used for valgrind/helgrind as it does not 
// support atomic operations and falsely reports this as a race condition
extern apr_thread_mutex_t *nx_atomic_mutex;

#define nx_atomic_lock() apr_thread_mutex_lock(nx_atomic_mutex)
#define nx_atomic_unlock() apr_thread_mutex_unlock(nx_atomic_mutex)
#define nx_atomic_set32(ptr, val) nx_atomic_lock(); *ptr = val; nx_atomic_unlock();

#define nx_atomic_read32(ptr)     \
    ({                            \
         apr_uint32_t _retval;    \
         nx_atomic_lock();        \
         _retval = *ptr;          \
         nx_atomic_unlock();      \
         _retval;                 \
    })
#define nx_atomic_add32(ptr, val) nx_atomic_lock(); *ptr += val; nx_atomic_unlock();
#define nx_atomic_sub32(ptr, val) nx_atomic_lock(); *ptr -= val; nx_atomic_unlock();

#else //NX_NOATOMIC
#include <apr_atomic.h>
#define nx_atomic_lock() /* nx_atomic_lock is a NOP here */
#define nx_atomic_unlock() /* nx_atomic_unlock is a NOP here */
#define nx_atomic_set32(ptr, val) apr_atomic_set32(ptr, val)
#define nx_atomic_read32(ptr) apr_atomic_read32(ptr)
#define nx_atomic_add32(ptr, val) apr_atomic_add32(ptr, val)
#define nx_atomic_sub32(ptr, val) apr_atomic_sub32(ptr, val)
#endif


#endif	/* __NX_ATOMIC_H */
