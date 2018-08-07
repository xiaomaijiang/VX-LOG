/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>
#include <stdlib.h>

#include "error_debug.h"
#include "exception.h"
#include "str.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

static uint32_t _string_limit = NX_STRING_DEFAULT_LIMIT;


uint32_t nx_string_get_limit()
{
    return ( _string_limit );
}



/*
 * Frees only dynamic part of the string, not itself
 */
void nx_string_kill(nx_string_t *string)
{
    ASSERT(string != NULL);
    
    if ( string->flags & NX_STRING_FLAG_CONST )
    {
	return;
    }
    if ( string->buf != NULL )
    {
	free(string->buf);
	string->buf = NULL;
    }
}



/*
 * Free the whole structure including dynamic parts
 */
void nx_string_free(nx_string_t *string)
{
    ASSERT(string != NULL);

    nx_string_kill(string);
    free(string);
}



nx_string_t *nx_string_new()
{
    nx_string_t *retval;

    retval = malloc(sizeof(nx_string_t));
    retval->buf = malloc(NX_STRING_DEFAULT_SIZE);
    *(retval->buf) = '\0';
    retval->bufsize = NX_STRING_DEFAULT_SIZE;
    retval->len = 0;
    retval->flags = 0;

    return ( retval );
}



nx_string_t *nx_string_new_size(size_t len)
{
    nx_string_t *retval;

    if ( len == 0 )
    {
	return ( nx_string_new() );
    }

    if ( len > _string_limit )
    {
	throw_msg("oversized string, limit is %d bytes", _string_limit);
    }

    retval = malloc(sizeof(nx_string_t));
    retval->buf = malloc(len);
    *(retval->buf) = '\0';
    retval->bufsize = (uint32_t) len;
    retval->len = 0;
    retval->flags = 0;

    return ( retval );
}



nx_string_t *nx_string_create(const char *src, int len)
{
    nx_string_t *retval;

    ASSERT(src != NULL);

    if ( len < 0 )
    {
	len = (int) strlen(src);
    }
    if ( len > (int) _string_limit )
    {
	throw_msg("oversized string, limit is %d bytes", _string_limit);
    }

    retval = malloc(sizeof(nx_string_t));
    retval->buf = malloc((size_t) len + 1);
    retval->bufsize = (uint32_t) len + 1;
    retval->len = (uint32_t) len;
    retval->flags = 0;
    memcpy(retval->buf, src, (size_t) len);
    retval->buf[len] = '\0';

    return ( retval );
}



nx_string_t *nx_string_create_owned(char *src, int len)
{
    nx_string_t *retval;

    ASSERT(src != NULL);

    if ( len < 0 )
    {
	len = (int) strlen(src);
    }
    if ( len > (int) _string_limit )
    {
	throw_msg("oversized string, limit is %d bytes", _string_limit);
    }

    retval = malloc(sizeof(nx_string_t));
    retval->buf = src;
    retval->bufsize = (uint32_t) len + 1;
    retval->len = (uint32_t) len;
    retval->flags = 0;

    return ( retval );
}



nx_string_t *nx_string_init_const(nx_string_t *dst, const char *src)
{
    ASSERT(dst != NULL);
    ASSERT(src != NULL);

    dst->buf = (char *)src;
    dst->len = (uint32_t) strlen(src);
    dst->bufsize = 0;
    dst->flags = NX_STRING_FLAG_CONST;

    return ( dst );
}



static void ensure_size(nx_string_t *dst, size_t len)
{
    apr_size_t newsize;
    
    if ( dst->bufsize > len )
    {
	return;
    }

    if ( len > _string_limit )
    {
	throw_msg("string limit (%d bytes) reached", _string_limit);
    }

    if ( dst->bufsize < 1024 )
    {
	newsize = dst->bufsize * 2;
    }
    else
    {
	newsize = (dst->bufsize * 3) / 2;
	if ( newsize > _string_limit )
	{
	    newsize = _string_limit;
	}
    }
    if ( newsize <= len )
    {
	newsize = len;
    }

    ASSERT(dst->buf != NULL);

    dst->buf = realloc(dst->buf, dst->bufsize + len);
    dst->bufsize += (uint32_t) len;
}



void nx_string_ensure_size(nx_string_t *str, size_t len)
{
    ASSERT(str != NULL);
    ASSERT(len > 0);

    ensure_size(str, len);
}



