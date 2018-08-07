/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>
#include <pcre.h>

#include "error_debug.h"
#include "value.h"
#include "date.h"
#include "exception.h"
#include "../core/nxlog.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


static nx_keyval_t value_types[] =
{
    { NX_VALUE_TYPE_INTEGER, "integer" },
    { NX_VALUE_TYPE_STRING, "string" },
    { NX_VALUE_TYPE_DATETIME, "datetime" },
    { NX_VALUE_TYPE_REGEXP, "regexp" },
    { NX_VALUE_TYPE_BOOLEAN, "boolean" },
    { NX_VALUE_TYPE_IP4ADDR, "ip4addr" },
    { NX_VALUE_TYPE_IP6ADDR, "ip6addr" },
    { NX_VALUE_TYPE_BINARY, "binary" },
    { NX_VALUE_TYPE_UNKNOWN, "unknown" },
    { 0, NULL },
};



/*
 * Frees only the dynamic part of the value, not itself
 */
void nx_value_kill(nx_value_t *value)
{
    ASSERT(value != NULL);

    if ( value->defined == TRUE )
    {
	switch ( value->type )
	{
	    case NX_VALUE_TYPE_STRING:
		if ( value->string != NULL )
		{
		    nx_string_free(value->string);
		    value->string = NULL;
		}
		break;
	    case NX_VALUE_TYPE_INTEGER:
	    case NX_VALUE_TYPE_DATETIME:
	    case NX_VALUE_TYPE_BOOLEAN:
	    case NX_VALUE_TYPE_IP4ADDR:
	    case NX_VALUE_TYPE_IP6ADDR:
		// self-contained, no need to free anything
		break;
	    case NX_VALUE_TYPE_BINARY:
		if ( value->binary.value != NULL )
		{
		    free(value->binary.value);
		}
		break;
	    case NX_VALUE_TYPE_REGEXP:
		if ( value->regexp.pcre != NULL )
		{
		    pcre_free(value->regexp.pcre);
		    value->regexp.pcre = NULL;
		}
		if ( value->regexp.str != NULL )
		{
		    free(value->regexp.str);
		    value->regexp.str = NULL;
		}
		if ( value->regexp.replacement != NULL )
		{
		    free(value->regexp.replacement);
		}
		break;    
	    default:
		nx_panic("invalid value type: %d", value->type);
	}
	value->defined = FALSE; // make sure we will not be called twice
    }
}


/*
 * Free the whole structure including dynamic parts
 */
void nx_value_free(nx_value_t *value)
{
    ASSERT(value != NULL);

    nx_value_kill(value);
    free(value);
}



nx_value_t *nx_value_clone(nx_value_t *dst, const nx_value_t *value)
{
    nx_value_t *retval = dst;

    ASSERT(value != NULL);

    if ( retval == NULL )
    {
	retval = malloc(sizeof(nx_value_t));
	ASSERT(retval != NULL);
    }

    if ( value->defined == TRUE )
    {
	switch ( value->type )
	{
	    case NX_VALUE_TYPE_STRING:
		retval->string = nx_string_clone(value->string);
		ASSERT(retval->string != NULL);
		break;
	    case NX_VALUE_TYPE_INTEGER:
		retval->integer = value->integer;
		break;
	    case NX_VALUE_TYPE_DATETIME:
		retval->datetime = value->datetime;
		break;
	    case NX_VALUE_TYPE_BOOLEAN:
		retval->boolean = value->boolean;
		break;
	    case NX_VALUE_TYPE_IP4ADDR:
		memcpy(retval->ip4addr, value->ip4addr, sizeof(uint8_t) * 4);
		break;
	    case NX_VALUE_TYPE_IP6ADDR:
		memcpy(retval->ip6addr, value->ip6addr, sizeof(uint16_t) * 8);
		break;
	    case NX_VALUE_TYPE_BINARY:
		ASSERT(value->binary.value != NULL);
		retval->binary.value = malloc(value->binary.len);
		ASSERT(retval->binary.value != NULL);
		memcpy(retval->binary.value, value->binary.value, value->binary.len);
		retval->binary.len = value->binary.len;
		break;    
	    case NX_VALUE_TYPE_REGEXP:
		ASSERT(value->regexp.pcre_size > 0);
		retval->regexp.pcre = malloc(value->regexp.pcre_size);
		memcpy(retval->regexp.pcre, value->regexp.pcre, value->regexp.pcre_size);
		retval->regexp.pcre_size = value->regexp.pcre_size;
		retval->regexp.str = strdup(value->regexp.str);
		if ( value->regexp.replacement == NULL )
		{
		    retval->regexp.replacement = NULL;
		}
		else
		{
		    retval->regexp.replacement = strdup(value->regexp.replacement);
		}
		retval->regexp.modifiers = value->regexp.modifiers;
		break;
	    default:
		nx_panic("invalid value type: %d", value->type);
	}
    }
    retval->defined = value->defined;
    retval->type = value->type;

    return ( retval );
}



