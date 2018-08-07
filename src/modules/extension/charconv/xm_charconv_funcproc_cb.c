/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "../../../common/module.h"
#include "xm_charconv.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

// TODO: optimization
// implement utf8_validate() and use this to test input if dstenc is utf-8,
// there in no need to call iconv in this case.
// Autodetection and conversion should use persistent iconv_ctx with mutex locking.
// For auto, it should be an array of iconv_ctx.


void nx_expr_proc__convert_fields(nx_expr_eval_ctx_t *eval_ctx,
				  nx_module_t *module,
				  nx_expr_list_t *args)
{
    nx_expr_list_elem_t *src, *dst;
    nx_value_t srcenc, dstenc;
    nx_xm_charconv_conf_t *modconf;
    nx_exception_t e;
    iconv_t * volatile iconv_ctx = NULL;
    nx_string_t * volatile tmpstr = NULL;

    ASSERT(module != NULL);

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available for convert_fields(), possibly dropped");
    }

    modconf = (nx_xm_charconv_conf_t *) module->config;
    ASSERT(modconf != NULL);

    ASSERT(args != NULL);
    src = NX_DLIST_FIRST(args);
    ASSERT(src != NULL);
    ASSERT(src->expr != NULL);
    dst = NX_DLIST_NEXT(src, link);
    ASSERT(dst != NULL);
    ASSERT(dst->expr != NULL);

    nx_expr_evaluate(eval_ctx, &srcenc, src->expr);

    if ( srcenc.defined != TRUE )
    {
	throw_msg("'srcencoding' is undef");
    }
    if ( srcenc.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&srcenc);
	throw_msg("string type required for 'srcencoding'");
    }

    try
    {
	nx_expr_evaluate(eval_ctx, &dstenc, dst->expr);
    }
    catch(e)
    {
	nx_value_kill(&srcenc);
	rethrow(e);
    }
    if ( dstenc.defined != TRUE )
    {
	nx_value_kill(&srcenc);
	throw_msg("'dstencoding' is undef");
    }
    if ( dstenc.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&dstenc);
	nx_value_kill(&srcenc);
	throw_msg("string type required for 'dstencoding'");
    }

    try
    {
	iconv_t cd;
	nx_logdata_field_t *field;
	int32_t inbytesleft;
	int32_t	outbytesleft;
	int32_t converted;
	const char *inptr;
	char *outptr;
	const char *ok_enc;

	if ( strcasecmp(srcenc.string->buf, "auto") != 0 )
	{
	    cd = nx_iconv_open(dstenc.string->buf, srcenc.string->buf);
	    iconv_ctx = &cd;
	}

	log_debug("converting from %s to %s", srcenc.string->buf, dstenc.string->buf);

	for ( field = NX_DLIST_FIRST(&(eval_ctx->logdata->fields));
	      field != NULL;
	      field = NX_DLIST_NEXT(field, link) )
	{
	    ASSERT(field->value != NULL);
	    if ( (field->value->defined == TRUE) && 
		 (field->value->type == NX_VALUE_TYPE_STRING) &&
		 (field->value->string->len > 0) )
	    {
		// assume string can get 3 times the size after conversion
		tmpstr = nx_string_new_size(field->value->string->len * 3 + 1);
		outptr = tmpstr->buf;
		outbytesleft = (int32_t) tmpstr->bufsize - 1;
		inptr = field->value->string->buf;
		inbytesleft = (int32_t) field->value->string->len;
		ok_enc = NULL;

		if ( iconv_ctx == NULL )
		{ // auto
		    converted = nx_convert_auto(&outptr, &outbytesleft, &inptr, &inbytesleft,
						dstenc.string->buf, modconf->num_charsets,
						modconf->autocharsets, &ok_enc);
		    log_debug("detected charset: %s", ok_enc);
		}
		else
		{
		    converted = nx_convert_ctx(&outptr, &outbytesleft, &inptr, &inbytesleft,
					       iconv_ctx, TRUE);
		}
		ASSERT(converted < (int32_t) tmpstr->bufsize);
		tmpstr->buf[converted] = '\0';
		tmpstr->len = (uint32_t) converted;

		if ( field->value->string == eval_ctx->logdata->raw_event )
		{
		    eval_ctx->logdata->raw_event = tmpstr;
		}
		nx_string_free(field->value->string);
		field->value->string = tmpstr;
	    }
	}
	if ( iconv_ctx != NULL )
	{
	    iconv_close(*iconv_ctx);
	}
    }
    catch(e)
    {
	nx_value_kill(&srcenc);
	nx_value_kill(&dstenc);
	if ( iconv_ctx != NULL )
	{
	    iconv_close(*iconv_ctx);
	}
	if ( tmpstr != NULL )
	{
	    nx_string_free(tmpstr);
	}
	rethrow(e);
    }
    nx_value_kill(&srcenc);
    nx_value_kill(&dstenc);
}



