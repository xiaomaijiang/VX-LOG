/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_PM_BUFFER_H
#define __NX_PM_BUFFER_H

#include "../../../common/types.h"
#include "../../../common/logqueue.h"

#define NX_PM_BUFFER_DEFAULT_CHUNK_SIZE_LIMIT 10000

typedef enum nx_pm_buffer_type
{
    NX_PM_BUFFER_TYPE_MEM = 1,
    NX_PM_BUFFER_TYPE_DISK,
} nx_pm_buffer_type;

typedef struct nx_pm_buffer_conf_t
{
    uint64_t buffer_size;
    uint64_t buffer_maxsize;
    uint64_t buffer_warnlimit;
    nx_logqueue_t *queue;
    boolean warned;
    boolean warned_full;
    nx_pm_buffer_type type;
    // struct members for disk based buffer (chunked files)
    const char		*basedir;	///< directory to save chunk files under
    int			size;		///< number of elements in the queue
    apr_file_t 		*push_file;	///< current open logqueue chunk file which we write to
    int			push_count;	///< number of logdata elements in the current file chunk
    int64_t		push_id;	///< current chunk id
    int			push_limit;	///< maximum number of logdata elements in a chunk
    apr_file_t 		*pop_file;	///< current open logqueue chunk file which we read from
    int64_t		pop_id;		///< current chunk id
    apr_off_t		pop_pos;	///< current position
    int			pop_count;	///< number of logdata elements read from the current file chunk
} nx_pm_buffer_conf_t;



#endif	/* __NX_PM_BUFFER_H */
