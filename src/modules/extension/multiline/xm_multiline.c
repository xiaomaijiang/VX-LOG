/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "../../../common/expr-parser.h"
#include "xm_multiline.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

//extern nx_module_exports_t nx_module_exports_xm_multiline;


static boolean match_line(nx_value_t *pattern, const char *subject, size_t len)
{
    int pcre_result;

    if ( pattern->type == NX_VALUE_TYPE_STRING )
    {
	if ( len < pattern->string->len )
	{
	    len = pattern->string->len;
	}
	if ( strncmp(subject, pattern->string->buf, len) == 0 )
	{
	    return ( TRUE );
	}
    }
    else // REGEXP
    {
	int ovector[NX_EXPR_MAX_CAPTURED_FIELDS * 3];

	ASSERT(pattern->type == NX_VALUE_TYPE_REGEXP);
	pcre_result = pcre_exec(pattern->regexp.pcre, NULL, 
				subject, 
				(int) len, 0, 0,
				ovector, NX_EXPR_MAX_CAPTURED_FIELDS * 3);

	if ( pcre_result >= 0 )
	{ // got match
	    return ( TRUE );
	}
	else
	{
	    switch ( pcre_result )
	    {
		case PCRE_ERROR_NOMATCH:
		    log_debug("regexp [%s] doesn't match subject string [%s]",
			      pattern->regexp.str, subject);
		    break;
		case PCRE_ERROR_NULL:
		    nx_panic("invalid arguments (code, ovector or ovecsize are invalid)");
		case PCRE_ERROR_BADOPTION:
		    nx_panic("invalid option in options parameter");
		case PCRE_ERROR_BADMAGIC:
		    nx_panic("invalid pcre magic value");
		case PCRE_ERROR_UNKNOWN_NODE:
		case PCRE_ERROR_INTERNAL:
		    nx_panic("pcre bug or buffer overflow error");
		case PCRE_ERROR_NOMEMORY:
		    nx_panic("pcre_malloc() failed");
		case PCRE_ERROR_MATCHLIMIT:
		    log_error("pcre match_limit reached");
		    break;
		case PCRE_ERROR_BADUTF8:
		    log_error("invalid pcre utf-8 byte sequence");
		    break;
		case PCRE_ERROR_BADUTF8_OFFSET:
		    log_error("invalid pcre utf-8 byte sequence offset");
		    break;
		case PCRE_ERROR_PARTIAL:
		    break;
		case PCRE_ERROR_BADPARTIAL:
		    nx_panic("PCRE_ERROR_BADPARTIAL");
		case PCRE_ERROR_BADCOUNT:
		    nx_panic("negative ovecsize");
		default:
		    log_error("unknown pcre error in pcre_exec(): %d", pcre_result);
		    break;
	    }
	}
    }

    return ( FALSE );
}



