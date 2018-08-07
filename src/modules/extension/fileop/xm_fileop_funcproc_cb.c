/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <unistd.h>
#include <sys/stat.h>
#include <apr_fnmatch.h>

#include "../../../common/module.h"
#include "../../../common/alloc.h"
#include "../../../core/nxlog.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

static boolean _file_exists(const char *filename, apr_pool_t *pool)
{
    apr_status_t rv;
    boolean retval = FALSE;
    apr_finfo_t finfo;

    rv = apr_stat(&finfo, filename, APR_FINFO_TYPE, pool);
    
    if ( rv == APR_SUCCESS )
    {
	if ( finfo.filetype == APR_REG )
	{
	    retval = TRUE;
	}
    }
    else if ( APR_STATUS_IS_ENOENT(rv) )
    {
    }
    else if ( APR_STATUS_IS_ENOTDIR(rv) )
    {
    }
    else
    {
	CHECKERR_MSG(rv, "failed to check whether file '%s' exists", filename);
    }

    return ( retval );
}



/* Reopen logfile if filename is our logfile */
static boolean _reopen_logfile(const char *filename)
{
    nx_ctx_t *ctx;
    const char *fname;
    apr_file_t *oldlog;

    ctx = nx_ctx_get();
	
    if ( ctx->logfile != NULL )
    {
	apr_file_name_get(&fname, ctx->logfile);
   
	if ( strcmp(fname, filename) == 0 )
	{
	    oldlog = ctx->logfile;
	    CHECKERR_MSG(apr_file_open(&(ctx->logfile), filename,
				       APR_WRITE | APR_CREATE | APR_APPEND,
				       APR_OS_DEFAULT, ctx->pool),
			 "couldn't open logfile '%s' for writing", filename);
	    apr_file_close(oldlog);
	    log_info("LogFile %s reopened", filename);
	    return ( TRUE );
	}
    }
    return ( FALSE );
}



void nx_expr_proc__xm_fileop_file_cycle(nx_expr_eval_ctx_t *eval_ctx,
					nx_module_t *module,
					nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t file;
    nx_expr_list_elem_t *val;
    nx_value_t value;
    apr_status_t rv;
    apr_pool_t *pool = NULL;
    nx_exception_t e;
    volatile int64_t max = 0;
    int32_t i, last;
    nx_string_t *tmpstr = NULL;
    nx_string_t *tmpstr2 = NULL;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    nx_expr_evaluate(eval_ctx, &file, arg->expr);

    if ( file.defined != TRUE )
    {
	throw_msg("'file' is undef");
    }
    if ( file.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'file'");
    }

    val = NX_DLIST_NEXT(arg, link);
    if ( val != NULL )
    {
	ASSERT(val->expr != NULL);
	nx_expr_evaluate(eval_ctx, &value, val->expr);
	if ( value.type != NX_VALUE_TYPE_INTEGER )
	{
	    nx_value_kill(&file);
	    nx_value_kill(&value);
	    throw_msg("integer type required for 'max'");
	}
	if ( value.defined == TRUE )
	{
	    if ( value.integer <= 0 )
	    {
		nx_value_kill(&file);
		nx_value_kill(&value);
		throw_msg("'max' must be a positive integer");
	    }
	    max = value.integer;
	}
    }

    try
    {
	pool = nx_pool_create_core();
	
	if ( _file_exists(file.string->buf, pool) == TRUE )
	{  // check if the file we need to cycle exists
	    tmpstr = nx_string_new();
	    tmpstr2 = nx_string_new();
	    last = 0;
	    for ( i = 1; i < 2147483647 /*APR_INT32_MAX*/; i++ )
	    {
		nx_string_sprintf(tmpstr, "%s.%d", file.string->buf, i);
		if ( _file_exists(tmpstr->buf, pool) == FALSE )
		{
		    break;
		}
		if ( (max > 0) && (i >= max) )
		{
		    log_info("removing file %s", tmpstr->buf);
		    if ( (rv = apr_file_remove(tmpstr->buf, NULL)) != APR_SUCCESS )
		    {
			log_aprerror(rv, "failed to remove file '%s'", tmpstr->buf);
		    }
		}
		else
		{
		    last = i;
		}
	    }
	    if ( last > 0 )
	    { // now starting from the last existing file, cycle them
		for ( i = last; i > 0; i-- )
		{
		    nx_string_sprintf(tmpstr, "%s.%d", file.string->buf, i);
		    nx_string_sprintf(tmpstr2, "%s.%d", file.string->buf, i + 1);
		    log_debug("cycling %s to %s", tmpstr->buf, tmpstr2->buf);
		    if ( (rv = apr_file_rename(tmpstr->buf, tmpstr2->buf, NULL)) != APR_SUCCESS )
		    {
			log_aprerror(rv, "failed to rename file from '%s' to '%s'", 
				     tmpstr->buf, tmpstr2->buf);
		    }
		}
	    }
	    // finally rename file to file.1
	    nx_string_sprintf(tmpstr, "%s.%d", file.string->buf, 1);
	    if ( (rv = apr_file_rename(file.string->buf, tmpstr->buf, NULL)) != APR_SUCCESS )
	    {
		log_aprerror(rv, "failed to rename file from '%s' to '%s'", 
			     file.string->buf, tmpstr->buf);
	    }
	    _reopen_logfile(file.string->buf);

	    nx_string_free(tmpstr);
	    nx_string_free(tmpstr2);
	}
	apr_pool_destroy(pool);
	nx_value_kill(&file);
    }
    catch(e)
    {
	nx_value_kill(&file);
	if ( pool != NULL )
	{
	    apr_pool_destroy(pool);
	}
	if ( tmpstr != NULL )
	{
	    nx_string_free(tmpstr);
	}
	if ( tmpstr2 != NULL )
	{
	    nx_string_free(tmpstr2);
	}
	log_exception(e);
    }
}



