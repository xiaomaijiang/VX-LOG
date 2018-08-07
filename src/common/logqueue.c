/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "error_debug.h"
#include "logqueue.h"
#include "serialize.h"
#include "exception.h"
#include "alloc.h"
#include "module.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


nx_logqueue_t *nx_logqueue_new(apr_pool_t *pool,
			       const char *name)
{
    nx_logqueue_t *logqueue;

    ASSERT(pool != NULL);
    ASSERT(name != NULL);

    logqueue = apr_pcalloc(pool, sizeof(nx_logqueue_t));
    logqueue->pool = pool;
    logqueue->name = apr_pstrdup(pool, name);

    CHECKERR(apr_thread_mutex_create(&(logqueue->mutex), APR_THREAD_MUTEX_UNNESTED, pool));
    
    logqueue->list = apr_pcalloc(pool, sizeof(nx_logqueue_list_t));
    logqueue->size = 0;

    return ( logqueue );
}



void nx_logqueue_init(nx_logqueue_t *logqueue)
{
    ASSERT(logqueue != NULL);

    logqueue->limit = NX_LOGQUEUE_LIMIT;
    logqueue->basedir = apr_pstrdup(logqueue->pool, nx_module_get_cachedir());
}



int nx_logqueue_push(nx_logqueue_t *logqueue, nx_logdata_t *logdata)
{
    int retval = 0;

    ASSERT(logqueue != NULL);
    ASSERT(logdata != NULL);

    {
	CHECKERR(apr_thread_mutex_lock(logqueue->mutex));
    
	ASSERT(logdata->link.prev == NULL);
	ASSERT(logdata->link.next == NULL);
	log_debug("before nx_logqueue_push, size: %d", logqueue->size);
	ASSERT(logqueue->size >= 0);
	NX_DLIST_INSERT_TAIL(logqueue->list, logdata, link);
	(logqueue->size)++;
	retval = logqueue->size;

	CHECKERR(apr_thread_mutex_unlock(logqueue->mutex));
    }

    return ( retval );
}



/**
 * Get the first element from the queue without actually removing it.
 * Returns the size of the queue excluding the current element.
 */

int nx_logqueue_peek(nx_logqueue_t *logqueue, nx_logdata_t **logdata)
{
    nx_logdata_t *ld = NULL;
    int size = 0;

    ASSERT(logqueue != NULL);
    *logdata = NULL;
    
//    log_debug("before nx_logqueue_peek, size: %d", logqueue->size);
    CHECKERR(apr_thread_mutex_lock(logqueue->mutex));
    ASSERT(logqueue->size >= 0);
    {
	if ( logqueue->size > 0 )
	{
	    ld = NX_DLIST_FIRST(logqueue->list);
	    ASSERT(ld != NULL);
	    logqueue->needpop = TRUE;
	    size = logqueue->size - 1;
	}
	else
	{
	    ASSERT(NX_DLIST_EMPTY(logqueue->list) == TRUE);
	}

	*logdata = ld;
    }
    CHECKERR(apr_thread_mutex_unlock(logqueue->mutex));

    return ( size );
}



/**
 * Removes the logqueue from the queue.
 * Must be called after nx_logqueue_peek()
 * Returns the size of the queue after the element was removed.
 */

int nx_logqueue_pop(nx_logqueue_t *logqueue, nx_logdata_t *logdata)
{
    nx_logdata_t *ld = NULL;
    int size = 0;

    ASSERT(logqueue != NULL);

    CHECKERR(apr_thread_mutex_lock(logqueue->mutex));
    log_debug("before nx_logqueue_pop, size: %d", logqueue->size);

    ASSERT(logqueue->needpop == TRUE);
    ASSERT(logqueue->size >= 0);
    {
	ld = NX_DLIST_FIRST(logqueue->list);
	ASSERT(ld != NULL);
	ASSERT(ld == logdata);
	NX_DLIST_REMOVE(logqueue->list, ld, link);
	(logqueue->size)--;
	logqueue->needpop = FALSE;
    }
    size = logqueue->size;
    CHECKERR(apr_thread_mutex_unlock(logqueue->mutex));

    return ( size );
}



void nx_logqueue_lock(nx_logqueue_t *logqueue)
{
    ASSERT(logqueue != NULL);

    CHECKERR(apr_thread_mutex_lock(logqueue->mutex));
}



void nx_logqueue_unlock(nx_logqueue_t *logqueue)
{
    ASSERT(logqueue != NULL);

    CHECKERR(apr_thread_mutex_unlock(logqueue->mutex));
}



int nx_logqueue_size(nx_logqueue_t *logqueue)
{
    int queuesize;
    ASSERT(logqueue != NULL);

    // TODO: make logqueue->size atomic
    CHECKERR(apr_thread_mutex_lock(logqueue->mutex));
    queuesize = logqueue->size;
    CHECKERR(apr_thread_mutex_unlock(logqueue->mutex));
    
    return ( queuesize );
}



