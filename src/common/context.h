/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_CONTEXT_H
#define __NX_CONTEXT_H

#include <apr_thread_proc.h>
#include "exception.h"

struct nx_context_t
{
    apr_pool_t			*pool;
    void                        *user_data;
    void                        *thread_data;
    struct exception_context    exception_context;
};
typedef struct nx_context_t nx_context_t;

nx_context_t *nx_get_context();
nx_context_t *nx_init_context();
apr_threadkey_t *nx_get_context_key();


#endif	/* __NX_CONTEXT_H */
