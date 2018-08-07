/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "error_debug.h"
#include "logdata.h"
#include "serialize.h"
#include "exception.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

/*
 * Serialized logdata:
 *  flags
 *  datalen
 *  data
 *  num_fields
 *  fields
 *   field_keylen
 *   field_key
 *   field_datalen
 *   field_datatype
 *   field_data
 */

apr_size_t nx_logdata_serialized_size(const nx_logdata_t *logdata)
{
    apr_size_t size = 0;
    nx_logdata_field_t *field;

    size += 2; //num_fields

    for ( field = NX_DLIST_FIRST(&(logdata->fields));
	  field != NULL;
	  field = NX_DLIST_NEXT(field, link) )
    {
	size += 2; // keylen
	size += strlen(field->key); //key
	size += nx_value_serialized_size(field->value);
    }

    return ( size );
}



apr_size_t nx_logdata_to_membuf(const nx_logdata_t *logdata, char *buf, apr_size_t bufsize)
{
    char *ptr;
    apr_uint16_t num_field = 0;
    char *num_field_ptr;
    nx_logdata_field_t *field;
    apr_int16_t keylen;
    apr_size_t valsize;

    ASSERT(logdata != NULL);
    ASSERT(buf != NULL);

    ptr = buf;
    num_field_ptr = ptr;
    ptr += 2; //skip num_field, will be stored after the loop so we don't have to iterate twice

    //log_info("=>fields");
    for ( field = NX_DLIST_FIRST(&(logdata->fields));
	  field != NULL;
	  field = NX_DLIST_NEXT(field, link) )
    { // fields
	keylen = (apr_int16_t) strlen(field->key);
	nx_int16_to_le(ptr, &keylen);
	ptr += 2;
	memcpy(ptr, field->key, (size_t) keylen);
	ptr += keylen;
	//log_info("keylen: %u", keylen);
	//log_info("fieldname: '%s'", field->key);
	valsize = nx_value_to_membuf(field->value, ptr, bufsize - (apr_size_t) (ptr - buf));
	if ( valsize == 0 )
	{
	    return ( 0 ); // not enough space
	}
	ptr += valsize;
	num_field++;
    }
    nx_int16_to_le(num_field_ptr, &num_field); // num_field

    //log_info("<=fields (%u)", num_field);

    return ( (apr_size_t) (ptr - buf) );
}



nx_logdata_t *nx_logdata_from_membuf(const char *buf,
				     apr_size_t bufsize,
				     apr_size_t *bytes)
{
    nx_logdata_t *retval;
    apr_size_t offs = 0;
    apr_size_t vlen;
    apr_uint16_t fields;
    nx_value_t *val = NULL;
    char *key = NULL;
    apr_uint16_t keylen;
    apr_int16_t i;
    nx_exception_t e;

    if ( bufsize <= 1 )
    {
	throw_msg("not enough data to decode serialized logdata");
    }
    
    retval = malloc(sizeof(nx_logdata_t));
    ASSERT(retval != NULL);
    memset(retval, 0, sizeof(nx_logdata_t));
    NX_DLIST_INIT(&(retval->fields), nx_logdata_field_t, link);

    fields = nx_int16_from_le(buf);
    offs += 2;

    //log_info("=>fields (%u)", fields);
    try
    {
	for ( i = 0; i < fields; i++ )
	{
	    val = NULL;
	    key = NULL;
	    if ( offs >= bufsize )
	    {
		throw_msg("not enough data to decode serialized logdata field");
	    }
	    keylen = nx_int16_from_le(buf + offs);
	    //log_info("keylen: %u", keylen);
	    if ( keylen <= 0 )
	    {
		throw_msg("serialized logdata field name length is <= 0");
	    }
	    offs += 2;
	    if ( offs + (apr_size_t) keylen >= bufsize )
	    {
		throw_msg("not enough data to decode serialized field name (got %d, need %d)",
			  (int) (bufsize - offs), (int) keylen);
	    }

	    key = malloc((size_t) keylen + 1);
	    ASSERT(key != NULL);
	    apr_cpystrn(key, buf + offs, (apr_size_t) keylen + 1);
	    offs += (apr_size_t) keylen;
	    //log_info("fieldname: '%s'", key);

	    val = nx_value_from_membuf(buf + offs, bufsize - offs, &vlen);
	    ASSERT(val != NULL);
	    offs += vlen;

	    if ( (strcmp(key, "raw_event") == 0) && (val->type == NX_VALUE_TYPE_STRING) )
	    {
		retval->raw_event = val->string;
	    }
	    nx_logdata_append_field_value(retval, key, val);
	    free(key);
	}
	//log_info("<=fields (%u)", fields);
    }
    catch(e)
    {
	if ( key != NULL )
	{
	    free(key);
	}
	if ( val != NULL )
	{
	    nx_value_free(val);
	}
	nx_logdata_free(retval);
	rethrow(e);
    }

    if ( bytes != NULL )
    {
	*bytes = offs;
    }

    if ( retval->raw_event == NULL )
    {
	retval->raw_event = nx_string_new();
    }
    ASSERT(retval->raw_event->buf != NULL);
    return ( retval );
}

