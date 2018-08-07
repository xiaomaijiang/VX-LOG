/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_CONFIG_CACHE_H
#define __NX_CONFIG_CACHE_H

#include <apr_file_io.h>
#include "types.h"
#include "value.h"

#define NX_CONFIG_CACHE_VERSION "NX-CC-VERSION 001"

typedef struct nx_cc_item_t
{
    boolean		used;
    char		*key;
    nx_value_t		*value;
    apr_off_t		offs;		///< file offset in config cache
    boolean		needflush;	///< TRUE when item was updated since last write
} nx_cc_item_t;

boolean nx_config_cache_get_string(const char *module, const char *key, const char **result);
boolean nx_config_cache_get_int(const char *module, const char *key, int64_t *result);
void nx_config_cache_set_int(const char *module, const char *key, int64_t value);
void nx_config_cache_set_string(const char *module, const char *key, const char *data);
void nx_config_cache_remove(const char *module, const char *key);
void nx_config_cache_read();
void nx_config_cache_write();
void nx_config_cache_free();

#endif	/* __NX_CONFIG_CACHE_H */
