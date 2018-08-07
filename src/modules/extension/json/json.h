/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_JSON_H
#define __NX_JSON_H

#include "../../../common/str.h"
#include "../../../common/logdata.h"

#include "yajl/api/yajl_gen.h"
#include "yajl/api/yajl_parse.h"

typedef struct nx_json_parser_ctx_t
{
    yajl_gen g;
    nx_string_t *tmpstr;
    nx_logdata_t *logdata;
    int in_array;
    int in_map;
    char *key;
} nx_json_parser_ctx_t;

void nx_json_parse(nx_json_parser_ctx_t *ctx,
		   const char *json, size_t len);
nx_string_t *nx_logdata_to_json(nx_json_parser_ctx_t *ctx);

#endif	/* __NX_JSON_H */
