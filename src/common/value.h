/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_VALUE_H
#define __NX_VALUE_H

#include <pcre.h>
#include <apr_file_io.h>
#include "types.h"
#include "str.h"


typedef enum nx_value_type_t
{
    NX_VALUE_TYPE_INTEGER = 1,
    NX_VALUE_TYPE_STRING,
    NX_VALUE_TYPE_DATETIME,
    NX_VALUE_TYPE_REGEXP,
    NX_VALUE_TYPE_BOOLEAN,
    NX_VALUE_TYPE_IP4ADDR,
    NX_VALUE_TYPE_IP6ADDR,
    NX_VALUE_TYPE_BINARY,
    NX_VALUE_TYPE_UNKNOWN,
} nx_value_type_t;

typedef struct nx_binary_t
{
    char *value;
    unsigned int len;
} nx_binary_t;

typedef struct nx_value_t
{
    nx_value_type_t	type;
    boolean		defined;
    union
    {
	int64_t		integer;
	nx_string_t	*string;
	apr_time_t	datetime;
	struct nx_value_regexp
	{
	    char	*str;
	    char	*replacement; 	///< only used for s/a/b/ regexp replacement, not serialized!
	    pcre	*pcre;
	    size_t	pcre_size;
	    uint8_t	modifiers;	///< TODO: not serialized, do we need it to?
	} regexp;
	boolean		boolean;
	uint8_t		ip4addr[4];
	uint8_t    	ip6addr[16];
	nx_binary_t 	binary;
    };
} nx_value_t;

#define nx_value_init(value) memset(value, 0, sizeof(nx_value_t)); (value)->defined = TRUE;
void nx_value_free(nx_value_t *value);
void nx_value_kill(nx_value_t *value);
nx_value_t *nx_value_clone(nx_value_t *dst, const nx_value_t *value);
nx_value_t *nx_value_new(nx_value_type_t type);
nx_value_t *nx_value_init_string(nx_value_t *value, const char *str);
nx_value_t *nx_value_new_string(const char *str);
nx_value_t *nx_value_new_regexp(const char *str);
nx_value_t *nx_value_init_integer(nx_value_t *value, int64_t integer);
nx_value_t *nx_value_new_integer(int64_t integer);
nx_value_t *nx_value_init_datetime(nx_value_t *value, apr_time_t datetime);
nx_value_t *nx_value_new_datetime(apr_time_t datetime);
nx_value_t *nx_value_new_boolean(boolean value);
nx_value_t *nx_value_new_binary(const char *value, unsigned int len);
const char *nx_value_type_to_string(nx_value_type_t type);
nx_value_type_t nx_value_type_from_string(const char *str);
char *nx_value_to_string(nx_value_t *value);
nx_value_t *nx_value_from_string(const char *string, nx_value_type_t type);
apr_size_t nx_value_serialized_size(const nx_value_t *value);
apr_size_t nx_value_to_membuf(const nx_value_t *value, char *buf, apr_size_t bufsize);
nx_value_t *nx_value_from_membuf(const char *buf,
				 apr_size_t bufsize,
				 apr_size_t *bytes);
apr_size_t nx_value_size(const nx_value_t *value);
boolean nx_value_eq(const nx_value_t *v1, const nx_value_t *v2);
apr_status_t nx_value_to_file(nx_value_t *value, apr_file_t *file);
int64_t nx_value_parse_int(const char *string);
nx_value_t *nx_value_parse_ip4addr(nx_value_t *retval, const char *string);
void nx_bin2ascii(const char *src, unsigned int srclen, char *dst);
void nx_ascii2bin(const char *src, unsigned int srclen, char *dst);


#endif	/* __NX_VALUE_H */
