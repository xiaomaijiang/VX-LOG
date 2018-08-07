/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "xm_csv.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static void xm_csv_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_xm_csv_conf_t *modconf;
    char tmpchar;
    boolean quote_optional = FALSE;
    boolean got_quote_optional = FALSE;

    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_xm_csv_conf_t));

    module->config = modconf;

    nx_csv_ctx_init(&(modconf->ctx));
    modconf->ctx.quote_method = 0;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "quotechar") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "QuoteChar needs a parameter");
	    }
	    tmpchar = nx_csv_get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid QuoteChar parameter: %s", curr->args);
	    }
	    modconf->ctx.quotechar = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "escapechar") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "EscapeChar needs a parameter");
	    }
	    tmpchar = nx_csv_get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid EscapeChar parameter: %s", curr->args);
	    }
	    modconf->ctx.escapechar = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "delimiter") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "Delimiter needs a parameter");
	    }
	    tmpchar = nx_csv_get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid Delimiter parameter: %s", curr->args);
	    }
	    modconf->ctx.delimiter = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "QuoteOptional") == 0 )
	{
	    log_warn("QuoteOptional has been deprecated, use 'QuoteMethod string' instead.");
	    nx_cfg_get_boolean(module->directives, "QouteOptional", &quote_optional);
	    got_quote_optional = TRUE;

	}
	else if ( strcasecmp(curr->directive, "QuoteMethod") == 0 )
	{
	    if ( modconf->ctx.quote_method != 0 )
	    {
		nx_conf_error(curr, "QuoteMethod already declared");
	    }
	    if ( strcasecmp(curr->args, "string") == 0 )
	    {
		modconf->ctx.quote_method = NX_CSV_QUOTE_METHOD_STRING;
	    }
	    else if ( strcasecmp(curr->args, "all") == 0 )
	    {
		modconf->ctx.quote_method = NX_CSV_QUOTE_METHOD_ALL;
	    }
	    else if ( strcasecmp(curr->args, "none") == 0 )
	    {
		modconf->ctx.quote_method = NX_CSV_QUOTE_METHOD_NONE;
	    }
	    else
	    {
		nx_conf_error(curr, "Invalid parameter for QuoteMethod: %s", curr->args);
	    }
	}
	else if ( strcasecmp(curr->directive, "EscapeControl") == 0 )
	{
	    nx_cfg_get_boolean(module->directives, "EscapeControl",
			       &(modconf->ctx.escape_control));
	}
	else if ( strcasecmp(curr->directive, "Fields") == 0 )
	{
	    modconf->ctx.num_field = nx_module_parse_fields(modconf->ctx.fields, curr->args);
	}
	else if ( strcasecmp(curr->directive, "FieldTypes") == 0 )
	{
	    modconf->ctx.num_type = nx_module_parse_types(modconf->ctx.types, curr->args);
	}
	else if ( strcasecmp(curr->directive, "UndefValue") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "UndefValue needs a parameter");
	    }
	    modconf->ctx.undefvalue = apr_pstrdup(module->pool, curr->args);
	}

	curr = curr->next;
    }
    
    if ( got_quote_optional == TRUE )
    {
	if ( modconf->ctx.quote_method != 0 )
	{
	    nx_conf_error(module->directives, "invalid use of deprecated QuoteOptional with QuoteMethod");
	}
	if ( quote_optional == TRUE )
	{
	    modconf->ctx.quote_method = NX_CSV_QUOTE_METHOD_STRING;
	}
	else
	{
	    modconf->ctx.quote_method = NX_CSV_QUOTE_METHOD_ALL;
	}
    }
    if ( modconf->ctx.quote_method == 0 )
    {
	modconf->ctx.quote_method = NX_CSV_QUOTE_METHOD_STRING;
    }

    if ( modconf->ctx.fields[0] == NULL )
    {
	nx_conf_error(module->directives, "mandatory 'Fields' directive missing");
    }

    if ( (modconf->ctx.num_type > 0) && (modconf->ctx.num_type != modconf->ctx.num_field) )
    {
	nx_conf_error(module->directives, "Number of 'Fields' must equal to 'FieldTypes' if both directives are defined");
    }
}


extern nx_module_exports_t nx_module_exports_xm_csv;

NX_MODULE_DECLARATION nx_xm_csv_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_csv_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    NULL,			// event
    NULL,			// info
    &nx_module_exports_xm_csv,	//exports
};
