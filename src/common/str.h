/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_STRING_H
#define __NX_STRING_H

#include "types.h"

#define NX_STRING_DEFAULT_SIZE 128
#define NX_STRING_DEFAULT_LIMIT 1024*1024 /* =1M will throw an exception over this limit */

typedef enum nx_string_flag_t
{
    NX_STRING_FLAG_NONE	= 0,
    NX_STRING_FLAG_CONST = 1 << 1, // buf is not allocated by malloc, should not be free()-d
} nx_string_flag_t;

typedef struct nx_string_t
{
    nx_string_flag_t	flags;
    char		*buf;		///< NUL terminated string
    uint32_t		bufsize;	///< size of buf
    uint32_t		len;		///< length of string in bytes not including the terminating NUL
} nx_string_t;

uint32_t nx_string_get_limit();
void nx_string_kill(nx_string_t *string);
void nx_string_free(nx_string_t *string);
nx_string_t *nx_string_new();
nx_string_t *nx_string_new_size(size_t len);
nx_string_t *nx_string_create(const char *src, int len);
nx_string_t *nx_string_create_owned(char *src, int len);
nx_string_t *nx_string_init_const(nx_string_t *dst, const char *src);
void nx_string_ensure_size(nx_string_t *str, size_t len);
nx_string_t *nx_string_append(nx_string_t *dst, const char *src, int srclen);
nx_string_t *nx_string_prepend(nx_string_t *dst, const char *src, int srclen);
nx_string_t *nx_string_set(nx_string_t *dst, const char *src, int srclen);
nx_string_t *nx_string_clone(const nx_string_t *str);
nx_string_t *nx_string_sprintf(nx_string_t 	*str,
			       const char	*fmt,
			       ...) PRINTF_FORMAT(2, 3);
nx_string_t *nx_string_sprintf_append(nx_string_t 	*str,
				      const char	*fmt,
				      ...);
nx_string_t *nx_string_escape(nx_string_t *str);
size_t nx_string_unescape_c(char *str);
boolean nx_string_validate_utf8(nx_string_t *str, boolean needfix, boolean throw);
nx_string_t *nx_string_strip_crlf(nx_string_t *str);

char *nx_utf8_find_next_char(char *p,
			     char *end);
boolean nx_utf8_is_valid_char(const char *src,
			      int32_t length);

#endif	/* __NX_STRING_H */