static nx_logdata_t *xm_multiline_input_func(nx_module_input_t *input,
					     void *data)
{
    volatile int i;
    nx_logdata_t * volatile retval = NULL;
    boolean foundcr = FALSE;
    nx_xm_multiline_ctx_t *ctx;
    nx_xm_multiline_conf_t *modconf;
    nx_module_t *module;
    boolean appendline;
    boolean gotline;
    volatile boolean done = FALSE;

    ASSERT(input != NULL);
    ASSERT(input->buflen >= 0);
    ASSERT(data != NULL);

    if ( input->buflen == 0 )
    {
	return ( NULL );
    }

    if ( input->ctx == NULL )
    {
	input->ctx = apr_pcalloc(input->pool, sizeof(nx_xm_multiline_ctx_t));
    }
    ctx = input->ctx;
    module = (nx_module_t *) data;
    modconf = (nx_xm_multiline_conf_t *) module->config;

    do
    {
	foundcr = FALSE;
	appendline = TRUE;
	gotline = FALSE;
	for ( i = 0; (i < input->buflen) && (gotline == FALSE) ; i++ )
	{
	    switch ( input->buf[input->bufstart + i] )
	    {
		case APR_ASCII_CR:
		    if ( foundcr == TRUE )
		    {
			gotline = TRUE;
			i--;
		    }
		    else
		    {
			foundcr = TRUE;
		    }
		    break;
		case APR_ASCII_LF:
		    if ( foundcr == TRUE )
		    {
			gotline = TRUE;
			i--;
		    }
		    else
		    {
			foundcr = TRUE;
		    }
		    break;
		default:
		    if ( foundcr == TRUE )
		    {
			gotline = TRUE;
			i--;
		    }
		    break;
	    }
	}

	if ( ctx->tmpline == NULL )
	{
	    ctx->tmpline = nx_logdata_new_logline(input->buf + input->bufstart, i);
	}
	else
	{
	    nx_logdata_append_logline(ctx->tmpline, input->buf + input->bufstart, i);
	}

	//log_info("tmpline: [%s]", ctx->tmpline->raw_event->buf);

	if ( foundcr == TRUE )
	{ // got a complete line
	    if ( module->exec != NULL )
	    {
		nx_expr_eval_ctx_t eval_ctx;
		nx_exception_t e;

		nx_expr_eval_ctx_init(&eval_ctx, ctx->tmpline, module, input);
		try
		{
		    nx_expr_statement_list_execute(&eval_ctx, module->exec);
		}
		catch(e)
		{
		    log_exception(e);
		}
		if ( eval_ctx.logdata == NULL )
		{ // dropped
		    //log_info("dropped");
		    appendline = FALSE;
		    ctx->tmpline = NULL;
		}
		else
		{
		    // TODO: merge fields
		}
		nx_expr_eval_ctx_destroy(&eval_ctx);
	    }

	    if ( appendline == TRUE )
	    { // not dropped
		size_t len;
		boolean gotheaderline = FALSE;
		boolean gotendline = FALSE;

		//log_info("appendline");
		// ignore trailing new line
		for ( len = ctx->tmpline->raw_event->len;
		      (len > 0) && ((ctx->tmpline->raw_event->buf[len - 1] == APR_ASCII_CR) ||
				    (ctx->tmpline->raw_event->buf[len - 1] == APR_ASCII_LF)); len--);
		// Check headerline
		if ( modconf->headerline != NULL )
		{
		    gotheaderline = match_line(modconf->headerline, 
					       ctx->tmpline->raw_event->buf, len);
		}

		// Check endline
		if ( modconf->endline != NULL )
		{
		    gotendline = match_line(modconf->endline,
					    ctx->tmpline->raw_event->buf, len);
		}

//		log_debug("gotheader: %d gotend: %d for [%s]", gotheaderline, gotendline,
//			  ctx->tmpline->raw_event->buf);

		if ( (gotendline == TRUE) && (gotheaderline == TRUE) )
		{
		    log_error("HeaderLine and Endline both match");
		}

		//log_info("gotheaderline: %d string: [%s]", gotheaderline, ctx->tmpline->raw_event->buf);
		if ( ctx->logdata == NULL )
		{
		    if ( (modconf->fixedlinecount <= 0) && (gotheaderline == FALSE) ) 
		    { // return if no header is found and there is no fixed linecount
			//log_info("return logdata: no header, no fixed linecount");
			retval = ctx->tmpline;
			ctx->tmpline = NULL;
			ctx->linecount = 0;
			done = TRUE;
		    }
		    else
		    {
			ctx->logdata = ctx->tmpline;
			ctx->tmpline = NULL;
			(ctx->linecount)++;
		    }
		}
		else
		{ // there is saved data
		    if ( gotendline == TRUE )
		    {
			nx_string_append(ctx->logdata->raw_event,
					 ctx->tmpline->raw_event->buf,
					 (int) ctx->tmpline->raw_event->len);
			nx_logdata_free(ctx->tmpline);
			ctx->tmpline = NULL;
			retval = ctx->logdata;
			ctx->logdata = NULL;
			done = TRUE;
		    }
		    else if ( gotheaderline == TRUE )
		    {
			//log_info("return logdata %lu: header ok", ctx->logdata);
			retval = ctx->logdata;
			ctx->logdata = ctx->tmpline;
			ctx->tmpline = NULL;
			(ctx->linecount)++;
			done = TRUE;
		    }
		    else
		    {
			nx_string_append(ctx->logdata->raw_event,
					 ctx->tmpline->raw_event->buf,
					 (int) ctx->tmpline->raw_event->len);
			nx_logdata_free(ctx->tmpline);
			ctx->tmpline = NULL;
			(ctx->linecount)++;
		    }
		}

		// Check FixedLineCount
		if ( modconf->fixedlinecount > 0 ) 
		{
		    if ( ctx->linecount >= modconf->fixedlinecount )
		    {
			//log_info("return logdata: fixed linecount");
			retval = ctx->logdata;
			ctx->logdata = NULL;
			ctx->linecount = 0;
			done = TRUE;
		    }
		}
	    }
	}
	input->buflen -= i;
	input->bufstart += i;
	//log_info("buflen: %d, foundcr: %d, i: %d, linecount: %d", input->buflen, foundcr, i, ctx->linecount);
	if ( input->buflen <= 0 )
	{
	    done = TRUE;
	}
    }
    while ( done != TRUE );

    if ( retval != NULL )
    {
	nx_string_strip_crlf(retval->raw_event);
    }

    return ( retval );
}