void nx_expr_func__convert(nx_expr_eval_ctx_t *eval_ctx,
			   nx_module_t *module,
			   nx_value_t *retval,
			   int32_t num_arg,
			   nx_value_t *args)
{
    nx_xm_charconv_conf_t *modconf;
    nx_exception_t e;
    iconv_t * volatile iconv_ctx = NULL;
    nx_string_t * volatile tmpstr = NULL;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 3);
    ASSERT(module != NULL);

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available for convert(), possibly dropped");
    }

    modconf = (nx_xm_charconv_conf_t *) module->config;
    ASSERT(modconf != NULL);

    retval->type = NX_VALUE_TYPE_STRING;

    if ( args[0].defined != TRUE ) 
    {
	retval->defined = FALSE;
	return;
    }

    if ( (args[1].defined != TRUE) || (args[2].defined != TRUE) ) 
    {
	throw_msg("srcencoding or dstencoding is undef in function 'convert(string, srcencoding, dstencoding)'");
    }
    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("invalid '%s' type of first argument for function 'convert(string, srcencoding, dstencoding)'",
		  nx_value_type_to_string(args[0].type));
    }
    if ( args[1].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("invalid '%s' type of second argument for function 'convert(string, srcencoding, dstencoding)'",
		  nx_value_type_to_string(args[1].type));
    }
    if ( args[2].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("invalid '%s' type of third argument for function 'convert(string, srcencoding, dstencoding)'",
		  nx_value_type_to_string(args[2].type));
    }

    if ( args[0].string->len == 0 )
    {
	retval->defined = TRUE;
	retval->string = nx_string_new();
	return;
    }

    try
    {
	iconv_t cd;
	int32_t inbytesleft;
	int32_t	outbytesleft;
	int32_t converted;
	const char *inptr;
	char *outptr;
	const char *ok_enc;

	if ( strcasecmp(args[1].string->buf, "auto") != 0 )
	{
	    cd = nx_iconv_open(args[2].string->buf, args[1].string->buf);
	    iconv_ctx = &cd;
	}

	log_debug("converting from %s to %s", args[1].string->buf, args[2].string->buf);
	tmpstr = nx_string_new_size(args[0].string->len * 3 + 1);
	outptr = tmpstr->buf;
	outbytesleft = (int32_t) tmpstr->bufsize - 1;
	inptr = args[0].string->buf;
	inbytesleft = (int32_t) args[0].string->len;
	ok_enc = NULL;

	if ( iconv_ctx == NULL )
	{ // auto
	    converted = nx_convert_auto(&outptr, &outbytesleft, &inptr, &inbytesleft,
					args[2].string->buf, modconf->num_charsets,
					modconf->autocharsets, &ok_enc);
	    log_debug("detected charset: %s", ok_enc);
	}
	else
	{
	    converted = nx_convert_ctx(&outptr, &outbytesleft, &inptr, &inbytesleft,
				       iconv_ctx, TRUE);
	}
	ASSERT(converted < (int32_t) tmpstr->bufsize);
	tmpstr->buf[converted] = '\0';
	tmpstr->len = (uint32_t) converted;
	retval->defined = TRUE;
	retval->string = tmpstr;
	if ( iconv_ctx != NULL )
	{
	    iconv_close(*iconv_ctx);
	}
    }
    catch(e)
    {
	if ( iconv_ctx != NULL )
	{
	    iconv_close(*iconv_ctx);
	}
	if ( tmpstr != NULL )
	{
	    nx_string_free(tmpstr);
	}
	rethrow(e);
    }
}
