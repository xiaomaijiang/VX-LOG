/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <unistd.h>
#include <apr_lib.h>
#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/serialize.h"
#include "../../../common/alloc.h"

#include "pm_buffer.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static void pm_buffer_add_event(nx_module_t *module)
{
    nx_event_t *event;

    event = nx_event_new();
    event->module = module;
    event->delayed = TRUE;
    event->type = NX_EVENT_READ;
    event->time = apr_time_now() + APR_USEC_PER_SEC;
    event->priority = module->priority;
    nx_event_add(event);
}



static apr_size_t pm_buffer_push_disk(nx_module_t *module, nx_logdata_t *logdata)
{
    nx_pm_buffer_conf_t *modconf;
    apr_size_t memsize = 0;
    nx_exception_t e;
    char *buf = NULL;
    apr_pool_t *pool;
    char filename[APR_PATH_MAX];
    apr_uint32_t size32;

    ASSERT(module != NULL);
    ASSERT(logdata != NULL);

    modconf = (nx_pm_buffer_conf_t *) module->config;
    
    try
    {
	if ( modconf->push_count >= modconf->push_limit )
	{ // check if the current chunk is over the limit
	    ASSERT(modconf->push_file != NULL);
	    pool = apr_file_pool_get(modconf->push_file);
	    apr_file_close(modconf->push_file);
	    modconf->push_file = NULL;
	    apr_pool_destroy(pool);
	    (modconf->push_id)++;
	    modconf->push_count = 0;
	} 
	if ( modconf->push_file == NULL )
	{ // open chunk file which we append the logdata to
	    if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s.%"APR_INT64_T_FMT".q",
			      modconf->basedir, module->name, modconf->push_id) == sizeof(filename) )
	    {
		throw_msg("disk buffer filename length limit exceeeded");
	    }
	    pool = nx_pool_create_child(module->pool);
	    CHECKERR_MSG(apr_file_open(&(modconf->push_file), filename, APR_WRITE | APR_CREATE | APR_TRUNCATE,
				       APR_OS_DEFAULT, pool),
			 "couldn't open disk buffer file %s for writing", filename);
	}
	memsize = nx_logdata_serialized_size(logdata);
	buf = malloc(memsize + 4);
	size32 = (apr_uint32_t) memsize;
	nx_int32_to_le(buf, &size32);
	ASSERT(nx_logdata_to_membuf(logdata, buf + 4, memsize) == memsize);
	memsize += 4;
	CHECKERR_MSG(apr_file_write(modconf->push_file, buf, &memsize),
		     "failed to write disk buffer file");
	(modconf->push_count)++;
	
	free(buf);
	buf = NULL;
    }
    catch(e)
    {
	if ( buf != NULL )
	{
	    free(buf);
	    buf = NULL;
	}
	rethrow(e);
    }
    ASSERT(modconf->size >= 0);
    (modconf->size)++;

    return ( memsize );
}