// return the buffered data regardless even if it is incomplete
static nx_logdata_t *xm_multiline_flush_func(nx_module_input_t *input,
					     void *data)
{
    nx_logdata_t *retval = NULL;
    nx_xm_multiline_ctx_t *ctx;
    nx_module_t *module;

    if ( input->ctx == NULL )
    {
	return ( NULL );
    }
    ctx = input->ctx;
    module = (nx_module_t *) data;

    if ( ctx->logdata == NULL )
    {
	if ( ctx->tmpline != NULL )
	{
	    retval = ctx->tmpline;
	    ctx->tmpline = NULL;
	}
    }
    else
    { // we have saved logdata
	if ( ctx->tmpline != NULL )
	{
	    nx_string_append(ctx->logdata->raw_event,
			     ctx->tmpline->raw_event->buf,
			     (int) ctx->tmpline->raw_event->len);
	    nx_logdata_free(ctx->tmpline);
	    ctx->tmpline = NULL;
	    retval = ctx->logdata;
	    ctx->logdata = NULL;
	}
	else
	{
	    retval = ctx->logdata;
	    ctx->logdata = NULL;
	}
    }

    if ( retval != NULL )
    {
	nx_string_strip_crlf(retval->raw_event);
    }

    return ( retval );
}



static void xm_multiline_config(nx_module_t *module)
{
    nx_xm_multiline_conf_t *modconf;
    const nx_directive_t * volatile curr;
    nx_exception_t e;

    modconf = apr_pcalloc(module->pool, sizeof(nx_xm_multiline_conf_t));
    module->config = modconf;

    curr = module->directives;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "headerline") == 0 )
	{
	    if ( modconf->headerline != NULL )
	    {
		nx_conf_error(curr, "HeaderLine is already defined");
	    }

	    try
	    {
		if ( (strlen(curr->args) > 1) &&
		     (curr->args[0] == '/') &&
		     (curr->args[strlen(curr->args) - 1] == '/') )
		{
		    // TODO: this is a hack because regexp and division are not correctly
		    // handled by the lexer/parser. expr-tokens.l needs to be fixed and
		    // needs to be stateful.
		    curr->args[strlen(curr->args) - 1] = '\0';
		    modconf->headerline = nx_value_new_regexp(curr->args + 1);
		}
		else
		{
		    modconf->headerline_expr = nx_expr_parse(module, curr->args, module->pool, curr->filename,
							     curr->line_num, curr->argsstart);
		    if ( modconf->headerline_expr == NULL )
		    {
			throw_msg("invalid or empty expression for HeaderLine: '%s'", curr->args);
		    }

		    if ( !((modconf->headerline_expr->rettype == NX_VALUE_TYPE_STRING) ||
			   (modconf->headerline_expr->rettype == NX_VALUE_TYPE_REGEXP)) )
		    {
			throw_msg("string or regexp type required in expression, found '%s'",
				  nx_value_type_to_string(modconf->headerline_expr->rettype));
		    }
		    if ( modconf->headerline_expr->type == NX_EXPR_TYPE_VALUE )
		    {
			ASSERT(modconf->headerline_expr->value.defined == TRUE);
			modconf->headerline = &(modconf->headerline_expr->value);
		    }
		}
	    }
	    catch(e)
	    {
		log_exception(e);
		nx_conf_error(curr, "invalid expression in 'HeaderLine'");
	    }
	}
	else if ( strcasecmp(curr->directive, "endline") == 0 )
	{
	    if ( modconf->endline != NULL )
	    {
		nx_conf_error(curr, "EndLine is already defined");
	    }

	    try
	    {
		if ( (strlen(curr->args) > 1) &&
		     (curr->args[0] == '/') &&
		     (curr->args[strlen(curr->args) - 1] == '/') )
		{
		    // TODO: this is a hack because regexp and division are not correctly
		    // handled by the lexer/parser. expr-tokens.l needs to be fixed and
		    // needs to be stateful.
		    curr->args[strlen(curr->args) - 1] = '\0';
		    modconf->endline = nx_value_new_regexp(curr->args + 1);
		}
		else
		{
		    modconf->endline_expr = nx_expr_parse(module, curr->args, module->pool, curr->filename,
							     curr->line_num, curr->argsstart);
		    if ( modconf->endline_expr == NULL )
		    {
			throw_msg("invalid or empty expression for EndLine: '%s'", curr->args);
		    }
		    if ( !((modconf->endline_expr->rettype == NX_VALUE_TYPE_STRING) ||
			   (modconf->endline_expr->rettype == NX_VALUE_TYPE_REGEXP)) )
		    {
			throw_msg("string or regexp type required in expression, found '%s'",
				  nx_value_type_to_string(modconf->endline_expr->rettype));
		    }
		    if ( modconf->endline_expr->type == NX_EXPR_TYPE_VALUE )
		    {
			ASSERT(modconf->endline_expr->value.defined == TRUE);
			modconf->endline = &(modconf->endline_expr->value);
		    }
		}
	    }
	    catch(e)
	    {
		log_exception(e);
		nx_conf_error(curr, "invalid expression in 'Endline'");
	    }

	}
	else if ( strcasecmp(curr->directive, "fixedlinecount") == 0 )
	{
	    if ( modconf->fixedlinecount != 0 )
	    {
		nx_conf_error(curr, "FixedLineCount is already defined");
	    }
	    if ( sscanf(curr->args, "%u", &(modconf->fixedlinecount)) != 1 )
	    {
		nx_conf_error(curr, "invalid number: %s", curr->args);
	    }
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( (modconf->headerline == NULL) && 
	 (modconf->endline == NULL) &&
	 (modconf->fixedlinecount == 0) )
    {
	nx_conf_error(module->directives, "At least one of HeaderLine, EndLine or FixedLineCount is required");
    }

    if ( (modconf->headerline != NULL) &&
	 (modconf->endline != NULL) &&
	 (nx_value_eq(modconf->headerline, modconf->endline) == TRUE) )
    {
	nx_conf_error(module->directives, "HeaderLine and EndLine cannot be the same");
    }

    if ( nx_module_input_func_lookup(module->name) == NULL )
    {
	//FIXME: flush func
	nx_module_input_func_register(NULL, module->name, &xm_multiline_input_func,
				      &xm_multiline_flush_func, module);
	log_debug("Inputreader '%s' registered", module->name);
    }
}



NX_MODULE_DECLARATION nx_xm_multiline_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_multiline_config,	// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    NULL,			// event
    NULL,			// info
//    &nx_module_exports_xm_multiline, //exports
    NULL,			//exports
};