void nx_expr_proc__xm_fileop_file_rename(nx_expr_eval_ctx_t *eval_ctx,
					 nx_module_t *module,
					 nx_expr_list_t *args)
{
    nx_expr_list_elem_t *src, *dst;
    nx_value_t srcval, dstval;
    nx_exception_t e;
    apr_status_t rv;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    src = NX_DLIST_FIRST(args);
    ASSERT(src != NULL);
    ASSERT(src->expr != NULL);
    dst = NX_DLIST_NEXT(src, link);
    ASSERT(dst != NULL);
    ASSERT(dst->expr != NULL);

    nx_expr_evaluate(eval_ctx, &srcval, src->expr);

    if ( srcval.defined != TRUE )
    {
	throw_msg("'src' is undef");
    }
    if ( srcval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&srcval);
	throw_msg("string type required for 'src'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &dstval, dst->expr);
    }
    catch(e)
    {
	nx_value_kill(&srcval);
	rethrow(e);
    }
    if ( dstval.defined != TRUE )
    {
	nx_value_kill(&srcval);
	throw_msg("'dst' is undef");
    }
    if ( dstval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&dstval);
	nx_value_kill(&srcval);
	throw_msg("string type required for 'dst'");
    }

    if ( (rv = apr_file_rename(srcval.string->buf, dstval.string->buf, NULL)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to rename file from '%s' to '%s'", 
		     srcval.string->buf, dstval.string->buf);
    }

    _reopen_logfile(srcval.string->buf);

    nx_value_kill(&srcval);
    nx_value_kill(&dstval);
}



void nx_expr_proc__xm_fileop_file_copy(nx_expr_eval_ctx_t *eval_ctx,
				       nx_module_t *module,
				       nx_expr_list_t *args)
{
    nx_expr_list_elem_t *src, *dst;
    nx_value_t srcval, dstval;
    nx_exception_t e;
    apr_status_t rv;
    apr_pool_t *pool;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    src = NX_DLIST_FIRST(args);
    ASSERT(src != NULL);
    ASSERT(src->expr != NULL);
    dst = NX_DLIST_NEXT(src, link);
    ASSERT(dst != NULL);
    ASSERT(dst->expr != NULL);

    nx_expr_evaluate(eval_ctx, &srcval, src->expr);

    if ( srcval.defined != TRUE )
    {
	throw_msg("'src' is undef");
    }
    if ( srcval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&srcval);
	throw_msg("string type required for 'src'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &dstval, dst->expr);
    }
    catch(e)
    {
	nx_value_kill(&srcval);
	rethrow(e);
    }
    if ( dstval.defined != TRUE )
    {
	nx_value_kill(&srcval);
	throw_msg("'dst' is undef");
    }
    if ( dstval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&dstval);
	nx_value_kill(&srcval);
	throw_msg("string type required for 'dst'");
    }
    
    pool = nx_pool_create_core();
    if ( (rv = apr_file_copy(srcval.string->buf, dstval.string->buf, APR_FILE_SOURCE_PERMS,
			     pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to copy file from '%s' to '%s'", 
		     srcval.string->buf, dstval.string->buf);
    }
    apr_pool_destroy(pool);
    nx_value_kill(&srcval);
    nx_value_kill(&dstval);
}