nx_value_t *nx_value_init_string(nx_value_t *value, const char *str)
{
    ASSERT(str != NULL);
    ASSERT(value != NULL);

    value->type = NX_VALUE_TYPE_STRING;
    value->string = nx_string_create(str, -1);
    value->defined = TRUE;
    ASSERT(value->string != NULL);

    return ( value );
}



nx_value_t *nx_value_new_string(const char *str)
{
    nx_value_t *retval;

    ASSERT(str != NULL);

    retval = malloc(sizeof(nx_value_t));
    retval->type = NX_VALUE_TYPE_STRING;
    retval->string = nx_string_create(str, -1);
    retval->defined = TRUE;
    ASSERT(retval->string != NULL);

    return ( retval );
}



nx_value_t *nx_value_new_regexp(const char *str)
{
    nx_value_t *retval;
    pcre *regexp;
    const char *error = NULL;
    int erroroffs = 0;
    int rc;
    size_t size;

    ASSERT(str != NULL);

    retval = malloc(sizeof(nx_value_t));
    retval->type = NX_VALUE_TYPE_REGEXP;
    retval->defined = TRUE;

    regexp = pcre_compile(str, 0, &error,  &erroroffs, NULL);

    if ( regexp == NULL )
    {
	throw_msg("failed to compile regular expression '%s', error at position %d: %s",
		  str, erroroffs, error);
    }

    rc = pcre_fullinfo(regexp, NULL, PCRE_INFO_SIZE, &size); 
    if ( rc < 0 )
    {
	free(retval);
	pcre_free(regexp);
	throw_msg("failed to get compiled regexp size");
    }
    ASSERT(size > 0);

    retval->regexp.pcre = regexp;
    retval->regexp.pcre_size = (size_t) size;
    retval->regexp.str = strdup(str);

    return ( retval );
}



nx_value_t *nx_value_init_integer(nx_value_t *value, int64_t integer)
{
    ASSERT(value != NULL);

    value->type = NX_VALUE_TYPE_INTEGER;
    value->integer = integer;
    value->defined = TRUE;

    return ( value );
}



nx_value_t *nx_value_new_integer(int64_t integer)
{
    nx_value_t *retval;

    retval = malloc(sizeof(nx_value_t));
    ASSERT(retval != NULL);
    retval->type = NX_VALUE_TYPE_INTEGER;
    retval->integer = integer;
    retval->defined = TRUE;

    return ( retval );
}



nx_value_t *nx_value_init_datetime(nx_value_t *value, apr_time_t datetime)
{
    ASSERT(value != NULL);

    value->type = NX_VALUE_TYPE_DATETIME;
    value->datetime = datetime;
    value->defined = TRUE;

    return ( value );
}



nx_value_t *nx_value_new_datetime(apr_time_t datetime)
{
    nx_value_t *retval;

    retval = malloc(sizeof(nx_value_t));
    ASSERT(retval != NULL);
    retval->type = NX_VALUE_TYPE_DATETIME;
    retval->datetime = datetime;
    retval->defined = TRUE;

    return ( retval );
}



