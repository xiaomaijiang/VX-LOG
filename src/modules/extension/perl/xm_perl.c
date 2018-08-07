/*
 * This file is part of the nxlog log collector tool.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 * License:
 * Copyright (C) 2012 by Botond Botyanszki
 * This library is free software; you can redistribute it and/or modify
 * it under the same terms as Perl itself, either Perl version 5.8.5 or,
 * at your option, any later version of Perl 5 you may have available.
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "xm_perl.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

EXTERN_C void xs_init(pTHX);

static void xm_perl_config(nx_module_t *module)
{
    nx_xm_perl_conf_t *modconf;
    const nx_directive_t *curr;

    modconf = apr_pcalloc(module->pool, sizeof(nx_xm_perl_conf_t));
    module->config = modconf;

    curr = module->directives;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "perlcode") == 0 )
	{
	    if ( modconf->perlcode != NULL )
	    {
		nx_conf_error(curr, "PerlCode is already defined");
	    }
	    modconf->perlcode = apr_pstrdup(module->pool, curr->args);
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( modconf->perlcode == NULL )
    {
	nx_conf_error(module->directives, "'PerlCode' is required");
    }
}



static void xm_perl_init(nx_module_t *module)
{
    nx_xm_perl_conf_t *modconf;
    char *args[3];

    ASSERT(module != NULL);

    modconf = (nx_xm_perl_conf_t *) module->config;
    ASSERT(modconf != NULL);
    args[0] = "nxlog";
    args[1] = modconf->perlcode;
    args[2] = NULL;

    if ( modconf->perl_interpreter == NULL )
    {
	dTHX;
	modconf->perl_interpreter = perl_alloc();
	PL_perl_destruct_level = 1;
	perl_construct(modconf->perl_interpreter);
	PERL_SET_CONTEXT(modconf->perl_interpreter);
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
    }

    if ( perl_parse(modconf->perl_interpreter, xs_init, 2, args, NULL) )
    {
	if ( modconf->perl_interpreter != NULL )
	{
	    dTHX;
	    PERL_SET_CONTEXT(modconf->perl_interpreter);
	    perl_destruct(modconf->perl_interpreter);
	    perl_free(modconf->perl_interpreter);
	    modconf->perl_interpreter = NULL;
	}

	throw_msg("the perl interpreter failed to parse %s", modconf->perlcode);
    }
}



static void xm_perl_shutdown(nx_module_t *module)
{
    nx_xm_perl_conf_t *modconf;

    ASSERT(module != NULL);

    modconf = (nx_xm_perl_conf_t *) module->config;
    ASSERT(modconf != NULL);

    if ( modconf->perl_interpreter != NULL )
    {
	dTHX;
	PERL_SET_CONTEXT(modconf->perl_interpreter);
	PL_perl_destruct_level = 1;
	perl_destruct(modconf->perl_interpreter);
	perl_free(modconf->perl_interpreter);
	modconf->perl_interpreter = NULL;
    }
}



extern nx_module_exports_t nx_module_exports_xm_perl;

NX_MODULE_DECLARATION nx_xm_perl_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_perl_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    xm_perl_init,		// init
    xm_perl_shutdown,		// shutdown
    NULL,			// event
    NULL,			// info
    &nx_module_exports_xm_perl, //exports
};