apr_size_t nx_logqueue_to_file(nx_logqueue_t *logqueue)
{
    apr_file_t *file = NULL;
    apr_size_t retval = 0;
    nx_logdata_t *logdata;
    apr_uint32_t size32;
    char *buf = NULL;
    nx_exception_t e;
    char filename[APR_PATH_MAX];

    ASSERT(logqueue != NULL);

    if ( logqueue->size == 0 )
    { // queue empty, don't save anything
	return ( 0 );
    }

    if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s.q",
		      logqueue->basedir, logqueue->name) == sizeof(filename) )
    {
	throw_msg("logqueue filename length limit exceeeded");
    }

    CHECKERR_MSG(apr_file_open(&file, filename, APR_WRITE | APR_CREATE | APR_TRUNCATE,
			       APR_OS_DEFAULT, logqueue->pool),
		 "couldn't open logqueue file %s for writing", filename);

    try
    {
	for ( logdata = NX_DLIST_FIRST(logqueue->list);
	      logdata != NULL;
	      logdata = NX_DLIST_NEXT(logdata, link) )
	{
	    apr_size_t memsize;
	    memsize = nx_logdata_serialized_size(logdata);
	
	    buf = malloc(memsize + 4);
	    size32 = (apr_uint32_t) memsize;
	    nx_int32_to_le(buf, &size32);
	    ASSERT(nx_logdata_to_membuf(logdata, buf + 4, memsize) == memsize);
	    memsize += 4;
	    CHECKERR_MSG(apr_file_write(file, buf, &memsize),
		     "failed to write logqueue to file");
	    retval += memsize;
	    free(buf);
	    buf = NULL;
	}
    }
    catch(e)
    {
	if ( buf != NULL )
	{
	    free(buf);
	}
	if ( file != NULL )
	{
	    apr_file_close(file);
	}
	rethrow(e);
    }

    if ( buf != NULL )
    {
	free(buf);
    }
    if ( file != NULL )
    {
	apr_file_close(file);
    }

    return ( retval );
}



apr_size_t nx_logqueue_from_file(nx_logqueue_t *logqueue)
{
    apr_status_t rv;
    nx_logdata_t * volatile logdata = NULL;
    char *buf = NULL;
    apr_size_t bytes, bytesread;
    char sizebuf[4];
    nx_exception_t e;
    apr_size_t totalread = 0;
    char filename[APR_PATH_MAX];
    apr_file_t *file = NULL;

    ASSERT(logqueue != NULL);

    if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s.q",
		      logqueue->basedir, logqueue->name) == sizeof(filename) )
    {
	throw_msg("logqueue filename length limit exceeeded");
    }

    if ( (rv = apr_file_open(&file, filename, APR_READ,
			     APR_OS_DEFAULT, logqueue->pool)) != APR_SUCCESS )
    {
	if ( APR_STATUS_IS_ENOENT(rv) )
	{ // queuefile doesn't exist, nothing to do
	    return ( totalread );
	}
	throw(rv, "module %s couldn't open logqueue file %s for reading", filename);
    }

    try
    {
	for ( ; ; )
	{
	    bytes = bytesread = 4;
	    if ( (rv = apr_file_read(file, sizebuf, &bytesread)) != APR_SUCCESS )
	    {
		if ( (rv == APR_EOF) && (bytesread == 0) )
		{ //zero sized file, or end of logqueue file
		    break;
		}
		throw(rv, "failed to read 4 bytes from file, cannot fill logqueue");
	    }
	    ASSERT(bytes == bytesread);
	    
	    bytesread = bytes = (apr_size_t) nx_int32_from_le(sizebuf);
	
	    buf = malloc(bytes);
	    ASSERT(buf != NULL);
	
	    CHECKERR_MSG(apr_file_read(file, buf, &bytesread),
			 "failed to read %d bytes from file, cannot fill logqueue", bytes);
	    ASSERT(bytes == bytesread);

	    logdata = nx_logdata_from_membuf(buf, bytes, &bytesread);
	    ASSERT(bytes == bytesread);
	    totalread += bytesread + 4;

	    if ( logdata == NULL )
	    {
		throw_msg("failed to read logdata from file, cannot fill logqueue");
	    }
	
	    nx_logqueue_push(logqueue, logdata);
	    free(buf);
	    buf = NULL;
	}
    }
    catch(e)
    {
	if ( buf != NULL )
	{
	    free(buf);
	}
	if ( file != NULL )
	{
	    apr_file_close(file);
	}
	rethrow(e);
    }
    if ( buf != NULL )
    {
	free(buf);
    }
    if ( file != NULL )
    {
	apr_file_close(file);
    }
    // read successfully, remove it
    apr_file_remove(filename, logqueue->pool);

    return ( totalread );
}