void nx_expr_proc__xm_fileop_file_remove(nx_expr_eval_ctx_t *eval_ctx,
					 nx_module_t *module,
					 nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t file;
    apr_status_t rv;
    apr_pool_t *pool = NULL;
    apr_dir_t *dir;
    nx_exception_t e;
    nx_string_t *dirname = NULL;
    nx_string_t *fname = NULL;
    char *filename;
    char *idx;
    int flags = 0;
    apr_finfo_t finfo;
    nx_expr_list_elem_t *older;
    nx_value_t olderval;
    apr_time_t older_time = 0LL;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    nx_expr_evaluate(eval_ctx, &file, arg->expr);

    if ( file.defined != TRUE )
    {
	throw_msg("'file' is undef");
    }

    try
    {
	if ( file.type != NX_VALUE_TYPE_STRING )
	{
	    throw_msg("string type required for 'file'");
	}

	older = NX_DLIST_NEXT(arg, link);
	if ( older != NULL )
	{
	    ASSERT(older->expr != NULL);
	    nx_expr_evaluate(eval_ctx, &olderval, older->expr);
	    if ( olderval.type != NX_VALUE_TYPE_DATETIME )
	    {
		nx_value_kill(&olderval);
		throw_msg("datetime type required for 'older'");
	    }
	    if ( olderval.defined == TRUE )
	    {
		older_time = olderval.datetime;
	    }
	}

	if ( apr_fnmatch_test(file.string->buf) != 0 )
	{ // we have wildcards, expand it
	    pool = nx_pool_create_core();

	    filename = file.string->buf;
	    log_debug("file_remove() called with wildcarded path: %s", filename);

	    idx = strrchr(filename, '/');
#ifdef WIN32
	    flags = APR_FNM_CASE_BLIND;
	    if ( idx == NULL ) 
	    {
		idx = strrchr(filename, '\\');
	    }
#endif
	    if ( idx == NULL )
	    {
		dirname = nx_string_create("."NX_DIR_SEPARATOR, -1);
	    }
	    else
	    {
		dirname = nx_string_create(filename, (int) (idx + 1 - filename));
		filename = idx + 1;
	    }
	    log_debug("file_remove(): checking for matching files under %s", dirname->buf);

	    CHECKERR_MSG(apr_dir_open(&dir, dirname->buf, pool),
			 "failed to open directory: %s", dirname->buf);
	    fname = nx_string_new();

	    while ( apr_dir_read(&finfo, APR_FINFO_NAME | APR_FINFO_TYPE | APR_FINFO_CTIME,
				 dir) == APR_SUCCESS )
	    {
		if ( finfo.filetype == APR_REG )
		{
		    log_debug("checking '%s' against wildcard '%s':", finfo.name, filename);
		    if ( apr_fnmatch(filename, finfo.name, flags) == APR_SUCCESS )
		    {
			nx_string_sprintf(fname, "%s%s", dirname->buf, finfo.name);

			if ( (older_time == 0) ||
			     ((older_time != 0) && (finfo.ctime < older_time)) )
			{
			    log_debug("'%s' matches wildcard '%s' and is 'older', removing",
				      fname->buf, file.string->buf);
			    log_info("removing file %s", fname->buf);
			    rv = apr_file_remove(fname->buf, NULL);
			    if ( APR_STATUS_IS_ENOENT(rv) )
			    {
			    }
			    else if ( rv == APR_SUCCESS )
			    {
			    }
			    else
			    {
				log_aprerror(rv, "failed to remove file '%s'", fname->buf);
			    }
			    _reopen_logfile(fname->buf);
			}
		    }
		}
	    }
	    nx_string_free(fname);
	    fname = NULL;
	    apr_pool_destroy(pool);
	    pool = NULL;
	}
	else
	{
	    rv = apr_file_remove(file.string->buf, NULL);
	    if ( APR_STATUS_IS_ENOENT(rv) )
	    {
	    }
	    else
	    {
		CHECKERR_MSG(rv, "failed to remove file '%s'", file.string->buf);
	    }
	}
    }
    catch(e)
    {
	log_exception(e);
    }
    nx_value_kill(&file);
    if ( pool != NULL )
    {
	apr_pool_destroy(pool);
    }
    if ( dirname != NULL )
    {
	nx_string_free(dirname);
    }
    if ( fname != NULL )
    {
	nx_string_free(fname);
    }
}



void nx_expr_proc__xm_fileop_file_link(nx_expr_eval_ctx_t *eval_ctx,
				       nx_module_t *module,
				       nx_expr_list_t *args)
{
#ifdef HAVE_APR_FILE_LINK
    nx_expr_list_elem_t *src, *dst;
    nx_value_t srcval, dstval;
    nx_exception_t e;
    apr_status_t rv;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    src = NX_DLIST_FIRST(args);
    ASSERT(src != NULL);
    ASSERT(src->expr != NULL);
    dst = NX_DLIST_NEXT(src, link);
    ASSERT(dst != NULL);
    ASSERT(dst->expr != NULL);

    nx_expr_evaluate(eval_ctx, &srcval, src->expr);

    if ( srcval.defined != TRUE )
    {
	throw_msg("'src' is undef");
    }
    if ( srcval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&srcval);
	throw_msg("string type required for 'src'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &dstval, dst->expr);
    }
    catch(e)
    {
	nx_value_kill(&srcval);
	rethrow(e);
    }
    if ( dstval.defined != TRUE )
    {
	nx_value_kill(&srcval);
	throw_msg("'dst' is undef");
    }
    if ( dstval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&dstval);
	nx_value_kill(&srcval);
	throw_msg("string type required for 'dst'");
    }

    if ( (rv = apr_file_link(srcval.string->buf, dstval.string->buf)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to link file from '%s' to '%s'", 
		     srcval.string->buf, dstval.string->buf);
    }
    nx_value_kill(&srcval);
    nx_value_kill(&dstval);