nx_value_t *nx_value_new_boolean(boolean value)
{
    nx_value_t *retval;

    retval = malloc(sizeof(nx_value_t));
    ASSERT(retval != NULL);
    retval->type = NX_VALUE_TYPE_BOOLEAN;
    retval->boolean = value;
    retval->defined = TRUE;

    return ( retval );
}



nx_value_t *nx_value_new(nx_value_type_t type)
{
    nx_value_t *retval;

    retval = malloc(sizeof(nx_value_t));
    memset(retval, 0, sizeof(nx_value_t));
    ASSERT(retval != NULL);
    retval->type = type;
    retval->defined = TRUE;

    return ( retval );
}



nx_value_t *nx_value_new_binary(const char *value, unsigned int len)
{
    nx_value_t *retval;
    char *val;

    ASSERT(len > 0);
    retval = malloc(sizeof(nx_value_t));
    ASSERT(retval != NULL);
    val = malloc((size_t) len);
    ASSERT(val != NULL);
    memcpy(val, value, (size_t)len);
    retval->type = NX_VALUE_TYPE_BINARY;
    retval->binary.value = val;
    retval->binary.len = len;
    retval->defined = TRUE;
    
    return ( retval );
}



const char *nx_value_type_to_string(nx_value_type_t type)
{
    int i;

    for ( i = 0; value_types[i].value != NULL; i++ )
    {
	if ( (int) type == value_types[i].key )
	{
	    return ( value_types[i].value );
	}
    }

    nx_panic("invalid value type: %d", type);

    return ( "invalid" );
}



nx_value_type_t nx_value_type_from_string(const char *str)
{
    int i;

    for ( i = 0; value_types[i].value != NULL; i++ )
    {
	if ( strcasecmp(value_types[i].value, str) == 0 )
	{
	    return ( value_types[i].key );
	}
    }
    throw_msg("invalid type: %s", str);
}



void nx_bin2ascii(const char *src, unsigned int srclen, char *dst)
{
    static char hex[] = { "0123456789abcdef" };
    unsigned int i;

    ASSERT(src != NULL);
    ASSERT(dst != NULL);

    for ( i = 0; i < srclen; i++ )
    {
	dst[2 * i] = hex[src[i] >> 4];
	dst[2 * i + 1] = hex[src[i] & 0x0f];
    }
    dst[2 * i] = '\0';
}



void nx_ascii2bin(const char *src, unsigned int srclen, char *dst)
{
    unsigned int i;
    int hexval = 0;

    ASSERT(src != NULL);
    ASSERT(dst != NULL);

    if ( srclen % 2 != 0 )
    {
	throw_msg("binary has odd encoding length");
    }
    for ( i = 0; i < srclen; i += 2 )
    {
	hexval = 0;
	if ( (src[i] >= '0') && (src[i] <= '9') )
	{
	    hexval += (src[i] - '0');
	}
	else if ( (src[i] >= 'a') && (src[i] <= 'f') )
	{
	    hexval += src[i] - 'a' + 10;
	}
	else
	{
	    throw_msg("invalid character '%c' in encoded binary", src[i]);
	}
	hexval = hexval << 4;

	if ( (src[i + 1] >= '0') && (src[i + 1] <= '9') )
	{
	    hexval += (src[i + 1] - '0');
	}
	else if ( (src[i + 1] >= 'a') && (src[i + 1] <= 'f') )
	{
	    hexval += src[i + 1] - 'a' + 10;
	}
	
	dst[i / 2] = (char) hexval;
    }
}



/**
 * \return NULL if the value is undefined
 */

