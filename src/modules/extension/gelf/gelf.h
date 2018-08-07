/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_GELF_H
#define __NX_GELF_H

#include "../../../common/str.h"
#include "../../../common/logdata.h"

#include "../../extension/json/yajl/api/yajl_gen.h"
#include "../../extension/json/yajl/api/yajl_parse.h"

typedef struct nx_gelf_ctx_t
{
    yajl_gen g;
    nx_string_t *gelfstr;
    nx_logdata_t *logdata;
    int in_array;
    int in_map;
    char *key;
    unsigned int shortmessagelength;
} nx_gelf_ctx_t;

nx_string_t *nx_logdata_to_gelf(nx_gelf_ctx_t *ctx);

#endif	/* __NX_GELF_H */
