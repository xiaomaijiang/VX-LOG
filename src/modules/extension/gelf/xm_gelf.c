/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "gelf.h"
#include "xm_gelf.h"

#include "zlib.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static void xm_gelf_writer_udp(nx_module_output_t *output,
			       void *data)
{
    nx_gelf_ctx_t *gelfctx;
    nx_string_t *gelfstr;
    z_stream strm;
    nx_xm_gelf_conf_t *gelfconf = (nx_xm_gelf_conf_t *) data;
    int retval;
    nx_exception_t e;

    ASSERT(output != NULL);
    ASSERT(gelfconf != NULL);

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ASSERT(deflateInit(&strm, Z_DEFAULT_COMPRESSION) == Z_OK);

    gelfctx = (nx_gelf_ctx_t *) nx_module_output_data_get(output, "gelfctx");
    if ( gelfctx == NULL )
    {
	gelfctx = apr_pcalloc(output->pool, sizeof(nx_gelf_ctx_t));
	gelfctx->shortmessagelength = gelfconf->shortmessagelength;
	nx_module_output_data_set(output, "gelfctx", gelfctx);
    }

    gelfctx->logdata = output->logdata;

    gelfstr = nx_logdata_to_gelf(gelfctx);
    //log_info(gelfstr->buf);

    try
    {
	strm.next_in = (unsigned char *) gelfstr->buf;
	strm.avail_in = gelfstr->len;
	strm.next_out = (unsigned char *) output->buf;
	strm.avail_out = (uInt) output->bufsize;
	retval = deflate(&strm, Z_FINISH);
	switch ( retval )
	{
	    case Z_STREAM_END:
		break;
	    case Z_OK:
		break;
	    case Z_MEM_ERROR:
		throw_msg("zlib compression error, insufficient memory (Z_MEM_ERROR) %s", 
			  strm.msg == NULL ? "" : strm.msg);
	    case Z_DATA_ERROR:
		throw_msg("zlib compression error, data error (Z_DATA_ERROR) %s", 
			  strm.msg == NULL ? "" : strm.msg);
	    case Z_BUF_ERROR:
		throw_msg("zlib compression error, buffer error (Z_BUF_ERROR) %s", 
			  strm.msg == NULL ? "" : strm.msg);
	    case Z_ERRNO:
		throw_errno("zlib compression error (Z_ERRNO) %s", 
			    strm.msg == NULL ? "" : strm.msg);
	    default:
		throw_msg("zlib compression error (%d) %s", retval, 
			  strm.msg == NULL ? "" : strm.msg);
	}
    }
    catch(e)
    {
	deflateEnd(&strm);
	nx_string_free(gelfstr);
	rethrow(e);
    }

    if ( (retval != Z_STREAM_END) && (strm.avail_out == 0) )
    {
	output->buflen = 0;
	output->bufstart = 0;
	log_error("Output buffer too small (%d), couldn't compress GELF packet (size: %d). Please increase BufferSize.",
		  (int) output->bufsize, gelfstr->len);
    }
    else
    {
	output->buflen = output->bufsize - strm.avail_out;
	output->bufstart = 0;
    }
    deflateEnd(&strm);
    nx_string_free(gelfstr);
}



static void xm_gelf_writer_tcp(nx_module_output_t *output,
			       void *data)
{
    nx_gelf_ctx_t *gelfctx;
    nx_xm_gelf_conf_t *gelfconf = (nx_xm_gelf_conf_t *) data;

    ASSERT(output != NULL);
    ASSERT(gelfconf != NULL);

    gelfctx = (nx_gelf_ctx_t *) nx_module_output_data_get(output, "gelfctx");
    if ( gelfctx == NULL )
    {
	gelfctx = apr_pcalloc(output->pool, sizeof(nx_gelf_ctx_t));
	gelfctx->shortmessagelength = gelfconf->shortmessagelength;
	nx_module_output_data_set(output, "gelfctx", gelfctx);
    }

    gelfctx->logdata = output->logdata;

    if ( gelfctx->gelfstr != NULL )
    {
	nx_string_free(gelfctx->gelfstr);
	gelfctx->gelfstr = NULL;
    }
    gelfctx->gelfstr = nx_logdata_to_gelf(gelfctx);
    if ( gelfconf->usenulldelimiter == FALSE )
    {
	nx_string_append(gelfctx->gelfstr, "\n", 1);
    }
    //log_info("%s", gelfctx->gelfstr->buf);
    output->buf = gelfctx->gelfstr->buf;
    output->buflen = gelfctx->gelfstr->len;
    output->bufstart = 0;
    if ( gelfconf->usenulldelimiter == TRUE )
    { // include trailing NUL
	(output->buflen)++;
    }
}



static void xm_gelf_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_xm_gelf_conf_t *modconf;

    modconf = apr_pcalloc(module->pool, sizeof(nx_xm_gelf_conf_t));
    module->config = modconf;

    nx_module_output_func_register(NULL, "gelf", &xm_gelf_writer_udp, modconf);
    nx_module_output_func_register(NULL, "gelf_udp", &xm_gelf_writer_udp, modconf);
    nx_module_output_func_register(NULL, "gelf_tcp", &xm_gelf_writer_tcp, modconf);


    curr = module->directives;


    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "shortmessagelength") == 0 )
	{
	    if ( modconf->shortmessagelength != 0 )
	    {
		nx_conf_error(curr, "ShortMessageLength is already defined");
	    }
	    if ( sscanf(curr->args, "%u", &(modconf->shortmessagelength)) != 1 )
	    {
		nx_conf_error(curr, "invalid ShortMessageLength: %s", curr->args);
	    }
	}
	else if ( strcasecmp(curr->directive, "UseNullDelimiter") == 0 )
	{
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}

	curr = curr->next;
    }

    modconf->usenulldelimiter = TRUE; // Default is TRUE
    nx_cfg_get_boolean(module->directives, "UseNullDelimiter", &(modconf->usenulldelimiter));

    if ( modconf->shortmessagelength == 0 )
    {
	modconf->shortmessagelength = 64;
    }
}



NX_MODULE_DECLARATION nx_xm_gelf_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_gelf_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    NULL,			// event
    NULL,			// info
    NULL,			//exports
};
