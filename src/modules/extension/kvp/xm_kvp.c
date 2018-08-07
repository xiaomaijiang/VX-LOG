/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "xm_kvp.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static void xm_kvp_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_xm_kvp_conf_t *modconf;
    char tmpchar;

    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_xm_kvp_conf_t));

    module->config = modconf;

    nx_kvp_ctx_init(&(modconf->ctx));
//    modconf->ctx.quote_method = 0;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "keyquotechar") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "KeyQuoteChar needs a parameter");
	    }
	    tmpchar = nx_kvp_get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid KeyQuoteChar parameter: %s", curr->args);
	    }
	    modconf->ctx.keyquotechar = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "valuequotechar") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "ValueQuoteChar needs a parameter");
	    }
	    tmpchar = nx_kvp_get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid ValueQuoteChar parameter: %s", curr->args);
	    }
	    modconf->ctx.valquotechar = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "escapechar") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "EscapeChar needs a parameter");
	    }
	    tmpchar = nx_kvp_get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid EscapeChar parameter: %s", curr->args);
	    }
	    modconf->ctx.escapechar = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "kvdelimiter") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "KVDelimiter needs a parameter");
	    }
	    tmpchar = nx_kvp_get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid KVDelimiter parameter: %s", curr->args);
	    }
	    modconf->ctx.kvdelimiter = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "kvpdelimiter") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "KVPDelimiter needs a parameter");
	    }
	    tmpchar = nx_kvp_get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid KVPDelimiter parameter: %s", curr->args);
	    }
	    modconf->ctx.kvpdelimiter = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "EscapeControl") == 0 )
	{
	    nx_cfg_get_boolean(module->directives, "EscapeControl",
			       &(modconf->ctx.escape_control));
	}

	curr = curr->next;
    }
/*
    if ( modconf->ctx.quote_method == 0 )
    {
	modconf->ctx.quote_method = NX_KVP_QUOTE_METHOD_AUTO;
    }
*/
}


extern nx_module_exports_t nx_module_exports_xm_kvp;

NX_MODULE_DECLARATION nx_xm_kvp_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_kvp_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    NULL,			// event
    NULL,			// info
    &nx_module_exports_xm_kvp,	//exports
};
