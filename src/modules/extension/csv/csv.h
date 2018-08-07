/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_CSV_H
#define __NX_CSV_H

#include "../../../common/types.h"
#include "../../../common/logdata.h"
#include "../../../common/module.h"

typedef enum nx_csv_quote_method_t
{
    NX_CSV_QUOTE_METHOD_STRING = 1,
    NX_CSV_QUOTE_METHOD_ALL,
    NX_CSV_QUOTE_METHOD_NONE,
} nx_csv_quote_method_t;

typedef struct nx_csv_ctx_t
{
    char delimiter;
    char quotechar;
    char escapechar;
    nx_csv_quote_method_t quote_method;
    boolean escape_control;
    int num_field;
    int num_type;
    const char *fields[NX_MODULE_MAX_FIELDS];
    nx_value_type_t types[NX_MODULE_MAX_FIELDS];
    const char *undefvalue;
} nx_csv_ctx_t;

void nx_csv_ctx_set_quotechar(nx_csv_ctx_t *ctx, const char quotechar);
void nx_csv_ctx_set_delimiter(nx_csv_ctx_t *ctx, const char delimiter);
void nx_csv_ctx_set_escapechar(nx_csv_ctx_t *ctx, const char escapechar);
void nx_csv_ctx_init(nx_csv_ctx_t *ctx);
char nx_csv_get_config_char(const char *str);
void nx_csv_parse(nx_logdata_t *logdata,
		  nx_csv_ctx_t *ctx,
		  const char *src,
		  size_t len);
nx_string_t *nx_logdata_to_csv(nx_csv_ctx_t *ctx, nx_logdata_t *logdata);
void nx_csv_ctx_set_fields(nx_csv_ctx_t *ctx, char *fields);
void nx_csv_ctx_set_types(nx_csv_ctx_t *ctx, char *types);

#endif /* __NX_CSV_H */