char *nx_value_to_string(nx_value_t *value)
{
    char *retval = NULL;
    nx_exception_t e;
    int i;

    ASSERT(value != NULL);
    
    if ( value->defined == FALSE )
    {
	return ( NULL );
    }

    try
    {
	switch ( value->type )
	{
	    case NX_VALUE_TYPE_INTEGER:
		retval = malloc(32);
		apr_snprintf(retval, 31, "%"APR_INT64_T_FMT, value->integer);
		break;
	    case NX_VALUE_TYPE_STRING:
		ASSERT(value->string != NULL);
		ASSERT(value->string->buf != NULL);
		retval = strdup(value->string->buf);
		break;
	    case NX_VALUE_TYPE_DATETIME:
		retval = malloc(20);
		CHECKERR_MSG(nx_date_to_iso(retval, 20, value->datetime),
			     "failed to convert time value to string");
		break;
	    case NX_VALUE_TYPE_REGEXP:
		retval = strdup(value->regexp.str);
		break;
	    case NX_VALUE_TYPE_BOOLEAN:
		if ( value->boolean == TRUE )
		{
		    retval = strdup("TRUE");
		}
		else
		{
		    retval = strdup("FALSE");
		}
		break;
	    case NX_VALUE_TYPE_IP4ADDR:
		retval = malloc(20);
		apr_snprintf(retval, 20, "%d.%d.%d.%d", value->ip4addr[0], value->ip4addr[1],
			     value->ip4addr[2], value->ip4addr[3]);
		break;
	    case NX_VALUE_TYPE_IP6ADDR:
	    {
#if APR_HAVE_IPV6 && HAVE_APR_SOCKADDR_IP_GETBUF
		apr_sockaddr_t sockaddr;

		memset(&sockaddr, 0, sizeof(apr_sockaddr_t));
		sockaddr.family = AF_INET6;
		sockaddr.ipaddr_ptr = &(sockaddr.sa.sin6.sin6_addr);
		sockaddr.sa.sin6.sin6_family = AF_INET6;
		for ( i = 0; i < 16; i++ )
		{
		    sockaddr.sa.sin6.sin6_addr.s6_addr[i] = value->ip6addr[i];
		}
		
		retval = malloc(64);
		
		CHECKERR_MSG(apr_sockaddr_ip_getbuf(retval, 64, &sockaddr),
		"couldn't convert IPv6 address to string");
#else
		throw_msg("cannot convert IPv6 value to string, no IPv6 support");
#endif
		break;    
	    }
	    case NX_VALUE_TYPE_BINARY:
		retval = malloc((size_t) value->binary.len * 2 + 1);
		nx_bin2ascii(value->binary.value, value->binary.len, retval);
		break;
	    default:
		nx_panic("invalid value type: %d", value->type);
	}
    }
    catch(e)
    {
	if ( retval != NULL )
	{
	    free(retval);
	}
	rethrow(e);
    }

    return ( retval );
}



int64_t nx_value_parse_int(const char *string)
{
    int i = 0;
    int base = 10;
    int mul = 1;
    int sign = 1;
    int len;
    long int val;
    char tmp[128];

    len = (int) strlen(string);
    ASSERT(len > 0);

    if ( string[0] == '-' )
    {
	sign = -1;
	i++;
	len--;
    }

    if ( strncasecmp(string + i, "0X", 2) == 0 )
    {
	i += 2;
	len -= 2;
	base = 16;
    }

    if ( !apr_isxdigit(string[i + len - 1]) )
    {
	switch ( apr_toupper(string[i + len - 1]) )
	{
	    case 'K':
		mul = 1024;
		break;
	    case 'M':
		mul = 1024 * 1024;
		break;
	    case 'G':
		mul = 1024 * 1024 * 1024;
		break;
	    case '\r':
		throw_msg("cannot parse integer \"%s\", invalid modifier: '\\r'", string);
	    case '\n':
		throw_msg("cannot parse integer \"%s\", invalid modifier: '\\n'", string);
	    default:
		throw_msg("cannot parse integer \"%s\", invalid modifier: '%c'", string, string[i + len - 1]);
	}
	len--;
    }
    if ( len > (int) sizeof(tmp) )
    {
	len = (int) sizeof(tmp) - 1;
    }
    apr_cpystrn(tmp, string + i, (apr_size_t) len + 1);
    if ( base == 16 )
    {
	if ( sscanf(tmp, "%lx", &val) != 1 )
	{
	    throw_msg("couldn't parse integer: %s", string);
	}
    }
    else
    {
	if ( sscanf(tmp, "%ld", &val) != 1 )
	{
	    throw_msg("couldn't parse integer: %s", tmp);
	}
    }

    return ( val * mul * sign );
}