static apr_size_t pm_buffer_pop_disk(nx_module_t *module, nx_logdata_t **logdata)
{
    char filename[APR_PATH_MAX];
    apr_pool_t *pool;
    nx_exception_t e;
    char lenbuf[4];
    apr_size_t bytes, bytesread;
    char *membuf = NULL;
    apr_status_t rv;
    nx_pm_buffer_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(logdata != NULL);

    modconf = (nx_pm_buffer_conf_t *) module->config;

    *logdata = NULL;
    if ( modconf->pop_file == NULL )
    {
	ASSERT(modconf->push_id > 0);
	ASSERT(modconf->pop_id > 0);
	if ( modconf->pop_id > modconf->push_id )
	{
	    // nothing to read
	    return ( 0 );
	}
	    
	if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s.%"APR_INT64_T_FMT".q",
			  modconf->basedir, module->name, modconf->pop_id) == sizeof(filename) )
	{
	    throw_msg("disk buffer filename length limit exceeeded");
	}
	pool = nx_pool_create_child(module->pool);
	CHECKERR_MSG(apr_file_open(&(modconf->pop_file), filename, APR_READ, APR_OS_DEFAULT, pool),
		     "couldn't open disk buffer file %s for reading in pm_buffer_pop_disk()", filename);
	modconf->pop_pos = 0;
    }

    // pop_file open
    ASSERT(modconf->pop_file != NULL);
    if ( modconf->push_id == modconf->pop_id )
    {
	ASSERT(modconf->pop_count <= modconf->push_count);
	if ( modconf->pop_count == modconf->push_count )
	{
	    // nothing to read
	    return ( 0 );
	}
    }
    else
    {
	ASSERT(modconf->push_id > modconf->pop_id);
    }

    try
    {
	bytesread = 4;
	rv = apr_file_read(modconf->pop_file, lenbuf, &bytesread);
	if ( APR_STATUS_IS_EOF(rv) )
	{
	    // end of chunk, reopen next, remove current
	    ASSERT(modconf->push_id > modconf->pop_id);

	    pool = apr_file_pool_get(modconf->pop_file);
	    if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s.%"APR_INT64_T_FMT".q",
			      modconf->basedir, module->name, modconf->pop_id) == sizeof(filename) )
	    {
		throw_msg("disk buffer filename length limit exceeeded");
	    }
	    apr_file_close(modconf->pop_file);
	    modconf->pop_file = NULL;
	    log_debug("removing chunk file: %s", filename);
	    apr_file_remove(filename, pool);
	    apr_pool_destroy(pool);
	    (modconf->pop_id)++;
	    if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s.%"APR_INT64_T_FMT".q",
			      modconf->basedir, module->name, modconf->pop_id) == sizeof(filename) )
	    {
		throw_msg("disk buffer filename length limit exceeeded");
	    }
	    pool = nx_pool_create_child(module->pool);
	    CHECKERR_MSG(apr_file_open(&(modconf->pop_file), filename, APR_READ, APR_OS_DEFAULT, pool),
			 "couldn't open disk buffer file %s for reading", filename);
	    modconf->pop_pos = 0;
	    modconf->pop_count = 0;
	    // try to read again
	    bytesread = 4;
	    rv = apr_file_read(modconf->pop_file, lenbuf, &bytesread);
	}
	if ( ! APR_STATUS_IS_EOF(rv) )
	{
	    CHECKERR_MSG(rv, "failed to read logdata length (4 bytes) from disk buffer file");
	    ASSERT(bytesread == 4);

	    bytes = nx_int32_from_le(lenbuf);
	    bytesread = bytes;
	    membuf = malloc(bytes);
	    CHECKERR_MSG(apr_file_read(modconf->pop_file, membuf, &bytesread),
			 "failed to read logdata from disk buffer file");
	    ASSERT(bytesread == bytes);
	    modconf->pop_pos = (apr_off_t) (bytesread + 4);
	    *logdata = nx_logdata_from_membuf(membuf, bytes, &bytesread);
	    free(membuf);
	    membuf = NULL;
	    (modconf->pop_count)++;
	    (modconf->size)--;
	    bytesread += 4;
	}
    }
    catch(e)
    {
	if ( membuf != NULL )
	{
	    free(membuf);
	    membuf = NULL;
	}
	rethrow(e);
    }

    return ( bytesread );
}



