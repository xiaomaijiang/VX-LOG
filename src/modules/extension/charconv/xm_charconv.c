/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "xm_charconv.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static const char *list_parse(char **src)
{
    size_t i = 0;
    char *retval = NULL;

    if ( **src == '\0' )
    {
	return ( NULL );
    }

    // skip space
    for ( ; (**src == ' ') || (**src == '\t'); (*src)++ );
    retval = *src;

    for ( i = 0; **src != '\0'; (*src)++ )
    {
        if ( (**src == ' ') ||
	     (**src == '\t') ||
	     (**src == ',') ||
	     (**src == ';') )
	{
	    (*src)++;
	    break;
	}
	retval[i] = **src;
	i++;
    }
    retval[i] = '\0';

    // skip space
    while ( (**src == ' ') || (**src == '\t') )
    {
	**src = '\0';
	(*src)++;
    }
    // skip delimiter
    while ( (**src == ',') || (**src == ';') )
    {
	**src = '\0';
	(*src)++;
    }

    return ( retval );
}



static void xm_charconv_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_xm_charconv_conf_t *modconf;
    char *list;
    const char *charset;
    int i;

    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_xm_charconv_conf_t));

    module->config = modconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "AutodetectCharsets") == 0 )
	{
	    list = apr_pstrdup(module->pool, curr->args);
	    for ( i = 0; (charset = list_parse(&list)) != NULL; i++ )
	    {
		log_debug("adding xm_charconv charset: %s", charset);
		if ( i >= NX_XM_CHARCONV_MAX_CHARSETS - 1 )
		{
		    throw_msg("maximum number of charsets reached, limit is %d",
			      NX_XM_CHARCONV_MAX_CHARSETS);
		}
		modconf->autocharsets[i] = charset;
	    }
	    modconf->num_charsets = i;
	}
	curr = curr->next;
    }

    log_debug("locale charset: %s", nx_get_locale_charset());
}



extern nx_module_exports_t nx_module_exports_xm_charconv;


NX_MODULE_DECLARATION nx_xm_charconv_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_charconv_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    NULL,			// event
    NULL,			// info
    &nx_module_exports_xm_charconv, //exports
};
