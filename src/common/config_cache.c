/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../core/nxlog.h"
#include "error_debug.h"
#include "config_cache.h"
#include "module.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define NX_CONFIG_CACHE_LIMIT (1024*1024*100) /* 100 Mb */
#define NX_CONFIG_CACHE_MAX_KEYLEN (APR_PATH_MAX + 64)

static void _free_item(nx_cc_item_t *item)
{
    ASSERT(item != NULL);

    
    if ( item->value != NULL )
    {
	nx_value_free(item->value);
    }
    if ( item->key != NULL )
    {
	free(item->key);
    }
    
    free(item);
}



static void _update_item(nx_cc_item_t *item)
{
    nx_ctx_t *ctx;
    apr_off_t offs;
    nx_exception_t e;

    ASSERT(item->value != NULL);

    ctx = nx_ctx_get();

    CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
    try
    {
	if ( ctx->ccfile == NULL )
	{
	    CHECKERR_MSG(apr_file_open(&(ctx->ccfile), ctx->ccfilename,
				       APR_READ | APR_WRITE | APR_CREATE | APR_TRUNCATE,
				       APR_OS_DEFAULT, ctx->pool),
			 "couldn't open config cache '%s' for writing", ctx->ccfilename);
	}
	// only integers are supported for now, string modifications trigger a whole rewrite
	if ( (item->offs > 0) && (item->value->type == NX_VALUE_TYPE_INTEGER) )

	{
	    offs = item->offs;
	    CHECKERR_MSG(apr_file_seek(ctx->ccfile, APR_SET, &offs),
			 "failed to seek in %s file", ctx->ccfilename);
	    CHECKERR_MSG(nx_value_to_file(item->value, ctx->ccfile), "failed to update config cache item");
	}
	else
	{ // write the whole file, not very efficient
	    nx_config_cache_write();
	}
	item->needflush = FALSE;
    }
    catch(e)
    {
	CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));
	rethrow(e);
    }
    CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));
}



static void _write_item(apr_file_t *file, nx_cc_item_t *item)
{
    nx_value_t key;
    nx_string_t str;
    nx_exception_t e;

    ASSERT(file != NULL);
    ASSERT(item != NULL);
    ASSERT(item->key != NULL);
    ASSERT(item->value != NULL);

    nx_value_init(&key);
    key.type = NX_VALUE_TYPE_STRING;
    nx_string_init_const(&str, item->key);
    key.string = &str;

    try
    {
	CHECKERR_MSG(nx_value_to_file(&key, file), "couldn't write key to cache file");
	if ( item->value->type == NX_VALUE_TYPE_INTEGER )
	{ // get offset for integer values so that updates can be done efficiently	    
	    item->offs = 0;
	    CHECKERR_MSG(apr_file_seek(file, APR_CUR, &(item->offs)), "failed to seek in cache file");
	}
	CHECKERR_MSG(nx_value_to_file(item->value, file), "couldn't write value to cache file");
    }
    catch(e)
    {
	nx_value_kill(&key);
	rethrow(e);
    }
}



static void _read_item(char * volatile buf,
		       apr_size_t volatile bufsize,
		       apr_hash_t *cache,
		       apr_size_t *bytesread)
{
    nx_cc_item_t *item = NULL;
    nx_cc_item_t *tmpitem = NULL;
    nx_value_t * volatile key = NULL;
    nx_value_t *value = NULL;
    apr_size_t readsize;
    nx_exception_t e;

    ASSERT(buf != NULL);
    ASSERT(cache != NULL);

    item = malloc(sizeof(nx_cc_item_t));
    memset(item, 0, sizeof(nx_cc_item_t));

    try
    {
	key = nx_value_from_membuf(buf, bufsize, &readsize);
	if ( (key == NULL) || (key->type != NX_VALUE_TYPE_STRING) )
	{
	    throw_msg("failed to read key from config cache");
	}
	buf = buf + readsize;
	*bytesread = readsize;
	bufsize -= readsize;
	item->key = strdup(key->string->buf);
	nx_value_free(key);
	key = NULL;

	value = nx_value_from_membuf(buf, bufsize, &readsize);
	if ( value == NULL )
	{
	    throw_msg("failed to read value from config cache");
	}
	item->value = value;

	if ( item->value->type == NX_VALUE_TYPE_INTEGER )
	{
	    log_debug("read config cache item: %s=%ld", item->key, item->value->integer);
	}
	else if ( item->value->type == NX_VALUE_TYPE_STRING )
	{
	    log_debug("read config cache item: %s=%s", item->key, item->value->string->buf);
	}
	else
	{
	    log_debug("read config cache item: %s", item->key);
	}

	*bytesread += readsize;
    }
    catch(e)
    {
	if ( item != NULL )
	{
	    _free_item(item);
	}
	if ( key != NULL )
	{
	    nx_value_free(key);
	}
	if ( value != NULL )
	{
	    nx_value_free(value);
	}
	rethrow(e);
    }

    tmpitem = (nx_cc_item_t *) apr_hash_get(cache, item->key, APR_HASH_KEY_STRING);
    ASSERT(tmpitem == NULL);
    apr_hash_set(cache, item->key, APR_HASH_KEY_STRING, (void *) item);
}