static void pm_buffer_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;
    nx_pm_buffer_conf_t *modconf;
    boolean can_send;
    nx_module_status_t status;
    apr_size_t bytesread;

    //log_info("nx_pm_buffer_data_available(), queue size: %d",  nx_logqueue_size(module->queue));
    
    modconf = (nx_pm_buffer_conf_t *) module->config;

    status = nx_module_get_status(module);

    if ( !((status == NX_MODULE_STATUS_RUNNING) || (status == NX_MODULE_STATUS_PAUSED)) )
    {
	log_debug("module %s not running, not processing any more data", module->name);
	return;
    }

    while ( (can_send = nx_module_can_send(module, 1.0)) == TRUE )
    {
	if ( modconf->type == NX_PM_BUFFER_TYPE_MEM )
	{
	    nx_logqueue_peek(modconf->queue, &logdata);
	    if ( logdata == NULL )
	    {
		break;
	    }
	    bytesread = nx_logdata_serialized_size(logdata) + 4;
	    ASSERT(modconf->buffer_size >= bytesread);
	    modconf->buffer_size -= bytesread;
	    nx_logqueue_pop(modconf->queue, logdata);
	    nx_module_progress_logdata(module, logdata);
	}
	else
	{ // DISK
	    bytesread = pm_buffer_pop_disk(module, &logdata);

	    if ( logdata == NULL )
	    {
		break;
	    }

	    ASSERT(modconf->buffer_size >= bytesread);
	    modconf->buffer_size -= bytesread;
	    nx_module_progress_logdata(module, logdata);
	}
    }

    if ( (can_send == FALSE) &&
	 (modconf->buffer_size >= modconf->buffer_maxsize) )
    { // can not send and can not store, do not do anything
	if ( modconf->warned_full != TRUE )
	{
	    log_warn("pm_buffer is full (%"APR_UINT64_T_FMT" kbytes)!",
		     (uint64_t) (modconf->buffer_size / 1024));
	    modconf->warned_full = TRUE;
	}

	log_debug("pm_buffer can not send and can not store, do not do anything");
	return;
    }

    if ( (modconf->warned_full == TRUE) &&
	 (modconf->buffer_size <= modconf->buffer_maxsize / 2) )
    {
	modconf->warned_full = FALSE;
    }
    if ( modconf->buffer_size <= (modconf->buffer_warnlimit / 2) )
    {
	modconf->warned = FALSE;
    }

    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    if ( can_send  == TRUE )
    {
	log_debug("pm_buffer can send");
	nx_module_progress_logdata(module, logdata);
    }
    else
    {
	log_debug("pm_buffer can not send [buffer size: %lu, count: %d]",
		  modconf->buffer_size, modconf->queue->size);

	if ( modconf->type == NX_PM_BUFFER_TYPE_MEM )
	{
	    modconf->buffer_size += nx_logdata_serialized_size(logdata) + 4;
	    nx_module_logqueue_pop(module, logdata);
	    nx_logqueue_push(modconf->queue, logdata);
	}
	else
	{ // DISK
	    apr_size_t written;
	    
	    written = pm_buffer_push_disk(module, logdata);
	    nx_module_logqueue_pop(module, logdata);
	    nx_logdata_free(logdata);
	    modconf->buffer_size += written;
	}

	if ( modconf->warned != TRUE )
	{
	    if ( (modconf->buffer_warnlimit > 0) &&
		 (modconf->buffer_size >= modconf->buffer_warnlimit) )
	    {
		log_warn("data in pm_buffer reached %lu kbytes",
			 (long unsigned) (modconf->buffer_size / 1024));
		modconf->warned = TRUE;
	    }
	}

	log_debug("pm_buffer stored logdata, buffer size is %lu (count %d)",
		  (long unsigned) modconf->buffer_size, modconf->queue->size);
    }
}



static void pm_buffer_config(nx_module_t *module)
{
    nx_pm_buffer_conf_t *modconf;
    const nx_directive_t *curr;
    const char *ptr;
    char qname[512];

    modconf = apr_pcalloc(module->pool, sizeof(nx_pm_buffer_conf_t));
    module->config = modconf;

    curr = module->directives;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "MaxSize") == 0 )
	{
	    if ( modconf->buffer_maxsize != 0 )
	    {
		nx_conf_error(curr, "MaxSize is already defined");
	    }
	    for ( ptr = curr->args; *ptr != '\0'; ptr++ )
	    {
		if ( ! apr_isdigit(*ptr) )
		{
		    nx_conf_error(curr, "invalid MaxSize '%s', digit expected", curr->args);
		}
	    }	    
	    if ( sscanf(curr->args, "%"APR_UINT64_T_FMT, &(modconf->buffer_maxsize)) != 1 )
	    {
		nx_conf_error(curr, "invalid MaxSize '%s'", curr->args);
	    }
	    modconf->buffer_maxsize *= 1024; // Kb -> bytes
	}
	else if ( strcasecmp(curr->directive, "WarnLimit") == 0 )
	{
	    if ( modconf->buffer_warnlimit != 0 )
	    {
		nx_conf_error(curr, "WarnLimit is already defined");
	    }
	    for ( ptr = curr->args; *ptr != '\0'; ptr++ )
	    {
		if ( ! apr_isdigit(*ptr) )
		{
		    nx_conf_error(curr, "invalid WarnLimit '%s', digit expected", curr->args);
		}
	    }	    
	    if ( sscanf(curr->args, "%"APR_UINT64_T_FMT, &(modconf->buffer_warnlimit)) != 1 )
	    {
		nx_conf_error(curr, "invalid WarnLimit '%s'", curr->args);
	    }
	    modconf->buffer_warnlimit *= 1024; // Kb -> bytes
	}
	else if ( strcasecmp(curr->directive, "Type") == 0 )
	{
	    if ( modconf->type != 0 )
	    {
		nx_conf_error(curr, "Type is already defined");
	    }
	    if ( strcasecmp(curr->args, "mem") == 0 )
	    {
		modconf->type = NX_PM_BUFFER_TYPE_MEM;
	    }
	    else if ( strcasecmp(curr->args, "disk") == 0 )
	    {
		modconf->type = NX_PM_BUFFER_TYPE_DISK;
	    }
	    else
	    {
		nx_conf_error(curr, "invalid Type '%s'", curr->args);
	    }
	}
	else if ( strcasecmp(curr->directive, "Directory") == 0 )
	{
	    if ( modconf->basedir != NULL )
	    {
		nx_conf_error(curr, "Directory is already defined");
	    }
	    modconf->basedir = apr_pstrdup(module->pool, curr->args);
	}

	curr = curr->next;
    }

    if ( modconf->buffer_maxsize == 0 )
    {
	nx_conf_error(module->directives, "MaxSize parameter must be defined for %s", module->name);
    }

    if ( modconf->type == 0 )
    {
	nx_conf_error(module->directives, "Type parameter (Mem|Disk) must be defined for %s", module->name);
    }

    if ( (modconf->type == NX_PM_BUFFER_TYPE_DISK) && (modconf->basedir == NULL) )
    {
	modconf->basedir = nx_module_get_cachedir();
    }

    apr_snprintf(qname, sizeof(qname), "%s:mem", module->name);
    modconf->queue = nx_logqueue_new(module->pool, qname);
    modconf->queue->basedir = nx_module_get_cachedir();
    if ( modconf->push_limit == 0 )
    {
	modconf->push_limit = NX_PM_BUFFER_DEFAULT_CHUNK_SIZE_LIMIT;
    }
}