#else
    throw_msg("file_link() is not available because apr_file_link is not provided by the linked apr library. Recompile with a newer apr version");
#endif
}



void nx_expr_proc__xm_fileop_file_append(nx_expr_eval_ctx_t *eval_ctx,
					 nx_module_t *module,
					 nx_expr_list_t *args)
{
    nx_expr_list_elem_t *src, *dst;
    nx_value_t srcval, dstval;
    nx_exception_t e;
    apr_status_t rv;
    apr_pool_t *pool;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    src = NX_DLIST_FIRST(args);
    ASSERT(src != NULL);
    ASSERT(src->expr != NULL);
    dst = NX_DLIST_NEXT(src, link);
    ASSERT(dst != NULL);
    ASSERT(dst->expr != NULL);

    nx_expr_evaluate(eval_ctx, &srcval, src->expr);

    if ( srcval.defined != TRUE )
    {
	throw_msg("'src' is undef");
    }
    if ( srcval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&srcval);
	throw_msg("string type required for 'src'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &dstval, dst->expr);
    }
    catch(e)
    {
	nx_value_kill(&srcval);
	rethrow(e);
    }
    if ( dstval.defined != TRUE )
    {
	nx_value_kill(&srcval);
	throw_msg("'dst' is undef");
    }
    if ( dstval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&dstval);
	nx_value_kill(&srcval);
	throw_msg("string type required for 'dst'");
    }

    pool = nx_pool_create_core();
    if ( (rv = apr_file_append(srcval.string->buf, dstval.string->buf, APR_FILE_SOURCE_PERMS,
			       pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to append file from '%s' to '%s'", 
		     srcval.string->buf, dstval.string->buf);
    }
    apr_pool_destroy(pool);
    nx_value_kill(&srcval);
    nx_value_kill(&dstval);
}



void nx_expr_proc__xm_fileop_file_write(nx_expr_eval_ctx_t *eval_ctx,
					nx_module_t *module,
					nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t file;
    nx_expr_list_elem_t *val;
    nx_value_t value;
    nx_exception_t e;
    apr_status_t rv;
    apr_pool_t *pool;
    apr_file_t *fd;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);
    val = NX_DLIST_NEXT(arg, link);
    ASSERT(val != NULL);
    ASSERT(val->expr != NULL);

    nx_expr_evaluate(eval_ctx, &file, arg->expr);

    if ( file.defined != TRUE )
    {
	throw_msg("'file' is undef");
    }
    if ( file.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'file'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &value, val->expr);
    }
    catch(e)
    {
	nx_value_kill(&file);
	rethrow(e);
    }
    if ( value.defined != TRUE )
    {
	nx_value_kill(&file);
	// do not write anything
	return;
    }
    if ( value.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	nx_value_kill(&value);
	throw_msg("string type required for 'value'");
    }

    pool = nx_pool_create_core();

    if ( (rv = apr_file_open(&fd, file.string->buf, APR_WRITE | APR_CREATE | APR_APPEND,
			     APR_OS_DEFAULT, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to open file '%s' when trying to write",
		     file.string->buf);
    }
    if ( rv == APR_SUCCESS )
    {
	if ( (rv = apr_file_write_full(fd, value.string->buf, value.string->len, NULL)) != APR_SUCCESS )
	{
	    log_aprerror(rv, "failed to write value to file '%s'", file.string->buf);
	}
	apr_file_close(fd);
    }
    apr_pool_destroy(pool);
    nx_value_kill(&file);
    nx_value_kill(&value);
}