/**
 * This is not thread safe, must ensure that modules are not running,
 * otherwise lock config_cache_mutex
 */
void nx_config_cache_write()
{
    nx_ctx_t *ctx;
    nx_cc_item_t *item;
    nx_value_t *version = NULL;
    nx_exception_t e;
    apr_hash_index_t *idx;
    const char *key;
    apr_ssize_t keylen;
    apr_status_t rv;

    ctx = nx_ctx_get();

    ASSERT(ctx != NULL);
    if ( ctx->nocache == TRUE )
    {
	log_debug("NoCache is TRUE, not writing config cache");
	return;
    }

    if ( apr_hash_count(ctx->config_cache) == 0 )
    { // no items to write
	log_debug("no entries found, not writing configcache.dat");
	return;
    }
    log_debug("nx_config_cache_write()");
    
    if ( ctx->ccfile == NULL )
    {
	rv = apr_file_open(&(ctx->ccfile), ctx->ccfilename,
			   APR_READ | APR_WRITE | APR_CREATE | APR_TRUNCATE,
			   APR_OS_DEFAULT, ctx->pool);
	if ( rv != APR_SUCCESS )
	{
	    log_aprerror(rv, "couldn't open config cache '%s' for writing", ctx->ccfilename);
	    return;
	}
    }
    else
    {
	CHECKERR_MSG(apr_file_trunc(ctx->ccfile, 0), "failed to truncate '%s'",
		     ctx->ccfilename);
    }
    version = nx_value_new_string(NX_CONFIG_CACHE_VERSION);
    try
    {
	CHECKERR_MSG(nx_value_to_file(version, ctx->ccfile),
		     "couldn't write version to cache file");

	idx = apr_hash_first(NULL, ctx->config_cache);

	while ( idx != NULL )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, (void **) &item);
	    idx = apr_hash_next(idx);
	    if ( item->used == FALSE )
	    { // only write items which have been read or written
		continue;
	    }
	    _write_item(ctx->ccfile, item);
	    item->needflush = FALSE;
	}
    }
    catch(e)
    {
	nx_value_free(version);
	rethrow(e);
    }

    log_debug("config cache written to %s", ctx->ccfilename);
}