static void set_len(nx_string_t *dst, size_t len)
{
    if ( (dst->bufsize > 1024) && (dst->bufsize > len) )
    {
	free(dst->buf);
	dst->buf = malloc(len);
	dst->bufsize = (uint32_t) len;
	dst->flags = 0;
    }
    else if ( dst->bufsize <= len )
    {
	ensure_size(dst, len);
    }
}



/*
 * srclen is -1 to calculate with strlen
 * dst must be initialized
 */

nx_string_t *nx_string_append(nx_string_t *dst, const char *src, int srclen)
{
    ASSERT(dst != NULL);
    ASSERT(src != NULL);
    ASSERT(!(dst->flags & NX_STRING_FLAG_CONST));

    if ( srclen < 0 )
    {
	srclen = (int) strlen(src);
    }
    else if ( srclen == 0 )
    {
	return ( dst );
    }

    if ( dst->len + (size_t) srclen + 1 > _string_limit )
    {
	uint32_t len;

	len = _string_limit - 1;
	if ( dst->len < len )
	{
	    ensure_size(dst, len + 1);
	    memcpy(dst->buf + dst->len, src, (apr_size_t) (len - dst->len));
	    dst->len = len;
	    dst->buf[dst->len] = '\0';
	    log_warn("string limit (%u) exceeded while trying to append", _string_limit);
	}
	// else silently discard since we have probably warned already
    }
    else
    {
	ensure_size(dst, dst->len + (size_t) srclen + 1);
        memcpy(dst->buf + dst->len, src, (apr_size_t) srclen);
	dst->len += (uint32_t) srclen;
	dst->buf[dst->len] = '\0';
    }

    return ( dst );
}



nx_string_t *nx_string_prepend(nx_string_t *dst, const char *src, int srclen)
{
    ASSERT(dst != NULL);
    ASSERT(src != NULL);
    ASSERT(!(dst->flags & NX_STRING_FLAG_CONST));

    if ( srclen < 0 )
    {
	srclen = (int) strlen(src);
    }
    else if ( srclen == 0 )
    {
	return ( dst );
    }

    if ( dst->len + (size_t) srclen + 1 > _string_limit )
    {
	uint32_t len;

	len = _string_limit - 1;
	if ( dst->len < len )
	{
	    ensure_size(dst, len + 1);
	    if ( srclen > _string_limit )
	    {
		memcpy(dst->buf, src, (size_t) len);
	    }
	    else
	    {
		memmove(dst->buf + srclen, dst->buf, (size_t) len - srclen);
		memcpy(dst->buf, src, (size_t) srclen);
	    }
	    dst->len = len;
	    dst->buf[dst->len] = '\0';
	    log_warn("string limit (%u) exceeded while trying to preppend", _string_limit);
	}
	// else silently discard since we have probably warned already
    }
    else
    {
	ensure_size(dst, dst->len + (size_t) srclen + 1);
	memmove(dst->buf + srclen, dst->buf, (size_t) dst->len);
	memcpy(dst->buf, src, (size_t) srclen);
	dst->len += (uint32_t) srclen;
	dst->buf[dst->len] = '\0';
    }
    return ( dst );
}



nx_string_t *nx_string_set(nx_string_t *dst, const char *src, int srclen)
{
    ASSERT(dst != NULL);
    ASSERT(src != NULL);
    ASSERT(!(dst->flags & NX_STRING_FLAG_CONST));

    if ( srclen < 0 )
    {
	srclen = (int) strlen(src);
    }

    set_len(dst, (size_t) srclen + 1);
    if ( srclen > 0 )
    {
	memcpy(dst->buf, src, (apr_size_t) srclen);
    }
    dst->len = (uint32_t) srclen;
    dst->buf[dst->len] = '\0';

    return ( dst );
}



nx_string_t *nx_string_clone(const nx_string_t *str)
{
    nx_string_t *retval;
    uint32_t len;

    ASSERT(str != NULL);
    ASSERT(str->buf != NULL);
    
    len = str->len;
    if ( len > _string_limit )
    {
	log_warn("truncating oversized string (%u) to StringLimit (%u) in nx_string_clone()", len, _string_limit);
	len = _string_limit - 1;
    }
    retval = nx_string_new_size(len + 1);
    retval->flags = 0;
    retval->len = len;
    memcpy(retval->buf, str->buf, len);
    retval->buf[len] = '\0';

    return ( retval );
}



