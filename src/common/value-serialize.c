/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "error_debug.h"
#include "value.h"
#include "serialize.h"
#include "exception.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

/*
 * Serialized value:
 *  type
 *  defined
 *  len
 *  data
 */

apr_size_t nx_value_serialized_size(const nx_value_t *value)
{
    apr_size_t size = 0;

    size += 1; //type
    size += 1 ; //defined

    if ( value->defined == TRUE )
    {
	switch ( value->type )
	{
	    case NX_VALUE_TYPE_INTEGER:
		size += 8;
		break;
	    case NX_VALUE_TYPE_STRING:
		size += 4;
		size += value->string->len;
		break;
	    case NX_VALUE_TYPE_DATETIME:
		size += 8;
		break;
	    case NX_VALUE_TYPE_REGEXP:
		size += 4;
		size += strlen(value->regexp.str);
	    case NX_VALUE_TYPE_BOOLEAN:
		size += 1;
		break;
	    case NX_VALUE_TYPE_IP4ADDR:
		size += 4;
		break;
	    case NX_VALUE_TYPE_IP6ADDR:
		size += 16;
		break;
	    case NX_VALUE_TYPE_BINARY:
		size += 4;
		size += value->binary.len;
		break;
	    case NX_VALUE_TYPE_UNKNOWN:
		nx_panic("cannot serialize UNKNOWN type");
	    default:
		nx_panic("invalid type: %d", value->type);
	}
    }

    return ( size );
}


#define NX_VALUE_HEADER_LEN 2

apr_size_t nx_value_to_membuf(const nx_value_t *value, char *buf, apr_size_t bufsize)
{
    size_t len;
    char *ptr;
    apr_size_t retval = 0;
    apr_uint32_t len32;
    int64_t datetime;

    ASSERT(value != NULL);
    ASSERT(buf != NULL);

    if ( bufsize < NX_VALUE_HEADER_LEN )
    {
	log_debug("not enough space: %u < HEADER_LEN", (unsigned int) bufsize);
	return ( 0 ); // not enough space
    }

    ptr = buf;
    *ptr = (uint8_t) value->type;
    ptr++;
    *ptr = (uint8_t) value->defined;
    ptr++;

    if ( value->defined == TRUE )
    {
	switch ( value->type )
	{
	    case NX_VALUE_TYPE_INTEGER:
		if ( bufsize < NX_VALUE_HEADER_LEN + 8 )
		{
		    return ( 0 ); // not enough space
		}
		nx_int64_to_le(ptr, &(value->integer));
		ptr += 8;
		break;
	    case NX_VALUE_TYPE_STRING:
		len = value->string->len;
		len32 = (apr_uint32_t) len;
		if ( bufsize < NX_VALUE_HEADER_LEN + len + 4 )
		{
		    return ( 0 ); // not enough space
		}
		nx_int32_to_le(ptr, &len32);
		ptr += 4;
		memcpy(ptr, value->string->buf, len);
		ptr += len;
		break;
	    case NX_VALUE_TYPE_DATETIME:
		if ( bufsize < NX_VALUE_HEADER_LEN + 8 )
		{
		    return ( 0 ); // not enough space
		}
		datetime = value->datetime;
		nx_int64_to_le(ptr, &datetime);
		ptr += 8;
		break;
	    case NX_VALUE_TYPE_REGEXP:
		len = strlen(value->regexp.str);
		len32 = (apr_uint32_t) len;
		if ( bufsize < NX_VALUE_HEADER_LEN + len + 4 )
		{
		    return ( 0 ); // not enough space
		}
		nx_int32_to_le(ptr, &len32);
		ptr += 4;
		memcpy(ptr, value->regexp.str, len);
		ptr += len;
	    case NX_VALUE_TYPE_BOOLEAN:
		if ( bufsize < NX_VALUE_HEADER_LEN + 1 )
		{
		    return ( 0 ); // not enough space
		}
		*ptr = (uint8_t) value->boolean;
		ptr += 1;
		break;
	    case NX_VALUE_TYPE_IP4ADDR:
		if ( bufsize < NX_VALUE_HEADER_LEN + 4 )
		{
		    return ( 0 ); // not enough space
		}
		memcpy(ptr, value->ip4addr, 4);
		ptr += 4;
		break;
	    case NX_VALUE_TYPE_IP6ADDR:
		if ( bufsize < NX_VALUE_HEADER_LEN + 16 )
		{
		    return ( 0 ); // not enough space
		}
		memcpy(ptr, value->ip6addr, 16);
		ptr += 16;
		break;
	    case NX_VALUE_TYPE_BINARY:
		len = value->binary.len;
		len32 = (apr_uint32_t) len;
		if ( bufsize < NX_VALUE_HEADER_LEN + len + 4 )
		{
		    return ( 0 ); // not enough space
		}
		nx_int32_to_le(ptr, &len32);
		ptr += 4;
		memcpy(ptr, value->binary.value, len);
		ptr += len;
		break;
	    case NX_VALUE_TYPE_UNKNOWN:
		nx_panic("cannot serialize UNKNOWN type");
	    default:
		nx_panic("invalid type: %d", value->type);
	}
    }

    retval = (apr_size_t) (ptr - buf);
    
    return ( retval );
}