void nx_config_cache_read()
{
    nx_ctx_t *ctx;
    apr_off_t offset = 0;
    char *buf = NULL;
    apr_status_t rv;
    apr_size_t nbytes, bytesread, bytesleft;
    nx_value_t *version = NULL;
    apr_file_t *ccfile;
    nx_exception_t e;

    ctx = nx_ctx_get();

    if ( ctx->nocache == TRUE )
    {
	return;
    }

    log_debug("reading config cache from %s", ctx->ccfilename);
    rv = apr_file_open(&ccfile, ctx->ccfilename,
		       APR_READ, APR_OS_DEFAULT, ctx->pool);
    if ( APR_STATUS_IS_ENOENT(rv) )
    { // nothing to read
	return;
    }
    CHECKERR_MSG(rv, "couldn't open config cache '%s'", ctx->ccfilename);

    try
    {
	CHECKERR_MSG(apr_file_seek(ccfile, APR_END, &offset),
		     "couldn't seek in cache file");
	if ( offset > NX_CONFIG_CACHE_LIMIT )
	{
	    throw_msg("cache file is too large (%d bytes)", offset);
	}
	if ( offset > 0 )
	{
	    nbytes = (apr_size_t) offset;
	    
	    buf = malloc((size_t) offset);

	    // seek back to start
	    offset = 0;
	    CHECKERR_MSG(apr_file_seek(ccfile, APR_SET, &offset),
			 "couldn't seek in cache file");

	    // read cache file into memory
	    if ( (rv = apr_file_read_full(ccfile, buf, nbytes, &bytesread)) != APR_SUCCESS )
	    {
		if ( rv != APR_EOF )
		{
		    CHECKERR_MSG(rv, "failed to read config cache file");
		}
	    }
	    
	    if ( nbytes != bytesread )
	    {
		throw_msg("read %d bytes, wanted %d", (int) bytesread, (int) nbytes);
	    }
	    bytesleft = nbytes;
	    
	    version = nx_value_from_membuf(buf, nbytes, &bytesread);
	    if ( version->type != NX_VALUE_TYPE_STRING )
	    {
		throw_msg("string expected for config cache version");
	    }
	    if ( strcmp(version->string->buf, NX_CONFIG_CACHE_VERSION) != 0 )
	    {
		throw_msg("config cache version mismatch, expected %s, got %s",
			  NX_CONFIG_CACHE_VERSION, version->string->buf);
	    }
	    
	    bytesleft -= bytesread;
	    while ( bytesleft > 0 )
	    {
		_read_item(buf + (nbytes - bytesleft), bytesleft,
			   ctx->config_cache, &bytesread);
		ASSERT(bytesread <= bytesleft);
		bytesleft -= bytesread;
	    }
	}
    }
    catch(e)
    {
	apr_file_close(ccfile);
	apr_file_remove(ctx->ccfilename, NULL);
	if ( buf != NULL )
	{
	    free(buf);
	}
	if ( version != NULL )
	{
	    nx_value_free(version);
	}
	rethrow_msg(e, "failed to read config cache");
    }

    if ( buf != NULL )
    {
	free(buf);
    }
    if ( version != NULL )
    {
	nx_value_free(version);
    }
    apr_file_close(ccfile);
    //apr_file_remove(ctx->ccfilename, NULL);
}



boolean nx_config_cache_get_int(const char *module, const char *key, int64_t *result)
{
    nx_ctx_t *ctx;
    nx_cc_item_t *item;
    char ckey[NX_CONFIG_CACHE_MAX_KEYLEN];

    ASSERT(module != NULL);
    ASSERT(key != NULL);

    if ( apr_snprintf(ckey, sizeof(ckey), "%s/%s", module, key) == sizeof(ckey) )
    {
	nx_panic("config cache key too long, limit is %d bytes", NX_CONFIG_CACHE_MAX_KEYLEN);
    }

    ctx = nx_ctx_get();

    CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
    item = (nx_cc_item_t *) apr_hash_get(ctx->config_cache, ckey, APR_HASH_KEY_STRING);
    CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));

    if ( item == NULL )
    {
	return ( FALSE );
    }

    if ( (strcmp(item->key, ckey) == 0) && (item->value->type == NX_VALUE_TYPE_INTEGER) )
    {
	*result = item->value->integer;
	item->used = TRUE;
	return ( TRUE );
    }
    
    return ( FALSE );
}



boolean nx_config_cache_get_string(const char *module, const char *key, const char **result)
{
    nx_ctx_t *ctx;
    nx_cc_item_t *item;
    char ckey[NX_CONFIG_CACHE_MAX_KEYLEN];

    ASSERT(module != NULL);
    ASSERT(key != NULL);

    ctx = nx_ctx_get();

    if ( apr_snprintf(ckey, sizeof(ckey), "%s/%s", module, key) == sizeof(ckey) )
    {
	nx_panic("config cache key too long, limit is %d bytes", NX_CONFIG_CACHE_MAX_KEYLEN);
    }

    CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
    item = (nx_cc_item_t *) apr_hash_get(ctx->config_cache, ckey, APR_HASH_KEY_STRING);
    CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));

    if ( item == NULL )
    {
	return ( FALSE );
    }

    if ( (strcmp(item->key, ckey) == 0) && (item->value->type == NX_VALUE_TYPE_STRING) )
    {
	ASSERT(item->value->string != NULL);
	*result = item->value->string->buf;
	item->used = TRUE;
	return ( TRUE );
    }

    return ( FALSE );
}



/**
 * Set an integer value in the config cache.
 */