nx_string_t *nx_string_sprintf(nx_string_t 	*str,
			       const char	*fmt,
			       ...)
{
    int len;
    va_list ap;
    nx_string_t *retval;

    va_start(ap, fmt);
    len = apr_vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if ( str != NULL )
    {
	retval = str;
	set_len(str, (size_t) len + 1);
    }
    else
    {
	retval = nx_string_new_size((size_t) len + 1);
    }
    va_start(ap, fmt);
    ASSERT(apr_vsnprintf(retval->buf, (apr_size_t) len + 1, fmt, ap) == len);
    va_end(ap);
    retval->len = (uint32_t) len;

    return ( retval );
}



nx_string_t *nx_string_sprintf_append(nx_string_t 	*str,
				      const char	*fmt,
				      ...)
{
    int len;
    va_list ap;

    ASSERT(str != NULL);
    va_start(ap, fmt);
    len = apr_vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if ( str->len + (size_t) len + 1 > _string_limit )
    {
	len = _string_limit - 1;
	if ( str->len < len )
	{
	    log_warn("string limit (%u) exceeded while trying to append", _string_limit);
	}
    }

    ensure_size(str, str->len + (size_t) len + 1);
    
    va_start(ap, fmt);
    ASSERT(apr_vsnprintf(str->buf + str->len, (apr_size_t) len + 1, fmt, ap) == len);
    va_end(ap);
    str->len += (uint32_t) len;

    return ( str );
}