void nx_expr_proc__xm_fileop_file_truncate(nx_expr_eval_ctx_t *eval_ctx,
					   nx_module_t *module,
					   nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t file;
    nx_expr_list_elem_t *offset;
    nx_value_t offsetval;
    nx_exception_t e;
    apr_off_t offs = 0;
    apr_status_t rv;
    apr_pool_t *pool;
    apr_file_t *fd;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    nx_expr_evaluate(eval_ctx, &file, arg->expr);

    if ( file.defined != TRUE )
    {
	throw_msg("'file' is undef");
    }
    if ( file.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'file'");
    }

    if ( (offset = NX_DLIST_NEXT(arg, link)) != NULL )
    {
	ASSERT(offset->expr != NULL);
	try
	{
	    nx_expr_evaluate(eval_ctx, &offsetval, offset->expr);
	}
	catch(e)
	{
	    nx_value_kill(&file);
	    rethrow(e);
	}

	if ( offsetval.defined != TRUE )
	{
	    nx_value_kill(&file);
	    throw_msg("'offset' is undef");
	}
	if ( offsetval.type != NX_VALUE_TYPE_INTEGER )
	{
	    nx_value_kill(&offsetval);
	    nx_value_kill(&file);
	    throw_msg("integer type required for 'offset'");
	}
	offs = (apr_off_t) offsetval.integer;
    }

    pool = nx_pool_create_core();

    if ( (rv = apr_file_open(&fd, file.string->buf, APR_WRITE | APR_CREATE, APR_OS_DEFAULT,
			     pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to open file '%s' when trying to truncate",
		     file.string->buf);
    }
    if ( rv == APR_SUCCESS )
    {
	if ( (rv = apr_file_trunc(fd, offs)) != APR_SUCCESS )
	{
	    log_aprerror(rv, "failed to truncate file '%s' to length %lu",
			 file.string->buf, (long unsigned int) offs);
	}
	apr_file_close(fd);
    }
    apr_pool_destroy(pool);
    nx_value_kill(&file);
}



void nx_expr_proc__xm_fileop_file_chown(nx_expr_eval_ctx_t *eval_ctx,
					nx_module_t *module,
					nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t file;
    nx_expr_list_elem_t *uid, *gid;
    nx_value_t uidval, gidval;
    nx_exception_t e;
    int rv;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    uid = NX_DLIST_NEXT(arg, link);
    ASSERT(uid != NULL);
    ASSERT(uid->expr != NULL);

    gid = NX_DLIST_NEXT(uid, link);
    ASSERT(gid != NULL);
    ASSERT(gid->expr != NULL);


    nx_expr_evaluate(eval_ctx, &file, arg->expr);

    if ( file.defined != TRUE )
    {
	throw_msg("'file' is undef");
    }
    if ( file.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'file'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &uidval, uid->expr);
    }
    catch(e)
    {
	nx_value_kill(&file);
	rethrow(e);
    }
    if ( uidval.defined != TRUE )
    {
	nx_value_kill(&file);
	throw_msg("'uid' is undef");
    }
    if ( uidval.type != NX_VALUE_TYPE_INTEGER )
    {
	nx_value_kill(&file);
	throw_msg("integer type required for 'uid'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &gidval, gid->expr);
    }
    catch(e)
    {
	nx_value_kill(&file);
	rethrow(e);
    }
    if ( gidval.defined != TRUE )
    {
	nx_value_kill(&file);
	throw_msg("'gid' is undef");
    }
    if ( gidval.type != NX_VALUE_TYPE_INTEGER )
    {
	nx_value_kill(&file);
	throw_msg("integer type required for 'gid'");
    }

#ifdef HAVE_CHOWN
    if ( (rv = chown(file.string->buf, (uid_t) uidval.integer, (gid_t) gidval.integer)) != 0 )
    {
	log_errno("failed to change file ownership on '%s'", file.string->buf);
    }
#else
    log_error("This platform does not support the file_chown() function");
#endif    
    nx_value_kill(&file);
    nx_value_kill(&uidval);
    nx_value_kill(&gidval);
}



void nx_expr_proc__xm_fileop_file_chown_usr_grp(nx_expr_eval_ctx_t *eval_ctx,
					nx_module_t *module,
					nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t file;
    nx_expr_list_elem_t *user, *group;
    nx_value_t usrval, grpval;
    nx_exception_t e;
    int rv;
    apr_status_t rv2;
    apr_uid_t uid;
    apr_gid_t tmp;
    apr_gid_t gid;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    user = NX_DLIST_NEXT(arg, link);
    ASSERT(user != NULL);
    ASSERT(user->expr != NULL);

    group = NX_DLIST_NEXT(user, link);
    ASSERT(group != NULL);
    ASSERT(group->expr != NULL);

    nx_expr_evaluate(eval_ctx, &file, arg->expr);

    if ( file.defined != TRUE )
    {
	throw_msg("'file' is undef");
    }
    if ( file.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'file'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &usrval, user->expr);
    }
    catch(e)
    {
	nx_value_kill(&file);
	rethrow(e);
    }
    if ( usrval.defined != TRUE )
    {
	nx_value_kill(&file);
	throw_msg("'user' is undef");
    }
    if ( usrval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'user'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &grpval, group->expr);
    }
    catch(e)
    {
	nx_value_kill(&file);
	rethrow(e);
    }
    if ( grpval.defined != TRUE )
    {
	nx_value_kill(&file);
	throw_msg("'group' is undef");
    }
    if ( grpval.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'group'");
    }
    
    if ( (rv2 = apr_gid_get(&gid, grpval.string->buf, module->pool)) != APR_SUCCESS )
    {
	log_aprerror(rv2, "failed to change file ownership on '%s', couldn't resolve group name '%s'",
		     file.string->buf, grpval.string->buf);
    }
    else
    { // APR_SUCCESS
	if ( (rv2 = apr_uid_get(&uid, &tmp, usrval.string->buf, module->pool)) != APR_SUCCESS )
	{
	    log_aprerror(rv2, "failed to change file ownership on '%s', couldn't resolve user name '%s'",
			 file.string->buf, usrval.string->buf);
	}
	else
	{ // APR_SUCCESS
#ifdef HAVE_CHOWN
	    if ( (rv = chown(file.string->buf, (uid_t)uid, (gid_t)gid)) != 0 )
	    {
		log_errno("failed to change file ownership on '%s'", file.string->buf);
	    }
#else
	    log_error("This platform does not support the file_chown() function");
#endif    
	}
    }
    nx_value_kill(&file);
    nx_value_kill(&usrval);
    nx_value_kill(&grpval);
}



