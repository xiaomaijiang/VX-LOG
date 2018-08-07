/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "xm_xml.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static void xm_xml_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_xm_xml_conf_t *modconf;

    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_xm_xml_conf_t));

    module->config = modconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}

	curr = curr->next;
    }
}


extern nx_module_exports_t nx_module_exports_xm_xml;

NX_MODULE_DECLARATION nx_xm_xml_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_xml_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    NULL,			// event
    NULL,			// info
    &nx_module_exports_xm_xml,	//exports
};