static void pm_buffer_start(nx_module_t *module)
{
    nx_pm_buffer_conf_t *modconf;
    apr_dir_t *dir = NULL;
    nx_exception_t e;
    apr_status_t rv;
    apr_finfo_t finfo;
    size_t len;
    size_t namelen;
    char fname[1024];
    apr_pool_t * volatile pool;
    char filename[APR_PATH_MAX];

    ASSERT(module->config != NULL);

    modconf = (nx_pm_buffer_conf_t *) module->config;

    if ( modconf->type == NX_PM_BUFFER_TYPE_DISK )
    {
	pool = nx_pool_create_core();

	try
	{
	    int tmp_id;
	    apr_file_t *tempfile;
	    char lenbuf[4];
	    apr_size_t bytes, bytesread, total = 0;
	    apr_off_t offs = 0;

	    CHECKERR_MSG(apr_dir_open(&dir, modconf->basedir, pool),
			 "failed to read disk buffer files from '%s'", modconf->basedir);
	
	    namelen = (size_t) apr_snprintf(fname, sizeof(fname), "%s.", module->name);

	    for ( ; ; )
	    {
		rv = apr_dir_read(&finfo, APR_FINFO_NAME, dir);
		if ( APR_STATUS_IS_ENOENT(rv) )
		{
		    break;
		}
		if ( rv != APR_SUCCESS )
		{
		    throw(rv, "readdir failed on %s", modconf->basedir);
		}

		len = strlen(finfo.name);
		
		if ( len <= namelen + 2 )
		{
		    continue;
		}
		if ( strcmp(finfo.name + len - 2, ".q") != 0 )
		{
		    continue;
		}
		if ( strncmp(finfo.name, fname, namelen) != 0 )
		{
		    continue;
		}
		tmp_id = atoi(finfo.name + namelen);
		if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s",
				  modconf->basedir, finfo.name) == sizeof(filename) )
		{
		    throw_msg("disk buffer filename length limit exceeeded");
		}
		
		CHECKERR_MSG(apr_file_open(&tempfile, filename, APR_READ, APR_OS_DEFAULT, pool),
			     "couldn't open disk buffer file %s for reading", filename);
		
		for ( total = 0; ; )
		{
		    bytesread = 4;
		    rv = apr_file_read(tempfile, lenbuf, &bytesread);
		    if ( APR_STATUS_IS_EOF(rv) )
		    {
			break;
		    }
		    total += bytesread;
		    CHECKERR_MSG(rv, "failed to read logdata length (4 bytes) from disk buffer file");
		    bytes = nx_int32_from_le(lenbuf);
		    offs = (apr_off_t) bytes;
		    CHECKERR_MSG(apr_file_seek(tempfile, APR_CUR, &offs),
				 "failed to seek forward in disk buffer file");
		    (modconf->size)++;
		    modconf->buffer_size += bytes + 4;
		}
		apr_file_close(tempfile);

		if ( total == 0 )
		{ // zero sized file
		    log_debug("removing zero sized file: %s", filename);
		    apr_file_remove(filename, pool);
		    continue;
		}

		if ( modconf->push_id < tmp_id )
		{
		    modconf->push_id = tmp_id;
		}
		if ( (modconf->pop_id == 0) || (modconf->pop_id > tmp_id) )
		{
		    modconf->pop_id = tmp_id;
		}
	    }
	    CHECKERR(apr_dir_close(dir));
	    dir = NULL;
	    apr_pool_destroy(pool);
	    pool = NULL;

	    (modconf->push_id)++;
	    if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s.%"APR_INT64_T_FMT".q",
			      modconf->basedir, module->name, modconf->push_id) == sizeof(filename) )
	    {
		throw_msg("disk buffer filename length limit exceeeded");
	    }
	    pool = nx_pool_create_core();
	    CHECKERR_MSG(apr_file_open(&(modconf->push_file), filename, APR_WRITE | APR_CREATE | APR_TRUNCATE,
				       APR_OS_DEFAULT, pool),
			 "couldn't open disk buffer file %s for writing", filename);
	    modconf->push_count = 0;
	    if ( modconf->pop_id == 0 )
	    {
		modconf->pop_id = 1;
	    }
	}
	catch(e)
	{
	    if ( dir != NULL )
	    {
		apr_dir_close(dir);
		dir = NULL;
	    }
	    if ( pool != NULL )
	    {
		apr_pool_destroy(pool);
	    }
	    rethrow(e);
	}
    }
    else
    { // Mem: read stored queue
	modconf->buffer_size = nx_logqueue_from_file(modconf->queue);
    }
}



