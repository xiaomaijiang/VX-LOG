/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_LOGDATA_H
#define __NX_LOGDATA_H

#include <apr_file_io.h>
#include <apr_network_io.h>

#include "types.h"
#include "value.h"
#include "dlist.h"

#define NX_LOGDATA_DEFAULT_BUFSIZE 150

#define NX_LOGDATA_BINARY_HEADER "\0NX\0"

typedef struct nx_logdata_field_t
{
    NX_DLIST_ENTRY(nx_logdata_field_t) link; ///< logdata key-value pairs are linked together in a list
    char			*key;
    nx_value_t			*value;
} nx_logdata_field_t;


typedef struct nx_logdata_field_list_t nx_logdata_field_list_t;
NX_DLIST_HEAD(nx_logdata_field_list_t, nx_logdata_field_t);


typedef struct nx_logdata_t
{
    NX_DLIST_ENTRY(nx_logdata_t) link; ///< all messages are linked together in a queue
    nx_string_t *raw_event; ///< shortcut to the raw_event field
    nx_logdata_field_list_t fields;	///< linked list of key-value pairs
} nx_logdata_t;

nx_logdata_t *nx_logdata_new_logline(const char *ptr, int len);
nx_logdata_t *nx_logdata_new();
void nx_logdata_free(nx_logdata_t *logdata);
void nx_logdata_field_free(nx_logdata_field_t *field);
void nx_logdata_append_logline(nx_logdata_t *logdata, 
			       const char *ptr,
			       int len);
nx_logdata_t *nx_logdata_clone(nx_logdata_t *logdata);
void nx_logdata_append_field_value(nx_logdata_t *logdata,
				   const char *key,
				   nx_value_t *value);
void nx_logdata_set_field_value(nx_logdata_t *logdata,
				const char *key,
				nx_value_t *value);
void nx_logdata_set_field(nx_logdata_t *logdata, nx_logdata_field_t *setfield);
void nx_logdata_rename_field(nx_logdata_t *logdata,
			     const char *old,
			     const char *new);
boolean nx_logdata_get_field_value(const nx_logdata_t *logdata,
				   const char *key,
				   nx_value_t *value);
nx_logdata_field_t *nx_logdata_get_field(const nx_logdata_t *logdata,
					 const char *key);
boolean nx_logdata_delete_field(nx_logdata_t *logdata,
				const char *key);
void nx_logdata_set_datetime(nx_logdata_t *logdata,
			     const char *key,
			     apr_time_t datetime);
void nx_logdata_set_string(nx_logdata_t *logdata,
			   const char *key,
			   const char *value);
void nx_logdata_set_integer(nx_logdata_t *logdata,
			    const char *key,
			    int64_t value);
void nx_logdata_set_binary(nx_logdata_t *logdata,
			   const char *key,
			   const char *value,
			   unsigned int len);
void nx_logdata_set_boolean(nx_logdata_t *logdata,
			    const char *key,
			    boolean value);
void nx_logdata_dump_fields(nx_logdata_t *logdata);

// serialize
apr_size_t nx_logdata_serialized_size(const nx_logdata_t *logdata);
apr_size_t nx_logdata_to_membuf(const nx_logdata_t *logdata, char *buf, apr_size_t bufsize);
nx_logdata_t *nx_logdata_from_membuf(const char *buf,
				     apr_size_t bufsize,
				     apr_size_t *bytes);

#endif	/* __NX_LOGDATA_H */