nx_string_t *nx_string_escape(nx_string_t *str)
{
    size_t i;

    ASSERT(str != NULL);

    for ( i = 0; i < str->len; i++ )
    {
	if ( !(str->buf[i] & 0x80) )
	{
	    switch ( str->buf[i] )
	    {
		case '\0':
		case '\\':
		case '\'':
		case '"':
		    if ( str->bufsize <= str->len + 1 )
		    {
			ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 1, str->buf + i, str->len - i);
		    str->buf[i] = '\\';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		    break;
		case '\n':
		    if ( str->bufsize <= str->len + 1 )
		    {
			ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		    str->buf[i] = '\\';
		    str->buf[i + 1] = 'n';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		    break;
		case '\r':
		    if ( str->bufsize <= str->len + 1 )
		    {
			ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		    str->buf[i] = '\\';
		    str->buf[i + 1] = 'r';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		    break;
		case '\t':
		    if ( str->bufsize <= str->len + 1 )
		    {
			ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		    str->buf[i] = '\\';
		    str->buf[i + 1] = 't';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		    break;
		case '\b':
		    if ( str->bufsize <= str->len + 1 )
		    {
			ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		    str->buf[i] = '\\';
		    str->buf[i + 1] = 'b';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		    break;
		default:
		    break;
	    }
	}
    }

    return ( str );
}


/* return the new length */
size_t nx_string_unescape_c(char *str)
{
    size_t i;
    size_t len;

    ASSERT(str != NULL);

    len = strlen(str);
    for ( i = 0; str[i] != '\0'; i++ )
    {
	if ( str[i] == '\\' )
	{
	    switch ( str[i + 1] )
	    {
		case '\\':
		    memmove(str + i + 1, str + i + 2, len - i - 1);
		    len--;
		    break;
		case '"':
		    memmove(str + i, str + i + 1, len - i);
		    len--;
		    break;
		case 'n':
		    str[i] = '\n';
		    memmove(str + i + 1, str + i + 2, len - i - 1);
		    len--;
		    break;
		case 'r':
		    str[i] = '\r';
		    memmove(str + i + 1, str + i + 2, len - i - 1);
		    len--;
		    break;
		case 't':
		    str[i] = '\t';
		    memmove(str + i + 1, str + i + 2, len - i - 1);
		    len--;
		    break;
		case 'b':
		    str[i] = '\b';
		    memmove(str + i + 1, str + i + 2, len - i - 1);
		    len--;
		    break;
		case 'x':
		case 'X':
		    if ( apr_isxdigit(str[i + 2]) && apr_isxdigit(str[i + 3]) )
		    {
			int c = 0;
			if ( (str[i + 2] >= '0') && (str[i + 2] <= '9') )
			{
			    c = str[i + 2] - '0';
			}
			else if ( (str[i + 2] >= 'a') && (str[i + 2] <= 'f') )
			{
			    c = str[i + 2] - 'a' + 10;
			}
			else if ( (str[i + 2] >= 'A') && (str[i + 2] <= 'F') )
			{
			    c = str[i + 2] - 'A' + 10;
			}
			c *= 16;

			if ( (str[i + 3] >= '0') && (str[i + 3] <= '9') )
			{
			    c += str[i + 3] - '0';
			}
			else if ( (str[i + 3] >= 'a') && (str[i + 3] <= 'f') )
			{
			    c += str[i + 3] - 'a' + 10;
			}
			else if ( (str[i + 3] >= 'A') && (str[i + 3] <= 'F') )
			{
			    c += str[i + 3] - 'A' + 10;
			}
			str[i] = (char) c;
			memmove(str + i + 1, str + i + 4, len - i - 3);
		    }
		    len -= 3;
		    break;
		default:
		    break;
	    }
	}
    }

    return ( len );
}



/**
 * Finds the next UTF-8 character in the string after p.
 */

char *nx_utf8_find_next_char(char *p,
			     char *end)
{
    ASSERT(p != NULL);

    if ( *p )
    {
	if ( end )
	{
	    for ( ++p; p < end && (*p & 0xc0) == 0x80; ++p );
	}
	else
	{
	    for ( ++p; (*p & 0xc0) == 0x80; ++p );
	}
    }
    return ( ((p == end) ? NULL : p) );
}



/**
 * Check if a utf-8 character is valid
 */

boolean nx_utf8_is_valid_char(const char *src,
			      int32_t length)
{
    char a;
    const char *srcptr = src + length;

    ASSERT(src != NULL);
    ASSERT(length >= 0);

    switch ( length )
    {
	default:
	    return ( FALSE );
	case 4:
	    if ( ((a = (*--srcptr)) < 0x80) || (a > 0xBF) )
	    {
		return ( FALSE );
	    }
	case 3:
	    if ( ((a = (*--srcptr)) < 0x80) || (a > 0xBF) )
	    {
		return ( FALSE );
	    }
	case 2:
	    if ( (a = (*--srcptr)) > 0xBF )
	    {
		return ( FALSE );
	    }

	    switch ( *src )
	    {
		case 0xE0:
		    if ( a < 0xA0 )
		    {
			return ( FALSE );
		    }
		    break;
		case 0xED:
		    if ( a > 0x9F )
		    {
			return ( FALSE );
		    }
		    break;
		case 0xF0:
		    if ( a < 0x90 )
		    {
			return ( FALSE );
		    }
		    break;
		case 0xF4:
		    if ( a > 0x8F )
		    {
			return ( FALSE );
		    }
		    break;
		default:
		    if ( a < 0x80 )
		    {
			return ( FALSE );
		    }
	    }

	case 1:
	    if ( (*src >= 0x80) && (*src < 0xC2) )
	    {
		    return ( FALSE );
	    }
    }
    if ( *src > 0xF4 )
    {
	return ( FALSE );
    }
    return ( TRUE );
}

#define NX_UNICODE_LAST_CHAR 0x10ffff
#define NX_UNICODE_SUR_HIGH_START 0xD800
#define NX_UNICODE_SUR_LOW_END 0xDFFF

static const int32_t _nx_trailing_bytes_for_utf8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,0,0
};

static const uint32_t _nx_offsets_from_utf8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 
						   0x03C82080UL, 0xFA082080UL, 0x82082080UL };


#define nx_utf8_next_char(p) (char *)((p) + _nx_trailing_bytes_for_utf8[(int32_t )(*p)] + 1)


boolean nx_string_validate_utf8(nx_string_t *str, boolean needfix, boolean throw)
{
    char *srcptr, *srcend;
    int32_t bytestoread;
    uint32_t chr;
    char *dstptr = NULL;
    boolean valid = TRUE;

    ASSERT(str != NULL);

    srcend = str->buf + str->len;
    srcptr = str->buf;

    while ( srcptr < srcend )
    {
	chr = 0;
	bytestoread = _nx_trailing_bytes_for_utf8[(int32_t) *srcptr];

    	if ( srcptr + bytestoread + 1 > srcend )
	{
	    if ( throw == TRUE )
	    {
		throw_msg("incomplete utf-8 byte sequence at end of input at pos %lld",
			  (long long int) (srcptr - str->buf));
	    }
	    else
	    {
		if ( needfix == TRUE )
		{
		    valid = FALSE;

		    dstptr = srcptr;
		    srcptr = nx_utf8_find_next_char(srcptr, srcend);
		    if ( srcptr == NULL )
		    {
			while ( dstptr < srcend )
			{
			    *dstptr++ = '?';
			}
			break;
		    }
		    else
		    {
			while ( dstptr < srcptr )
			{
			    *dstptr++ = '?';
			}
			continue;
		    }
		}
		else
		{
		    return ( FALSE );
		}
	    }
	}

	if ( nx_utf8_is_valid_char(srcptr, bytestoread + 1) == FALSE )
	{
	    if ( throw == TRUE )
	    {
		throw_msg("invalid utf-8 byte sequence at end of input at pos %lld",
			  (long long int) (srcptr - str->buf));
	    }
	    else
	    {
		if ( needfix == TRUE )
		{
		    valid = FALSE;

		    dstptr = srcptr;
		    srcptr = nx_utf8_find_next_char(srcptr, srcend);
		    if ( srcptr == NULL )
		    {
			while ( dstptr < srcend )
			{
			    *dstptr++ = '?';
			}
			break;
		    }
		    else
		    {
			while ( dstptr < srcptr )
			{
			    *dstptr++ = '?';
			}
			continue;
		    }
		}
		else
		{
		    return ( FALSE );
		}
	    }
	}

	switch ( bytestoread )
	{
	    case 5:
		chr += *srcptr++;
		chr <<= 6;
	    case 4:
		chr += *srcptr++;
		chr <<= 6;
	    case 3:
		chr += *srcptr++;
		chr <<= 6;
	    case 2:
		chr += *srcptr++;
		chr <<= 6;
	    case 1:
		chr += *srcptr++;
		chr <<= 6;
	    case 0:
		chr += *srcptr++;
		break;
	    default :
		nx_panic("invalid value in bytestoread (%d)", bytestoread);
	}

	chr -= _nx_offsets_from_utf8[bytestoread];

	if ( chr <= NX_UNICODE_LAST_CHAR )
	{
	    if ( ((chr >= NX_UNICODE_SUR_HIGH_START) && (chr <= NX_UNICODE_SUR_LOW_END))
		 || (chr == 0xFFFE) || (chr == 0xFFFF) )
	    {
		if ( throw == TRUE )
		{
		    throw_msg("invalid utf-8 byte sequence at pos %d",
			      (int) (srcptr - bytestoread - 1 - str->buf));
		}
		else
		{
		    if ( needfix == TRUE )
		    {
			valid = FALSE;

			dstptr = srcptr - bytestoread - 1;

			srcptr = nx_utf8_find_next_char(srcptr, srcend);
			if ( srcptr == NULL )
			{
			    while ( dstptr < srcend )
			    {
				*dstptr++ = '?';
			    }
			    break;
			}
			else
			{
			    while ( dstptr < srcptr )
			    {
				*dstptr++ = '?';
			    }
			    continue;
			}
		    }
		    else
		    {
			return ( FALSE );
		    }
		}
	    }
	}
	else
	{
	    if ( throw == TRUE )
	    {
		throw_msg("invalid utf-8 byte sequence at pos %d",
			  (int) (srcptr - bytestoread - 1 - str->buf));
	    }
	    else
	    {
		if ( needfix == TRUE )
		{
		    valid = FALSE;
		    dstptr = srcptr - bytestoread - 1;

		    srcptr = nx_utf8_find_next_char(srcptr, srcend);
		    if ( srcptr == NULL )
		    {
			while ( dstptr < srcend )
			{
			    *dstptr++ = '?';
			}
			break;
		    }
		    else
		    {
			while ( dstptr < srcptr )
			{
			    *dstptr++ = '?';
			}
			continue;
		    }
		}
		else
		{
		    return ( FALSE );
		}
	    }
	}
    }

    return ( valid );
}



nx_string_t *nx_string_strip_crlf(nx_string_t *str)
{
    ASSERT(str != NULL);

    if ( str->len > 0 )
    {
	if ( str->buf[str->len - 1] == APR_ASCII_LF )
	{
	    str->buf[str->len - 1] = '\0';
	    (str->len)--;
	    if ( str->len > 0 )
	    {
		if ( str->buf[str->len - 1] == APR_ASCII_CR )
		{
		    str->buf[str->len - 1] = '\0';
		    (str->len)--;
		}
	    }
	}
    }

    return ( str );
}