static void pm_buffer_stop(nx_module_t *module)
{
    nx_pm_buffer_conf_t *modconf;
    char filename[APR_PATH_MAX];
    apr_pool_t *pool;

    ASSERT(module != NULL);

    if ( module->config == NULL )
    {
	return;
    }

    modconf = (nx_pm_buffer_conf_t *) module->config;

    if ( modconf->type == NX_PM_BUFFER_TYPE_DISK )
    {
	// remove pop_file , we read it all
	if ( modconf->size == 0 )
	{ // queue empty, don't save anything
	    if ( modconf->pop_file != NULL )
	    {
		pool = apr_file_pool_get(modconf->pop_file);
		if ( apr_snprintf(filename, sizeof(filename), "%s"NX_DIR_SEPARATOR"%s.%"APR_INT64_T_FMT".q",
				  modconf->basedir, module->name, modconf->pop_id) == sizeof(filename) )
		{
		    throw_msg("disk buffer filename length limit exceeeded");
		}
		apr_file_close(modconf->pop_file);
		modconf->pop_file = NULL;
		apr_file_remove(filename, pool);
		apr_pool_destroy(pool);
	    }
	}
	if ( modconf->pop_file != NULL )
	{
	    pool = apr_file_pool_get(modconf->pop_file);
	    apr_file_close(modconf->pop_file);
	    modconf->pop_file = NULL;
	    apr_pool_destroy(pool);
	}

	if ( modconf->push_file != NULL )
	{
	    pool = apr_file_pool_get(modconf->push_file);
	    apr_file_close(modconf->push_file);
	    modconf->push_file = NULL;
	    apr_pool_destroy(pool);
	}

	modconf->pop_id = 0;
	modconf->push_id = 0;
	modconf->pop_count = 0;
	modconf->push_count = 0;
    }
    else if ( modconf->type == NX_PM_BUFFER_TYPE_MEM )
    {
	nx_logqueue_to_file(modconf->queue);
    }
}



static void pm_buffer_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    pm_buffer_data_available(module);
	    break;
	case NX_EVENT_READ:
	    pm_buffer_add_event(module);
	    pm_buffer_data_available(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}


extern nx_module_exports_t nx_module_exports_pm_buffer;

NX_MODULE_DECLARATION nx_pm_buffer_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_PROCESSOR,
    NULL,			// capabilities
    pm_buffer_config,		// config
    pm_buffer_start,		// start
    pm_buffer_stop, 		// stop
    NULL,	 		// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    pm_buffer_event,		// event
    NULL,			// info
    &nx_module_exports_pm_buffer,//exports
};