nx_value_t *nx_value_parse_ip4addr(nx_value_t *retval, const char *string)
{
    int ip4addr[4] = { 0, 0, 0, 0 };
    int i = 0;
    const char *ptr;
    boolean got = FALSE;

    ptr = string;

    while ( !(*ptr == '\0') || (*ptr == ' ') )
    {
	if ( i > 3 )
	{
	    throw_msg("invalid ipv4 address: %s", string);
	}
	if ( apr_isdigit(*ptr) )
	{
	    ip4addr[i] *= 10;
	    ip4addr[i] += *ptr - '0';
	    got = TRUE;

	    if ( ip4addr[i] > 255 )
	    {
		throw_msg("invalid ipv4 address: %s", string);
	    }
	}
	else if ( *ptr == '.' )
	{
	    if ( got == FALSE )
	    {
		throw_msg("invalid ipv4 address: %s", string);
	    }
	    i++;
	    got = FALSE;
	}
	else
	{
	    throw_msg("invalid character '%c' in ipv4 address '%s'", *ptr, string);
	}
	ptr++;
    }
    if ( got == FALSE )
    {
	throw_msg("invalid ipv4 address: %s", string);
    }

    if ( retval == NULL)
    {
	retval = nx_value_new(NX_VALUE_TYPE_IP4ADDR);
    }
    retval->ip4addr[0] = (uint8_t) ip4addr[0];
    retval->ip4addr[1] = (uint8_t) ip4addr[1];
    retval->ip4addr[2] = (uint8_t) ip4addr[2];
    retval->ip4addr[3] = (uint8_t) ip4addr[3];

    return ( retval );
}


#if APR_HAVE_IPV6
// This is a hack because apr does not define this function in the headers, though available
int apr_inet_pton(int af, const char *src, void *dst);

nx_value_t *nx_value_parse_ip6addr(const char *string)
{
    nx_value_t *retval = NULL;
    struct in6_addr in6addr;
    int i;

    memset(&in6addr, 0, sizeof(struct in6_addr));
    if ( apr_inet_pton(AF_INET6, string, &in6addr) != 1 )
    {
	CHECKERR_MSG(APR_FROM_OS_ERROR(errno),"couldn't parse IPv6 address: %s", string);
    }

    retval = nx_value_new(NX_VALUE_TYPE_IP6ADDR);
    for ( i = 0; i < 16; i++ )
    {
	retval->ip6addr[i] = in6addr.s6_addr[i];
    }

    return ( retval );
}
#else
nx_value_t *nx_value_parse_ip6addr(const char *string)
{
    throw_msg("cannot parse IPv6 string, no IPv6 support in apr");
}
#endif



nx_value_t *nx_value_from_string(const char *string, nx_value_type_t type)
{
    nx_value_t * volatile retval = NULL;
    apr_time_t t;
    char *binary;
    unsigned int len;
    int64_t integer;
    nx_exception_t e;

    ASSERT(string != NULL);

    switch ( type )
    {
	case NX_VALUE_TYPE_INTEGER:
	    integer = nx_value_parse_int(string);
	    retval = nx_value_new(type);
	    retval->integer = integer;
	    break;
	case NX_VALUE_TYPE_STRING:
	    retval = nx_value_new_string(string);
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    if ( nx_date_parse(&t, string, NULL) != APR_SUCCESS )
	    {
		throw_msg("Couldn't parse datetime value: '%s'", string);
	    }
	    retval = nx_value_new(type);
	    retval->datetime = t;
	    break;
	case NX_VALUE_TYPE_BOOLEAN:
	    retval = nx_value_new(type);
	    if ( strcasecmp(string, "true") == 0 )
	    {
		retval->boolean = TRUE;
	    }
	    else if ( strcasecmp(string, "false") == 0 )
	    {
		retval->boolean = FALSE;
	    }
	    else
	    {
		nx_value_free(retval);
		throw_msg("Invalid boolean: %s", string);
	    }
	    break;
	case NX_VALUE_TYPE_REGEXP:
	    retval = nx_value_new_regexp(string);
	    break;
	case NX_VALUE_TYPE_IP4ADDR:
	    retval = nx_value_parse_ip4addr(NULL, string);
	    break;
	case NX_VALUE_TYPE_IP6ADDR:
	    retval = nx_value_parse_ip6addr(string);
	    break;    
	case NX_VALUE_TYPE_BINARY:
	    retval = nx_value_new(type);
	    len = (unsigned int) strlen(string);
	    binary = malloc(len / 2);
	    retval->binary.value = binary;
	    retval->binary.len = len;
	    try
	    {
		nx_ascii2bin(string, len, retval->binary.value);
	    }
	    catch(e)
	    {
		free(retval);
		free(binary);
		rethrow(e);
	    }
	    break;
	default:
	    nx_panic("invalid value type: %d", type);
    }

    ASSERT(retval != NULL);

    return ( retval );
}