nx_value_t *nx_value_from_membuf(const char *buf,
				 apr_size_t bufsize,
				 apr_size_t *bytes)
{
    nx_value_t *retval;
    uint8_t defined;
    uint8_t type;
    apr_size_t offs;
    apr_uint32_t len;
    nx_exception_t e;

    ASSERT(buf != NULL);
    ASSERT(bufsize > 0 );

    if ( 2 > bufsize )
    {
	throw_msg("not enough data to decode serialized value (2 > bufsize)");
    }
    type = buf[0];
    defined = buf[1];
    offs = 2;
    
    retval = nx_value_new((nx_value_type_t) type);
    retval->defined = FALSE;

    try
    {
	if ( ((boolean) defined) == TRUE )
	{
	    switch ( retval->type )
	    {
		case NX_VALUE_TYPE_INTEGER:
		    if ( offs + 8 <= bufsize )
		    {
			retval->integer = (int64_t) nx_int64_from_le(buf + offs);
			offs += 8;
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized integer");
		    }
		    break;
		case NX_VALUE_TYPE_STRING:
		    if ( offs + 4 <= bufsize )
		    {
			len = nx_int32_from_le(buf + offs);
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized string length");
		    }
		    offs += 4;
		    if ( offs + len <= bufsize )
		    {
			retval->string = nx_string_create(buf + offs, (int) len);
			offs += len;
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized string buffer"
				  " (required: %d, available: %d)", len, bufsize - offs);
		    }
		    break;
		case NX_VALUE_TYPE_DATETIME:
		    if ( offs + 8 <= bufsize )
		    {
			retval->datetime = (apr_time_t) nx_int64_from_le(buf + offs);
			offs += 8;
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized datetime");
		    }
		break;
		case NX_VALUE_TYPE_REGEXP:
		    throw_msg("regexp should not need to be serialized");
		case NX_VALUE_TYPE_BOOLEAN:
		    if ( offs + 1 <= bufsize )
		    {
			retval->boolean = (boolean) buf[offs];
			offs++;
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized boolean value");
		    }
		    break;
		case NX_VALUE_TYPE_IP4ADDR:
		    if ( offs + 4 <= bufsize )
		    {
			memcpy(retval->ip4addr, buf + offs, 4);
			offs += 4;
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized ip4addr value");
		    }
		    break;
		case NX_VALUE_TYPE_IP6ADDR:
		    if ( offs + 16 <= bufsize )
		    {
			memcpy(retval->ip6addr, buf + offs, 16);
			offs += 16;
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized ip6addr value");
		    }
		    break;
		case NX_VALUE_TYPE_BINARY:
		    if ( offs + 4 <= bufsize )
		    {
			len = nx_int32_from_le(buf + offs);
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized binary length");
		    }
		    offs += 4;
		    if ( offs + len <= bufsize )
		    {
			retval->binary.value = malloc(len);
			ASSERT(retval->binary.value != NULL);
			memcpy(retval->binary.value, buf + offs, len);
			offs += len;
		    }
		    else
		    {
			throw_msg("not enough data to decode serialized binary buffer"
				  " (required: %d, available: %d)", len, bufsize - offs);
		    }
		    break;
		case NX_VALUE_TYPE_UNKNOWN:
		    throw_msg("UNKNOWN type not allowed in serialized binary format");
		default:
		    throw_msg("invalid type in serialized binary format: %d", retval->type);
	    }
	}
    }
    catch(e)
    {
	nx_value_free(retval);
	rethrow(e);
    }
    
    retval->defined = (boolean) defined;

    if ( bytes != NULL )
    {
	*bytes = offs;
    }

    return ( retval );
}



apr_status_t nx_value_to_file(nx_value_t *value, apr_file_t *file)
{
    apr_size_t size, bytes;
    char *buf = NULL;
    apr_status_t rv;

    ASSERT(value != NULL);
    ASSERT(file != NULL);

    size = nx_value_serialized_size(value);
    buf = malloc(size);
    ASSERT(buf != NULL);
    ASSERT(size == nx_value_to_membuf(value, buf, size));

    rv = apr_file_write_full(file, buf, size, &bytes);
    free(buf);

    return ( rv );
}