void nx_expr_proc__xm_fileop_file_chmod(nx_expr_eval_ctx_t *eval_ctx,
					nx_module_t *module,
					nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t file;
    nx_expr_list_elem_t *mode;
    nx_value_t modeval;
    nx_exception_t e;
    int rv;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    mode = NX_DLIST_NEXT(arg, link);
    ASSERT(mode != NULL);
    ASSERT(mode->expr != NULL);

    nx_expr_evaluate(eval_ctx, &file, arg->expr);

    if ( file.defined != TRUE )
    {
	throw_msg("'file' is undef");
    }
    if ( file.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'file'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &modeval, mode->expr);
    }
    catch(e)
    {
	nx_value_kill(&file);
	rethrow(e);
    }
    if ( modeval.defined != TRUE )
    {
	nx_value_kill(&file);
	throw_msg("'mode' is undef");
    }
    if ( modeval.type != NX_VALUE_TYPE_INTEGER )
    {
	nx_value_kill(&file);
	throw_msg("integer type required for 'mode'");
    }

#ifdef HAVE_CHMOD
    if ( (rv = chmod(file.string->buf, (mode_t) modeval.integer)) != 0 )
    {
	log_aprerror(rv, "failed to change file ownership on '%s'", file.string->buf);
    }
#else
    log_error("This platform does not support the file_chown() function");
#endif    
    nx_value_kill(&file);
}



void nx_expr_proc__xm_fileop_file_touch(nx_expr_eval_ctx_t *eval_ctx,
					nx_module_t *module,
					nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t file;
    apr_status_t rv;
    apr_pool_t *pool;
    apr_file_t *fd;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    nx_expr_evaluate(eval_ctx, &file, arg->expr);

    if ( file.defined != TRUE )
    {
	throw_msg("'file' is undef");
    }
    if ( file.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&file);
	throw_msg("string type required for 'file'");
    }

    pool = nx_pool_create_core();

    if ( (rv = apr_file_open(&fd, file.string->buf, APR_WRITE | APR_CREATE, APR_OS_DEFAULT,
			     pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to open file '%s' when trying to touch",
		     file.string->buf);
    }
    if ( rv == APR_SUCCESS )
    {
	apr_file_close(fd);
	rv = apr_file_mtime_set(file.string->buf, apr_time_now(), pool);
	if ( rv == APR_SUCCESS )
	{
	}
	else if ( APR_STATUS_IS_ENOTIMPL(rv) )
	{
	}
	else
	{
	    log_aprerror(rv, "failed to set mtime on file '%s' when trying to touch",
			 file.string->buf);
	}
    }

    apr_pool_destroy(pool);
    nx_value_kill(&file);
}



void nx_expr_proc__xm_fileop_dir_make(nx_expr_eval_ctx_t *eval_ctx,
				      nx_module_t *module,
				      nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t path;
    apr_status_t rv;
    apr_pool_t *pool;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    nx_expr_evaluate(eval_ctx, &path, arg->expr);

    if ( path.defined != TRUE )
    {
	throw_msg("'path' is undef");
    }
    if ( path.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&path);
	throw_msg("string type required for 'path'");
    }

    pool = nx_pool_create_core();
    if ( (rv = apr_dir_make_recursive(path.string->buf, APR_OS_DEFAULT, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to create directory '%s'", path.string->buf);
    }
    apr_pool_destroy(pool);
    nx_value_kill(&path);
}



void nx_expr_proc__xm_fileop_dir_remove(nx_expr_eval_ctx_t *eval_ctx,
					nx_module_t *module,
					nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t path;
    apr_status_t rv;
    apr_pool_t *pool;

    ASSERT(module != NULL);

    ASSERT(args != NULL);
    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);

    nx_expr_evaluate(eval_ctx, &path, arg->expr);

    if ( path.defined != TRUE )
    {
	throw_msg("'path' is undef");
    }
    if ( path.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&path);
	throw_msg("string type required for 'path'");
    }

    // TODO: remove contents before

    pool = nx_pool_create_core();
    rv = apr_dir_remove(path.string->buf, pool);
    if ( APR_STATUS_IS_ENOENT(rv) )
    {
    }
    else if ( rv == APR_SUCCESS )
    {
    }
    else
    {
	    log_aprerror(rv, "failed to remove directory '%s'", path.string->buf);
    }
    apr_pool_destroy(pool);
    nx_value_kill(&path);
}