apr_size_t nx_value_size(const nx_value_t *value)
{
    apr_size_t size = sizeof(nx_value_t);

    switch ( value->type )
    {
	case NX_VALUE_TYPE_INTEGER:
	    break;
	case NX_VALUE_TYPE_STRING:
	    size += value->string->len;
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    break;
	case NX_VALUE_TYPE_REGEXP:
	    nx_panic("FIXME");
	case NX_VALUE_TYPE_BOOLEAN:
	    break;
	case NX_VALUE_TYPE_IP4ADDR:
	    break;
	case NX_VALUE_TYPE_IP6ADDR:
	    break;
	case NX_VALUE_TYPE_BINARY:
	    size += value->binary.len;
	    break;
	case NX_VALUE_TYPE_UNKNOWN:
	    nx_panic("cannot serialize UNKNOWN type");
	default:
	    nx_panic("invalid type: %d", value->type);
    }

    return ( size );
}



boolean nx_value_eq(const nx_value_t *v1, const nx_value_t *v2)
{
    ASSERT(v1 != NULL);
    ASSERT(v2 != NULL);

    if ( v1->type != v2->type )
    {
	return ( FALSE );
    }
    if ( v1->defined != v2->defined )
    {
	return ( FALSE );
    }

    switch ( v1->type )
    {
	case NX_VALUE_TYPE_INTEGER:
	    if ( v1->integer != v2->integer )
	    {
		return ( FALSE );
	    }
	    break;
	case NX_VALUE_TYPE_STRING:
	    if ( strcmp(v1->string->buf, v2->string->buf) != 0 )
	    {
		return ( FALSE );
	    }
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    if ( v1->datetime != v2->datetime )
	    {
		return ( FALSE );
	    }
	    break;
	case NX_VALUE_TYPE_REGEXP:
	    ASSERT(v1->regexp.str != NULL);
	    ASSERT(v2->regexp.str != NULL);
	    if ( strcmp(v1->regexp.str, v2->regexp.str) != 0 )
	    {
		return ( FALSE );
	    }
	    break;
	case NX_VALUE_TYPE_BOOLEAN:
	    if ( v1->boolean != v2->boolean )
	    {
		return ( FALSE );
	    }
	    break;
	case NX_VALUE_TYPE_IP4ADDR:
	    if ( memcmp(v1->ip4addr, v2->ip4addr, sizeof(v1->ip4addr)) != 0 )
	    {
		return ( FALSE );
	    }
	    break;
	case NX_VALUE_TYPE_IP6ADDR:
	    if ( memcmp(v1->ip6addr, v2->ip6addr, sizeof(v1->ip6addr)) != 0 )
	    {
		return ( FALSE );
	    }
	    break;
	case NX_VALUE_TYPE_BINARY:
	    if ( v1->binary.len != v2->binary.len )
	    {
		return ( FALSE );
	    }
	    if ( memcmp(v1->binary.value, v2->binary.value, (size_t) v1->binary.len) != 0 )
	    {
		return ( FALSE );
	    }
	    break;
	case NX_VALUE_TYPE_UNKNOWN:
	    nx_panic("cannot compare UNKNOWN types");
	default:
	    nx_panic("invalid type: %d", v1->type);
    }

    return ( TRUE );
}