void nx_config_cache_set_int(const char *module, const char *key, int64_t value)
{
    nx_ctx_t *ctx;
    nx_cc_item_t *item = NULL;
    char ckey[NX_CONFIG_CACHE_MAX_KEYLEN];

    ASSERT(module != NULL);
    ASSERT(key != NULL);

    ctx = nx_ctx_get();

    if ( apr_snprintf(ckey, sizeof(ckey), "%s/%s", module, key) == sizeof(ckey) )
    {
	nx_panic("config cache key too long, limit is %d bytes", NX_CONFIG_CACHE_MAX_KEYLEN);
    }

    CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
    item = (nx_cc_item_t *) apr_hash_get(ctx->config_cache, ckey, APR_HASH_KEY_STRING);
    CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));

    if ( item != NULL )
    {
	ASSERT(item->value->type == NX_VALUE_TYPE_INTEGER);

	item->value->integer = value;
	item->used = TRUE;
	item->needflush = TRUE;
    }
    else
    {
	item = malloc(sizeof(nx_cc_item_t));
	memset(item, 0, sizeof(nx_cc_item_t));
	item->key = strdup(ckey);
	item->value = nx_value_new_integer(value);
	item->used = TRUE;
	item->needflush = TRUE;
	CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
	apr_hash_set(ctx->config_cache, item->key, APR_HASH_KEY_STRING, (void *) item);
	CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));
    }
}



void nx_config_cache_set_string(const char *module, const char *key, const char *value)
{
    nx_ctx_t *ctx;
    nx_cc_item_t *item;
    char ckey[NX_CONFIG_CACHE_MAX_KEYLEN];

    ASSERT(module != NULL);
    ASSERT(key != NULL);
    ASSERT(value != NULL);

    ctx = nx_ctx_get();

    if ( apr_snprintf(ckey, sizeof(ckey), "%s/%s", module, key) == sizeof(ckey) )
    {
	nx_panic("config cache key too long, limit is %d bytes", NX_CONFIG_CACHE_MAX_KEYLEN);
    }

    CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
    item = (nx_cc_item_t *) apr_hash_get(ctx->config_cache, ckey, APR_HASH_KEY_STRING);
    CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));

    if ( item != NULL )
    {
	ASSERT(item->value->type == NX_VALUE_TYPE_STRING);
	nx_value_free(item->value);
	item->value = nx_value_new_string(value);
	item->used = TRUE;
	item->needflush = TRUE;
    }
    else
    {
	item = malloc(sizeof(nx_cc_item_t));
	memset(item, 0, sizeof(nx_cc_item_t));
	item->key = strdup(ckey);
	item->value = nx_value_new_string(value);
	item->used = TRUE;
	item->needflush = TRUE;
	CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
	apr_hash_set(ctx->config_cache, item->key, APR_HASH_KEY_STRING, (void *) item);
	CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));
    }
}



void nx_config_cache_free()
{
    nx_ctx_t *ctx;
    nx_cc_item_t *item = NULL;
    apr_hash_index_t *idx;
    const char *key;
    apr_ssize_t keylen;

    ctx = nx_ctx_get();

    CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
    for ( idx = apr_hash_first(NULL, ctx->config_cache);
	  idx != NULL;
	  idx = apr_hash_next(idx) )
    {
	apr_hash_this(idx, (const void **) &key, &keylen, (void **) &item);
	apr_hash_set(ctx->config_cache, item->key, APR_HASH_KEY_STRING, NULL);
	_free_item(item);
    }
    if ( ctx->ccfile != NULL )
    {
	apr_file_close(ctx->ccfile);
	ctx->ccfile = NULL;
    }
    CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));
}



/**
 * Delete a value from the config cache.
 */
void nx_config_cache_remove(const char *module, const char *key)
{
    nx_ctx_t *ctx;
    nx_cc_item_t *item = NULL;
    char ckey[NX_CONFIG_CACHE_MAX_KEYLEN];

    ASSERT(module != NULL);
    ASSERT(key != NULL);

    ctx = nx_ctx_get();

    if ( apr_snprintf(ckey, sizeof(ckey), "%s/%s", module, key) == sizeof(ckey) )
    {
	nx_panic("config cache key too long, limit is %d bytes", NX_CONFIG_CACHE_MAX_KEYLEN);
    }

    CHECKERR(apr_thread_mutex_lock(ctx->config_cache_mutex));
    apr_hash_set(ctx->config_cache, ckey, APR_HASH_KEY_STRING, NULL);
    CHECKERR(apr_thread_mutex_unlock(ctx->config_cache_mutex));
}