void nx_expr_func__xm_fileop_file_read(nx_expr_eval_ctx_t *eval_ctx UNUSED,
				       nx_module_t *module UNUSED,
				       nx_value_t *retval,
				       int32_t num_arg,
				       nx_value_t *args)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_file_t *fd;
    apr_finfo_t finfo;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    retval->defined = FALSE;
    if ( args[0].defined == FALSE )
    {
	return;
    }

    pool = nx_pool_create_core();
    if ( (rv = apr_file_open(&fd, args[0].string->buf, APR_READ, APR_OS_DEFAULT,
			     pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to open file '%s' when trying to read its contents",
		     args[0].string->buf);
	apr_pool_destroy(pool);
	return;
    }

    if ( (rv = apr_stat(&finfo, args[0].string->buf, APR_FINFO_SIZE, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to determine file size of '%s'", args[0].string->buf);
	apr_pool_destroy(pool);
	return;
    }

    retval->string = nx_string_new_size((size_t) finfo.size);
    retval->string->len = (uint32_t) finfo.size;
    if ( (rv = apr_file_read_full(fd, retval->string->buf, (apr_size_t) finfo.size, NULL)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to read file contents from '%s'", args[0].string->buf);
	nx_string_free(retval->string);
	apr_pool_destroy(pool);
	return;
    }
    retval->defined = TRUE;

    apr_pool_destroy(pool);
}



void nx_expr_func__xm_fileop_file_exists(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					 nx_module_t *module UNUSED,
					 nx_value_t *retval,
					 int32_t num_arg,
					 nx_value_t *args)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_finfo_t finfo;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_BOOLEAN;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    retval->defined = TRUE;
    retval->boolean = FALSE;

    pool = nx_pool_create_core();
    rv = apr_stat(&finfo, args[0].string->buf, APR_FINFO_TYPE, pool);
    if ( rv == APR_SUCCESS )
    {
	if ( finfo.filetype == APR_REG )
	{
	    retval->boolean = TRUE;
	}
    }
    else if ( APR_STATUS_IS_ENOENT(rv) )
    {
    }
    else
    {
	log_aprerror(rv, "failed to check whether file '%s' exists", args[0].string->buf);
    }
    apr_pool_destroy(pool);
}



void nx_expr_func__xm_fileop_file_basename(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					   nx_module_t *module UNUSED,
					   nx_value_t *retval,
					   int32_t num_arg,
					   nx_value_t *args)
{
    char *idx;
    char *filename;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    filename = args[0].string->buf;
    idx = strrchr(filename, '/');
#ifdef WIN32
    if ( idx == NULL ) 
    {
        idx = strrchr(filename, '\\');
    }
#endif

    retval->defined = TRUE;

    if ( idx == NULL )
    {
	retval->string = nx_string_create(filename, -1);
	return;
    }

    retval->string = nx_string_create(idx + 1, -1);
}



void nx_expr_func__xm_fileop_file_dirname(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					  nx_module_t *module UNUSED,
					  nx_value_t *retval,
					  int32_t num_arg,
					  nx_value_t *args)
{
    char *idx;
    char *filename;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }
    retval->defined = TRUE;

    filename = args[0].string->buf;
    idx = strrchr(filename, '/');
#ifdef WIN32
    if ( idx == NULL ) 
    {
        idx = strrchr(filename, '\\');
    }
#endif

    retval->defined = TRUE;

    if ( idx == NULL )
    {
	retval->string = nx_string_new();
	return;
    }

    retval->string = nx_string_create(filename, (int) (idx - filename));
}



void nx_expr_func__xm_fileop_file_mtime(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					nx_module_t *module UNUSED,
					nx_value_t *retval,
					int32_t num_arg,
					nx_value_t *args)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_finfo_t finfo;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_DATETIME;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    pool = nx_pool_create_core();
    if ( (rv = apr_stat(&finfo, args[0].string->buf, APR_FINFO_MTIME, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to read file modification time on '%s'", args[0].string->buf);
	retval->defined = FALSE;
	apr_pool_destroy(pool);
	return;
    }

    retval->defined = TRUE;
    retval->datetime = finfo.mtime;

    apr_pool_destroy(pool);
}



void nx_expr_func__xm_fileop_file_ctime(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					nx_module_t *module UNUSED,
					nx_value_t *retval,
					int32_t num_arg,
					nx_value_t *args)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_finfo_t finfo;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_DATETIME;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    pool = nx_pool_create_core();
    if ( (rv = apr_stat(&finfo, args[0].string->buf, APR_FINFO_CTIME, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to read file creation time on '%s'", args[0].string->buf);
	retval->defined = FALSE;
	apr_pool_destroy(pool);
	return;
    }

    retval->defined = TRUE;
    retval->datetime = finfo.ctime;

    apr_pool_destroy(pool);
}



void nx_expr_func__xm_fileop_file_type(nx_expr_eval_ctx_t *eval_ctx UNUSED,
				       nx_module_t *module UNUSED,
				       nx_value_t *retval,
				       int32_t num_arg,
				       nx_value_t *args)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_finfo_t finfo;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    pool = nx_pool_create_core();
    if ( (rv = apr_stat(&finfo, args[0].string->buf, APR_FINFO_TYPE, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to read file type information on '%s'", args[0].string->buf);
	retval->defined = FALSE;
	apr_pool_destroy(pool);
	return;
    }

    retval->defined = TRUE;
    switch ( finfo.filetype )
    {
	case APR_REG:
	    retval->string = nx_string_create("FILE", -1);
	    break;
	case APR_DIR:
	    retval->string = nx_string_create("DIR", -1);
	    break;
	case APR_CHR:
	    retval->string = nx_string_create("CHAR", -1);
	    break;
	case APR_BLK:
	    retval->string = nx_string_create("BLOCK", -1);
	    break;
	case APR_PIPE:
	    retval->string = nx_string_create("PIPE", -1);
	    break;
	case APR_LNK:
	    retval->string = nx_string_create("LINK", -1);
	    break;
	case APR_SOCK:
	    retval->string = nx_string_create("SOCKET", -1);
	    break;
	case APR_NOFILE:
	case APR_UNKFILE:
	default:
	    retval->string = nx_string_create("UNKNOWN", -1);
	    break;
    }

    apr_pool_destroy(pool);
}



void nx_expr_func__xm_fileop_file_size(nx_expr_eval_ctx_t *eval_ctx UNUSED,
				       nx_module_t *module UNUSED,
				       nx_value_t *retval,
				       int32_t num_arg,
				       nx_value_t *args)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_finfo_t finfo;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    pool = nx_pool_create_core();
    if ( (rv = apr_stat(&finfo, args[0].string->buf, APR_FINFO_SIZE, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to determine file size of '%s'", args[0].string->buf);
	retval->defined = FALSE;
	apr_pool_destroy(pool);
	return;
    }

    retval->defined = TRUE;
    retval->integer = (int64_t) finfo.size;

    apr_pool_destroy(pool);
}



void nx_expr_func__xm_fileop_file_inode(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					nx_module_t *module UNUSED,
					nx_value_t *retval,
					int32_t num_arg,
					nx_value_t *args)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_finfo_t finfo;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    pool = nx_pool_create_core();
    if ( (rv = apr_stat(&finfo, args[0].string->buf, APR_FINFO_INODE, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to determine file inode of '%s'", args[0].string->buf);
	retval->defined = FALSE;
	apr_pool_destroy(pool);
	return;
    }

    retval->defined = TRUE;
    retval->integer = (int64_t) finfo.inode;

    apr_pool_destroy(pool);
}



void nx_expr_func__xm_fileop_dir_temp_get(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					  nx_module_t *module UNUSED,
					  nx_value_t *retval,
					  int32_t num_arg,
					  nx_value_t *args UNUSED)
{
    apr_status_t rv;
    apr_pool_t *pool;
    const char *tmpdir;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    retval->type = NX_VALUE_TYPE_STRING;
    pool = nx_pool_create_core();
    if ( (rv = apr_temp_dir_get(&tmpdir, pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "failed to get temp directory");
	retval->defined = FALSE;
	apr_pool_destroy(pool);
	return;
    }

    retval->defined = TRUE;
    retval->string = nx_string_create(tmpdir, -1);
    apr_pool_destroy(pool);
}



void nx_expr_func__xm_fileop_dir_exists(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					nx_module_t *module UNUSED,
					nx_value_t *retval,
					int32_t num_arg,
					nx_value_t *args)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_finfo_t finfo;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("'%s' type argument is invalid",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_BOOLEAN;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    retval->defined = TRUE;
    retval->boolean = FALSE;

    pool = nx_pool_create_core();
    rv = apr_stat(&finfo, args[0].string->buf, APR_FINFO_TYPE, pool);
    if ( rv == APR_SUCCESS )
    {
	if ( finfo.filetype == APR_DIR )
	{
	    retval->boolean = TRUE;
	}
    }
    else if ( APR_STATUS_IS_ENOENT(rv) )
    {
    }
    else
    {
	log_aprerror(rv, "failed to check whether directory '%s' exists", args[0].string->buf);
    }
    apr_pool_destroy(pool);
}
