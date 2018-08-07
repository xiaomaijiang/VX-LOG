/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */


#include <pcre.h>

#include "error_debug.h"
#include "value.h"
#include "expr.h"
#include "expr-grammar.h"
#include "expr-parser.h"
#include "../core/ctx.h"
#include "date.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

static void _regmatch(nx_expr_eval_ctx_t *eval_ctx,
		      nx_value_t *retval,
		      const nx_expr_t *left,
		      const nx_expr_t *right);


static void nx_expr_decl_init(nx_expr_decl_t *decl, 
			      nx_expr_parser_t *parser,
			      const char *exprname)
{
    ASSERT(decl != NULL);
    ASSERT(parser != NULL);

    decl->file = parser->file;
    if ( decl->file == NULL )
    {
	decl->file = "INPUT";
    }

    decl->line = parser->linenum;
    decl->pos = parser->linepos;
    decl->name = exprname;
    log_debug("%s declared at line %d, character %d in %s", decl->name, decl->line, decl->pos, decl->file);
}



void nx_expr_statement_list_execute(nx_expr_eval_ctx_t *eval_ctx,
				    const nx_expr_statement_list_t *stmnts)
{
    nx_expr_statement_t *stmnt;

    ASSERT(stmnts != NULL);
    ASSERT(eval_ctx != NULL);
    
    log_debug("executing statements");

    for ( stmnt = NX_DLIST_FIRST(stmnts);
	  stmnt != NULL;
	  stmnt = NX_DLIST_NEXT(stmnt, link) )
    {
	nx_expr_statement_execute(eval_ctx, stmnt);
    }
}



static void nx_expr_statement_assignment_execute(nx_expr_eval_ctx_t *eval_ctx,
						 const nx_expr_statement_t *stmnt)
{
    nx_value_t *value;
    nx_exception_t e;

    ASSERT(stmnt->assignment.lval->type == NX_EXPR_TYPE_FIELD);
    ASSERT(stmnt->assignment.lval->field != NULL);

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("missing logdata, assignment possibly after drop()");
    }

    value = malloc(sizeof(nx_value_t));
    try
    {
	nx_expr_evaluate(eval_ctx, value, stmnt->assignment.rval);
    }
    catch(e)
    {
	nx_value_free(value);
	rethrow(e);
    }

    nx_logdata_set_field_value(eval_ctx->logdata, stmnt->assignment.lval->field, value);
}



static void nx_expr_statement_ifelse_execute(nx_expr_eval_ctx_t *eval_ctx,
					     const nx_expr_statement_t *stmnt)
{
    nx_value_t value;
    nx_exception_t e;

    ASSERT(stmnt->ifelse.cond != NULL);

    try
    {
	nx_expr_evaluate(eval_ctx, &value, stmnt->ifelse.cond);
	if ( (value.defined == FALSE) ||
	     ((value.type == NX_VALUE_TYPE_BOOLEAN) && (value.boolean == FALSE)) ||
	     ((value.type == NX_VALUE_TYPE_INTEGER) && (value.integer == 0)) )
	{
	    if ( stmnt->ifelse.cond_false != NULL )
	    {
		nx_expr_statement_execute(eval_ctx, stmnt->ifelse.cond_false);
	    }
	}
	else
	{
	    if ( stmnt->ifelse.cond_true != NULL )
	    {
		nx_expr_statement_execute(eval_ctx, stmnt->ifelse.cond_true);
	    }
	}
    }
    catch(e)
    {
	rethrow(e);
	nx_value_kill(&value);
    }
    nx_value_kill(&value);
}



void nx_expr_statement_execute(nx_expr_eval_ctx_t *eval_ctx,
			       const nx_expr_statement_t *stmnt)
{
    nx_exception_t e;

    ASSERT(stmnt != NULL);
    ASSERT(eval_ctx != NULL);
    ASSERT(stmnt != NULL);

    if ( eval_ctx->dropped == TRUE )
    {
	return;
    }

    switch ( stmnt->type )
    {
	case NX_EXPR_STATEMENT_TYPE_PROCEDURE:
	    ASSERT(stmnt->procedure.cb != NULL);
	    try
	    {
		stmnt->procedure.cb(eval_ctx, stmnt->procedure.module, stmnt->procedure.args);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "procedure '%s' failed at line %d, character %d in %s. "
			    "statement execution has been aborted",
			    stmnt->decl.name, stmnt->decl.line, stmnt->decl.pos,
			    stmnt->decl.file);
	    }
	    break;
	case NX_EXPR_STATEMENT_TYPE_IFELSE:
	    try
	    {
		nx_expr_statement_ifelse_execute(eval_ctx, stmnt);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "%s failed at line %d, character %d in %s. "
			    "statement execution has been aborted",
			    stmnt->decl.name, stmnt->decl.line, stmnt->decl.pos,
			    stmnt->decl.file);
	    }
	    break;
	case NX_EXPR_STATEMENT_TYPE_ASSIGNMENT:
	    try
	    {
		nx_expr_statement_assignment_execute(eval_ctx, stmnt);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "%s failed at line %d, character %d in %s. "
			    "statement execution has been aborted",
			    stmnt->decl.name, stmnt->decl.line, stmnt->decl.pos,
			    stmnt->decl.file);
	    }
	    break;
	case NX_EXPR_STATEMENT_TYPE_BLOCK:
	    if ( stmnt->block.statements != NULL )
	    {
		nx_expr_statement_list_execute(eval_ctx, stmnt->block.statements);
	    }
	    break;
	case NX_EXPR_STATEMENT_TYPE_REGEXPREPLACE:
	    try
	    {
		nx_value_t retval;
		_regmatch(eval_ctx, &retval, stmnt->regexpreplace.lval,
			  stmnt->regexpreplace.rval);
		nx_value_kill(&retval);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "%s failed at line %d, character %d in %s. "
			    "statement execution has been aborted",
			    stmnt->decl.name, stmnt->decl.line, stmnt->decl.pos,
			    stmnt->decl.file);
	    }
	    break;
	case NX_EXPR_STATEMENT_TYPE_REGEXP:
	    try
	    {
		nx_value_t retval;
		_regmatch(eval_ctx, &retval, stmnt->regexp.lval,
			  stmnt->regexp.rval);
		nx_value_kill(&retval);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "%s failed at line %d, character %d in %s. "
			    "statement execution has been aborted",
			    stmnt->decl.name, stmnt->decl.line, stmnt->decl.pos,
			    stmnt->decl.file);
	    }
	    break;
	default:
	    nx_panic("invalid statement type %d", stmnt->type);
    }
}



static void nx_expr_eval_func(nx_expr_eval_ctx_t *eval_ctx,
			      nx_value_t *retval,
			      const nx_expr_t *expr)
{
    int volatile argc, i;
    nx_value_t *values;
    nx_expr_list_elem_t *arg;
    nx_exception_t e;
    
    if ( expr->function.args == NULL )
    {
	expr->function.cb(eval_ctx, expr->function.module, retval, 0, NULL);
	return;
    }

    for ( arg = NX_DLIST_FIRST(expr->function.args), argc = 0;
	  arg != NULL;
	  arg = NX_DLIST_NEXT(arg, link), argc++ );

    values = malloc((size_t) argc * sizeof(nx_value_t));
    memset(values, 0, (size_t) argc * sizeof(nx_value_t));

    try
    {
	for ( arg = NX_DLIST_FIRST(expr->function.args), i = 0;
	      arg != NULL;
	      arg = NX_DLIST_NEXT(arg, link), i++ )
	{
	    nx_expr_evaluate(eval_ctx, &(values[i]), arg->expr);
	}
	//log_debug("CB: %ld", values);
	expr->function.cb(eval_ctx, expr->function.module, retval, argc, values);
    }
    catch(e)
    {
	for ( arg = NX_DLIST_FIRST(expr->function.args), i = 0;
	      arg != NULL;
	      arg = NX_DLIST_NEXT(arg, link), i++ )
	{
	    nx_value_kill(&(values[i]));
	}
	free(values);
	rethrow(e);
    }

    for ( arg = NX_DLIST_FIRST(expr->function.args), i = 0;
	  arg != NULL;
	  arg = NX_DLIST_NEXT(arg, link), i++ )
    {
	//log_debug("KILL: %ld", values);
	nx_value_kill(&(values[i]));
    }
    free(values);
}



void nx_expr_eval_ctx_init(nx_expr_eval_ctx_t *eval_ctx,
			   nx_logdata_t *logdata,
			   nx_module_t *module,
			   nx_module_input_t *input)
{
    ASSERT(eval_ctx != NULL);

    eval_ctx->logdata = logdata;
    eval_ctx->module = module;
    eval_ctx->input = input;
    eval_ctx->num_captured = 0;
    eval_ctx->captured = NULL;
    eval_ctx->dropped = FALSE;
}



void nx_expr_eval_ctx_destroy(nx_expr_eval_ctx_t *eval_ctx)
{
    int i;

    ASSERT(eval_ctx != NULL);

    for ( i = 0; i < eval_ctx->num_captured; i++ )
    {
	nx_string_free(eval_ctx->captured[i]);
    }
    if ( eval_ctx->captured != NULL )
    {
	free(eval_ctx->captured);
    }
}



static void _inop(nx_expr_eval_ctx_t *eval_ctx,
		  nx_value_t *retval,
		  const nx_expr_t *expr,
		  const nx_expr_list_t *exprs)
{
    nx_value_t exprval;
    nx_exception_t e;
    nx_expr_list_elem_t *elem;
    int volatile elemcount = 0;
    nx_value_t *values = NULL;
    int i = 0;

    ASSERT(expr != NULL);
    ASSERT(exprs != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;
    retval->defined = TRUE;
    retval->boolean = FALSE;

    for ( elem = NX_DLIST_FIRST(exprs), elemcount = 0;
	  elem != NULL;
	  elem = NX_DLIST_NEXT(elem, link), elemcount++ );

    values = malloc((size_t) elemcount * sizeof(nx_value_t));
    memset(values, 0, (size_t) elemcount * sizeof(nx_value_t));
	
    nx_expr_evaluate(eval_ctx, &exprval, expr);

    if ( exprval.defined == FALSE )
    {
	// evaluate to UNDEF if expr is undef
	retval->defined = FALSE;
	return;
    }

    try
    {
	for ( elem = NX_DLIST_FIRST(exprs), i = 0;
	      elem != NULL;
	      elem = NX_DLIST_NEXT(elem, link), i++ )
	{
	    nx_expr_evaluate(eval_ctx, &(values[i]), elem->expr);

	    if ( nx_value_eq(&exprval, &(values[i])) == TRUE )
	    {
		retval->boolean = TRUE;
		break;
	    }
	}
    }
    catch(e)
    {
	for ( i = 0; i < elemcount; i++ )
	{
	    nx_value_kill(&(values[i]));
	}
	nx_value_kill(&exprval);
	if ( values != NULL )
	{
	    free(values);
	}
	rethrow(e);
    }

    for ( i = 0; i < elemcount; i++ )
    {
	nx_value_kill(&(values[i]));
    }
    nx_value_kill(&exprval);
    if ( values != NULL )
    {
	free(values);
    }

    return;
}



void nx_expr_evaluate(nx_expr_eval_ctx_t *eval_ctx,
		      nx_value_t *retval,
		      const nx_expr_t *expr)
{
    nx_exception_t e;

    ASSERT(retval != NULL);
    ASSERT(expr != NULL);
    ASSERT(eval_ctx != NULL);

    log_debug("evaluating expression '%s' at %s:%d",
	      expr->decl.name, expr->decl.file, expr->decl.line);
    retval->defined = FALSE;
    switch ( expr->type )
    {
	case NX_EXPR_TYPE_VALUE:
	    nx_value_clone(retval, &(expr->value));
	    break;
	case NX_EXPR_TYPE_FIELD:
	    if ( eval_ctx->logdata == NULL )
	    {
		throw_msg("missing logdata, no field available in this context at line %d,"
			  " character %d in %s. operation possibly after drop()",
			  expr->decl.line, expr->decl.pos,
			  expr->decl.file);
	    }
	    else
	    {
		nx_value_t tmpval;
		nx_logdata_get_field_value(eval_ctx->logdata, expr->field, &tmpval);
		nx_value_clone(retval, &tmpval);
	    }
	    break;
	case NX_EXPR_TYPE_CAPTURED:
	    ASSERT(expr->captured >= 0);
	    if ( eval_ctx->num_captured <= expr->captured )
	    {
		retval->defined = FALSE;
		retval->type = NX_VALUE_TYPE_STRING;
	    }
	    else
	    {
		retval->defined = TRUE;
		retval->type = NX_VALUE_TYPE_STRING;
		retval->string = nx_string_clone(eval_ctx->captured[expr->captured]);
	    }
	    break;
	case NX_EXPR_TYPE_UNOP:
	    log_debug("evaluating unop(%d)", expr->unop.op->type);
	    try
	    {
		expr->unop.cb(eval_ctx, retval, expr->unop.op);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "%s failed at line %d, character %d in %s. "
			    "expression evaluation has been aborted",
			    expr->decl.name, expr->decl.line, expr->decl.pos,
			    expr->decl.file);
	    }
	    break;
	case NX_EXPR_TYPE_BINOP:
	    log_debug("evaluating binop(%d, %d)", expr->binop.left->type,
		      expr->binop.right->type);
	    try
	    {
		expr->binop.cb(eval_ctx, retval, expr->binop.left, expr->binop.right);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "%s failed at line %d, character %d in %s. "
			    "expression evaluation has been aborted",
			    expr->decl.name, expr->decl.line, expr->decl.pos,
			    expr->decl.file);
	    }
	    break;
	case NX_EXPR_TYPE_INOP:
	    log_debug("evaluating in");
	    try
	    {
		_inop(eval_ctx, retval, expr->inop.expr, expr->inop.exprs);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "%s failed at line %d, character %d in %s. "
			    "expression evaluation has been aborted",
			    expr->decl.name, expr->decl.line, expr->decl.pos,
			    expr->decl.file);
	    }
	    break;
	case NX_EXPR_TYPE_FUNCTION:
	    log_debug("evaluating function");
	    try
	    {
		nx_expr_eval_func(eval_ctx, retval, expr);
	    }
	    catch(e)
	    {
		rethrow_msg(e, "function '%s' failed at line %d, character %d in %s. "
			    "expression evaluation has been aborted",
			    expr->decl.name, expr->decl.line, expr->decl.pos,
			    expr->decl.file);
	    }
	    break;
	default:
	    nx_panic("invalid expression type: %d", expr->type);
    }
}



static void _regmatch(nx_expr_eval_ctx_t *eval_ctx,
		      nx_value_t *retval,
		      const nx_expr_t *left,
		      const nx_expr_t *right)
{
    nx_value_t lval, rval;
    const pcre *regexp = NULL;
    const char *regexpstr = NULL;
    char *subject = NULL;
    const char *replacement = NULL;
    size_t replacement_length = 0;
    uint8_t modifiers;
    int result;
    int i;
    int ovector[NX_EXPR_MAX_CAPTURED_FIELDS * 3];
    nx_value_t fieldval;
    int global_match_recursion_cnt = 0;
    int options = 0;

    ASSERT(retval != NULL);
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    log_debug("regmatch");

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    // clear captured strings from pervious operation
    for ( i = 0; i < eval_ctx->num_captured; i++ )
    {
	nx_string_free(eval_ctx->captured[i]);
    }
    eval_ctx->num_captured = 0;
    if ( eval_ctx->captured != NULL )
    {
	free(eval_ctx->captured);
	eval_ctx->captured = NULL;
    }

    nx_expr_evaluate(eval_ctx, &lval, left);

    if ( lval.defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);
    if ( rval.defined == FALSE )
    {
	retval->defined = FALSE;
	nx_value_kill(&lval);
	return;
    }

    if ( lval.type == NX_VALUE_TYPE_STRING )
    {
	ASSERT(rval.type == NX_VALUE_TYPE_REGEXP);
	regexp = rval.regexp.pcre;
	regexpstr = rval.regexp.str;
	replacement = rval.regexp.replacement;
	modifiers = rval.regexp.modifiers;
	subject = lval.string->buf;
    }
    else if ( rval.type == NX_VALUE_TYPE_STRING )
    {
	ASSERT(lval.type == NX_VALUE_TYPE_REGEXP);
	regexp = lval.regexp.pcre;
	regexpstr = lval.regexp.str;
	replacement = lval.regexp.replacement;
	modifiers = lval.regexp.modifiers;
	subject = rval.string->buf;
    }
    else
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s =~ %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }

    ASSERT(subject != NULL);

    if ( replacement != NULL )
    {
	ASSERT(right->rettype == NX_VALUE_TYPE_REGEXP);
	ASSERT(left->type == NX_EXPR_TYPE_FIELD);
	replacement_length = strlen(replacement);
	if ( eval_ctx->logdata == NULL )
	{
	    throw_msg("missing logdata, no field available in this context at line %d,"
		      " character %d in %s. operation possibly after drop()",
		      left->decl.line, left->decl.pos,
		      left->decl.file);
	}
	nx_logdata_get_field_value(eval_ctx->logdata, left->field, &fieldval);
	ASSERT(fieldval.type == NX_VALUE_TYPE_STRING);
	ASSERT(fieldval.defined == TRUE);
	subject = fieldval.string->buf;
    }

    retval->boolean = FALSE;
    retval->defined = TRUE;

  match_global:
    result = pcre_exec(regexp, NULL, subject, (int) strlen(subject), 0, options,
		       ovector, NX_EXPR_MAX_CAPTURED_FIELDS * 3);
    
    if ( result >= 0 )
    {
	retval->boolean = TRUE;
    }
    if ( result < 0 )
    {
	switch ( result )
	{
	    case PCRE_ERROR_NOMATCH:
		log_debug("regexp /%s/ doesn't match subject string '%s'", regexpstr, subject);
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
		log_error("pcre match_limit reached for regexp /%s/", regexpstr);
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
		log_error("unknown pcre error in pcre_exec(): %d", result);
		break;
	}
    }
    else if ( result > 0 )
    {
	ASSERT(result < NX_EXPR_MAX_CAPTURED_FIELDS);

	// clear captured strings from pervious operation
	for ( i = 0; i < eval_ctx->num_captured; i++ )
	{
	    nx_string_free(eval_ctx->captured[i]);
	}
	eval_ctx->num_captured = 0;
	if ( eval_ctx->captured != NULL )
	{
	    free(eval_ctx->captured);
	    eval_ctx->captured = NULL;
	}

	eval_ctx->num_captured = result;
	eval_ctx->captured = malloc((size_t) eval_ctx->num_captured * sizeof(nx_string_t *));

	for ( i = 0; i < result; i++ )
	{ 
	    eval_ctx->captured[i] = nx_string_create(subject + ovector[i * 2],
						     (size_t) (ovector[i * 2 + 1] - ovector[i * 2]));
	}

	if ( replacement != NULL )
	{
	    ASSERT(ovector[1] >= ovector[0]);
	    nx_string_ensure_size(fieldval.string,
				  (size_t) fieldval.string->len + 
				  (size_t) (ovector[1] - ovector[0]) + replacement_length);
	    memmove(fieldval.string->buf + (size_t) ovector[0] + replacement_length,
		    fieldval.string->buf + (size_t) ovector[1],
		    fieldval.string->len - (size_t) ovector[1] + 1); // +1 is for the trailing NUL
	    if ( replacement_length > 0 )
	    {
		memcpy(fieldval.string->buf + ovector[0],
		       replacement, replacement_length);
	    }
	    ASSERT(fieldval.string->len >= (uint32_t) (ovector[1] - ovector[0]));
	    fieldval.string->len += ((uint32_t) (replacement_length - (size_t) (ovector[1] - ovector[0])));
	    subject = fieldval.string->buf;
	    if ( modifiers & NX_EXPR_REGEXP_MODIFIER_MATCH_GLOBAL )
	    {
		global_match_recursion_cnt++;
		if ( global_match_recursion_cnt > 10000 ) 
		{ // avoid an infinite loop with s/TEST/TEST/
		    nx_value_kill(&lval);
		    nx_value_kill(&rval);
		    throw_msg("likely infinite loop detected in regexp substitution");
		}
		else
		{
		    goto match_global; // not that ugly as it seems
		}
	    }
	}
    }

    nx_value_kill(&lval);
    nx_value_kill(&rval);
}



static void _notregmatch(nx_expr_eval_ctx_t *eval_ctx,
			 nx_value_t *retval,
			 const nx_expr_t *left,
			 const nx_expr_t *right)
{
    _regmatch(eval_ctx, retval, left, right);
    if ( retval->defined == TRUE )
    {
	retval->boolean = ! retval->boolean;
    }
}



static void _equal(nx_expr_eval_ctx_t *eval_ctx,
		   nx_value_t *retval,
		   const nx_expr_t *left,
		   const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(retval != NULL);
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    nx_expr_evaluate(eval_ctx, &lval, left);
    nx_expr_evaluate(eval_ctx, &rval, right);

    if ( lval.defined == FALSE )
    {
	if ( rval.defined == FALSE )
	{ // undef == undef = TRUE
	    retval->defined = TRUE;
	    retval->boolean = TRUE;
	    return;
	}
	retval->defined = FALSE;
	nx_value_kill(&rval);
	return;
    }

    if ( rval.defined == FALSE )
    {
	retval->defined = FALSE;
	nx_value_kill(&lval);
	return;
    }

    if ( lval.type != rval.type )
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s == %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }
    retval->defined = TRUE;

    switch ( lval.type )
    {
	case NX_VALUE_TYPE_STRING:
	    //log_debug("STRING: %s == %s", lval.string, rval.string);
	    retval->boolean = strcmp(lval.string->buf, rval.string->buf) == 0;
	    break;
	case NX_VALUE_TYPE_INTEGER:
	    //log_debug("INTEGER: %ld == %ld", lval.integer, rval.integer);
	    retval->boolean = lval.integer == rval.integer;
	    break;
	case NX_VALUE_TYPE_BOOLEAN:
	    //log_debug("BOOLEAN: %d == %d", lval.boolean, rval.boolean);
	    retval->boolean = lval.boolean == rval.boolean;
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    retval->boolean = lval.datetime == rval.datetime;
	    break;
	case NX_VALUE_TYPE_IP4ADDR:
	    retval->boolean =
		(lval.ip4addr[0] == rval.ip4addr[0]) && (lval.ip4addr[1] == rval.ip4addr[1]) && 
		(lval.ip4addr[2] == rval.ip4addr[2]) && (lval.ip4addr[3] == rval.ip4addr[3]);
    	    break;
	default:
	{
	    nx_value_type_t rtype, ltype;
	    
	    rtype = rval.type;
	    ltype = lval.type;
	    nx_value_kill(&rval);
	    nx_value_kill(&lval);

	    throw_msg("invalid types in operation: %s == %s",
		      nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
	}
    }

    nx_value_kill(&lval);
    nx_value_kill(&rval);
}



static void _notequal(nx_expr_eval_ctx_t *eval_ctx,
		      nx_value_t *retval,
		      const nx_expr_t *left,
		      const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(retval != NULL);
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    nx_expr_evaluate(eval_ctx, &lval, left);
    nx_expr_evaluate(eval_ctx, &rval, right);
    if ( lval.defined == FALSE )
    {
	if ( rval.defined == FALSE )
	{ // undef != undef = FALSE
	    retval->defined = TRUE;
	    retval->boolean = FALSE;
	    return;
	}
	retval->defined = FALSE;
	nx_value_kill(&rval);
	return;
    }

    if ( rval.defined == FALSE )
    {
	retval->defined = FALSE;
	nx_value_kill(&lval);
	return;
    }

    log_debug("%s != %s", nx_value_type_to_string(lval.type),
	      nx_value_type_to_string(rval.type));

    if ( lval.type != rval.type )
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s != %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }

    retval->defined = TRUE;

    switch ( lval.type )
    {
	case NX_VALUE_TYPE_STRING:
	    log_debug("%s != %s", lval.string->buf, rval.string->buf);
	    retval->boolean = strcmp(lval.string->buf, rval.string->buf) != 0;
	    break;
	case NX_VALUE_TYPE_INTEGER:
	    log_debug("%ld != %ld", lval.integer, rval.integer);
	    retval->boolean = lval.integer != rval.integer;
	    break;
	case NX_VALUE_TYPE_BOOLEAN:
	    retval->boolean = lval.boolean != rval.boolean;
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    retval->boolean = lval.datetime != rval.datetime;
	    break;
	case NX_VALUE_TYPE_IP4ADDR:
	    retval->boolean =
		(lval.ip4addr[0] != rval.ip4addr[0]) || (lval.ip4addr[1] != rval.ip4addr[1]) ||
		(lval.ip4addr[2] != rval.ip4addr[2]) || (lval.ip4addr[3] != rval.ip4addr[3]);
    	    break;
	default:
	{
	    nx_value_type_t rtype, ltype;
	    
	    rtype = rval.type;
	    ltype = lval.type;
	    nx_value_kill(&rval);
	    nx_value_kill(&lval);

	    throw_msg("invalid types in operation: %s != %s",
		      nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
	}
    }

    nx_value_kill(&lval);
    nx_value_kill(&rval);
}



static void _less(nx_expr_eval_ctx_t *eval_ctx,
		  nx_value_t *retval,
		  const nx_expr_t *left,
		  const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(retval != NULL);
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    nx_expr_evaluate(eval_ctx, &lval, left);
    if ( lval.defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);
    if ( rval.defined == FALSE )
    {
	retval->defined = FALSE;
	nx_value_kill(&lval);
	return;
    }

    if ( lval.type != rval.type )
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s < %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }
    retval->defined = TRUE;

    switch ( lval.type )
    {
	case NX_VALUE_TYPE_INTEGER:
	    retval->boolean = lval.integer < rval.integer;
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    retval->boolean = lval.datetime < rval.datetime;
	    break;
	default:
	{
	    nx_value_type_t rtype, ltype;

	    rtype = rval.type;
	    ltype = lval.type;
	    nx_value_kill(&rval);
	    nx_value_kill(&lval);

	    throw_msg("invalid types in operation: %s < %s",
		      nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
	}
    }
}



static void _le(nx_expr_eval_ctx_t *eval_ctx,
		nx_value_t *retval,
		const nx_expr_t *left,
		const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(retval != NULL);
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    nx_expr_evaluate(eval_ctx, &lval, left);
    if ( lval.defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);
    if ( rval.defined == FALSE )
    {
	retval->defined = FALSE;
	nx_value_kill(&lval);
	return;
    }

    if ( lval.type != rval.type )
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s <= %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }
    retval->defined = TRUE;

    switch ( lval.type )
    {
	case NX_VALUE_TYPE_INTEGER:
	    retval->boolean = lval.integer <= rval.integer;
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    retval->boolean = lval.datetime <= rval.datetime;
	    break;
	default:
	{
	    nx_value_type_t rtype, ltype;

	    rtype = rval.type;
	    ltype = lval.type;
	    nx_value_kill(&rval);
	    nx_value_kill(&lval);

	    throw_msg("invalid types in operation: %s <= %s",
		      nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
	}
    }
}



static void _greater(nx_expr_eval_ctx_t *eval_ctx,
		     nx_value_t *retval,
		     const nx_expr_t *left,
		     const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(retval != NULL);
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    nx_expr_evaluate(eval_ctx, &lval, left);
    if ( lval.defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);
    if ( rval.defined == FALSE )
    {
	retval->defined = FALSE;
	nx_value_kill(&lval);
	return;
    }

    if ( lval.type != rval.type )
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s > %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }
    retval->defined = TRUE;

    switch ( lval.type )
    {
	case NX_VALUE_TYPE_INTEGER:
	    retval->boolean = lval.integer > rval.integer;
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    retval->boolean = lval.datetime > rval.datetime;
	    break;
	default:
	{
	    nx_value_type_t rtype, ltype;

	    rtype = rval.type;
	    ltype = lval.type;
	    nx_value_kill(&rval);
	    nx_value_kill(&lval);

	    throw_msg("invalid types in operation: %s > %s",
		      nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
	}
    }
}



static void _greater_equal(nx_expr_eval_ctx_t *eval_ctx,
			   nx_value_t *retval,
			   const nx_expr_t *left,
			   const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(retval != NULL);
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    nx_expr_evaluate(eval_ctx, &lval, left);
    if ( lval.defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);
    if ( rval.defined == FALSE )
    {
	retval->defined = FALSE;
	nx_value_kill(&lval);
	return;
    }

    if ( lval.type != rval.type )
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s >= %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }
    retval->defined = TRUE;

    switch ( lval.type )
    {
	case NX_VALUE_TYPE_INTEGER:
	    retval->boolean = lval.integer >= rval.integer;
	    break;
	case NX_VALUE_TYPE_DATETIME:
	    retval->boolean = lval.datetime >= rval.datetime;
	    break;
	default:
	{
	    nx_value_type_t rtype, ltype;

	    rtype = rval.type;
	    ltype = lval.type;
	    nx_value_kill(&rval);
	    nx_value_kill(&lval);

	    throw_msg("invalid types in operation: %s >= %s",
		      nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
	}
    }
}



static void _and(nx_expr_eval_ctx_t *eval_ctx,
		 nx_value_t *retval,
		 const nx_expr_t *left,
		 const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(retval != NULL);
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    nx_expr_evaluate(eval_ctx, &lval, left);
    if ( lval.defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }
    else if ( (lval.type == NX_VALUE_TYPE_BOOLEAN) && (lval.boolean == FALSE) )
    { // short-circuit if left operand is false
	retval->defined = TRUE;
	retval->boolean = FALSE;
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);
    if ( rval.defined == FALSE )
    {
	retval->defined = FALSE;
	nx_value_kill(&lval);
	return;
    }

    if ( !((lval.type == NX_VALUE_TYPE_BOOLEAN) &&
	   (rval.type == NX_VALUE_TYPE_BOOLEAN)) )
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s and %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }

    retval->defined = TRUE;
    retval->boolean = lval.boolean && rval.boolean;
}



static void _or(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *left, const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(left != NULL);
    ASSERT(right != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;

    nx_expr_evaluate(eval_ctx, &lval, left);

    if ( lval.type != NX_VALUE_TYPE_BOOLEAN )
    {
	nx_value_type_t ltype;

	ltype = lval.type;
	nx_value_kill(&lval);

	throw_msg("boolean type expected for left value in and operation, found %s",
		  nx_value_type_to_string(ltype));
    }

    if ( (lval.defined == TRUE) && (lval.boolean == TRUE) )
    {
	retval->defined = TRUE;
	retval->boolean = TRUE;
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);

    if ( rval.type != NX_VALUE_TYPE_BOOLEAN )
    {
	nx_value_type_t rtype, ltype;

	rtype = rval.type;
	ltype = lval.type;
	nx_value_kill(&rval);
	nx_value_kill(&lval);

	throw_msg("invalid types in operation: %s or %s",
		  nx_value_type_to_string(ltype), nx_value_type_to_string(rtype));
    }

    if ( rval.defined == TRUE )
    {
	retval->defined = TRUE;
	retval->boolean = rval.boolean;
	return;
    }

    retval->defined = FALSE;
}



static void _plus(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *left, const nx_expr_t *right)
{
    nx_value_t lval, rval;
 
    ASSERT(left != NULL);
    ASSERT(right != NULL);

    nx_expr_evaluate(eval_ctx, &lval, left);
    nx_expr_evaluate(eval_ctx, &rval, right);

    if ( (lval.defined == FALSE) || (rval.defined == FALSE) ) 
    {
	if ( (lval.defined == TRUE) &&
	     (rval.defined == FALSE) &&
	     (lval.type == NX_VALUE_TYPE_STRING) )
	{
	    // string + undef = string
	    nx_value_init_string(retval, lval.string->buf);
	    nx_value_kill(&lval);

	    return;
	}

	if ( (rval.defined == TRUE) &&
	     (lval.defined == FALSE) &&
	     (rval.type == NX_VALUE_TYPE_STRING) )
	{
	    // undef + string = string
	    nx_value_init_string(retval, rval.string->buf);
	    nx_value_kill(&rval);

	    return;
	}

	// undef + undef = undef
	// !string + undef = undef
	// undef + !string = undef
	retval->defined = FALSE;
	retval->type = NX_VALUE_TYPE_UNKNOWN;
	nx_value_kill(&rval);
	nx_value_kill(&lval);
	return;
    }

    if ( (lval.type == NX_VALUE_TYPE_STRING) || (rval.type == NX_VALUE_TYPE_STRING) )
    {
	// string + string = string
	// ? + string = string
	// string + ? = string
	char *l, *r;
	boolean free_r = FALSE, free_l = FALSE;

	retval->defined = TRUE;
	retval->type = NX_VALUE_TYPE_STRING;

	if ( rval.type == NX_VALUE_TYPE_STRING )
	{
	    r = rval.string->buf;
	}
	else
	{
	    r = nx_value_to_string(&rval);
	    free_r = TRUE;
	}

	if ( lval.type == NX_VALUE_TYPE_STRING )
	{
	    l = lval.string->buf;
	}
	else
	{
	    l = nx_value_to_string(&lval);
	    free_l = TRUE;
	}

	retval->string = nx_string_sprintf(NULL, "%s%s", l, r);
	if ( free_l == TRUE )
	{
	    free(l);
	}
	if ( free_r == TRUE )
	{
	    free(r);
	}
	nx_value_kill(&lval);
	nx_value_kill(&rval);

	return;
    }
    
    if ( (lval.type == NX_VALUE_TYPE_INTEGER) && (rval.type == NX_VALUE_TYPE_INTEGER) )
    {
	retval->integer = lval.integer + rval.integer;
	retval->type = NX_VALUE_TYPE_INTEGER;
	retval->defined = TRUE;

	return;
    }

    if ( (lval.type == NX_VALUE_TYPE_DATETIME) && (rval.type == NX_VALUE_TYPE_INTEGER) )
    {
	retval->defined = TRUE;
	retval->type = NX_VALUE_TYPE_DATETIME;
	retval->datetime = lval.datetime + (rval.integer * APR_USEC_PER_SEC);
	return;
    }

    if ( (lval.type == NX_VALUE_TYPE_INTEGER) && (rval.type == NX_VALUE_TYPE_DATETIME) )
    {
	retval->defined = TRUE;
	retval->type = NX_VALUE_TYPE_DATETIME;
	retval->datetime = rval.datetime + (lval.integer * APR_USEC_PER_SEC);
	return;
    }

    nx_value_kill(&lval);
    nx_value_kill(&rval);

    throw_msg("invalid operation: %s + %s",
	      nx_value_type_to_string(lval.type),
	      nx_value_type_to_string(rval.type));
}



static void _minus(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *left, const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(left != NULL);
    ASSERT(right != NULL);

    nx_expr_evaluate(eval_ctx, &lval, left);

    retval->defined = FALSE;
    if ( lval.defined == FALSE )
    {
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);

    if ( rval.defined == FALSE )
    {
	nx_value_kill(&lval);
	return;
    }

    if ( rval.type == lval.type )
    {
	// integer - integer
	if ( rval.type == NX_VALUE_TYPE_INTEGER )
	{
	    retval->integer = lval.integer - rval.integer;
	    retval->type = NX_VALUE_TYPE_INTEGER;
	    retval->defined = TRUE;
	    return;
	}
	// datetime - datetime
	else if ( rval.type == NX_VALUE_TYPE_DATETIME )
	{
	    retval->integer = lval.datetime - rval.datetime;
	    retval->type = NX_VALUE_TYPE_INTEGER;
	    retval->defined = TRUE;
	    return;
	}
    }
    // datetime - integer
    if ( (lval.type == NX_VALUE_TYPE_DATETIME) && (rval.type == NX_VALUE_TYPE_INTEGER) )
    {
	retval->datetime = lval.datetime - (rval.integer * APR_USEC_PER_SEC);
	retval->type = NX_VALUE_TYPE_DATETIME;
	retval->defined = TRUE;
	return;
    }

    nx_value_kill(&rval);
    nx_value_kill(&lval);

    throw_msg("invalid operation: %s - %s",
	      nx_value_type_to_string(lval.type),
	      nx_value_type_to_string(rval.type));
}



static void _mul(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *left, const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(left != NULL);
    ASSERT(right != NULL);

    nx_expr_evaluate(eval_ctx, &lval, left);

    retval->defined = FALSE;
    if ( lval.defined == FALSE )
    {
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);

    if ( rval.defined == FALSE )
    {
	nx_value_kill(&lval);
	return;
    }

    if ( rval.type == lval.type )
    {
	if ( rval.type == NX_VALUE_TYPE_INTEGER )
	{
	    retval->integer = lval.integer * rval.integer;
	    retval->type = NX_VALUE_TYPE_INTEGER;
	    retval->defined = TRUE;
	}
	else
	{
	    nx_value_kill(&lval);
	    nx_value_kill(&rval);
	    throw_msg("invalid operation: %s * %s",
		      nx_value_type_to_string(lval.type),
		      nx_value_type_to_string(rval.type));
 	}
    }
    else
    {
	nx_value_kill(&lval);
	nx_value_kill(&rval);
	throw_msg("invalid operation: %s * %s",
		  nx_value_type_to_string(lval.type),
		  nx_value_type_to_string(rval.type));
    }
}



static void _div(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *left, const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(left != NULL);
    ASSERT(right != NULL);

    nx_expr_evaluate(eval_ctx, &lval, left);

    retval->defined = FALSE;
    if ( lval.defined == FALSE )
    {
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);

    if ( rval.defined == FALSE )
    {
	nx_value_kill(&lval);
	return;
    }

    if ( rval.type == lval.type )
    {
	if ( rval.type == NX_VALUE_TYPE_INTEGER )
	{
	    retval->integer = lval.integer / rval.integer;
	    retval->type = NX_VALUE_TYPE_INTEGER;
	    retval->defined = TRUE;
	}
	else
	{
	    nx_value_kill(&lval);
	    nx_value_kill(&rval);
	    throw_msg("invalid operation: %s / %s",
		      nx_value_type_to_string(lval.type),
		      nx_value_type_to_string(rval.type));
 	}
    }
    else
    {
	nx_value_kill(&lval);
	nx_value_kill(&rval);
	throw_msg("invalid operation: %s / %s",
		  nx_value_type_to_string(lval.type),
		  nx_value_type_to_string(rval.type));
    }
}



static void _mod(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *left, const nx_expr_t *right)
{
    nx_value_t lval, rval;

    ASSERT(left != NULL);
    ASSERT(right != NULL);

    nx_expr_evaluate(eval_ctx, &lval, left);

    retval->defined = FALSE;
    if ( lval.defined == FALSE )
    {
	return;
    }

    nx_expr_evaluate(eval_ctx, &rval, right);

    if ( rval.defined == FALSE )
    {
	nx_value_kill(&lval);
	return;
    }

    if ( rval.type == lval.type )
    {
	if ( rval.type == NX_VALUE_TYPE_INTEGER )
	{
	    retval->integer = lval.integer % rval.integer;
	    retval->type = NX_VALUE_TYPE_INTEGER;
	    retval->defined = TRUE;
	}
	else
	{
	    nx_value_kill(&lval);
	    nx_value_kill(&rval);
	    throw_msg("invalid operation: %s %% %s",
		      nx_value_type_to_string(lval.type),
		      nx_value_type_to_string(rval.type));
 	}
    }
    else
    {
	nx_value_kill(&lval);
	nx_value_kill(&rval);
	throw_msg("invalid operation: %s %% %s",
		  nx_value_type_to_string(lval.type),
		  nx_value_type_to_string(rval.type));
    }
}



nx_expr_t *nx_expr_new_binop(nx_expr_parser_t *parser,
			     int token,
			     const nx_expr_t *left,
			     const nx_expr_t *right)
{
    nx_expr_t *expr;

    if ( (left == NULL) || (right == NULL) )
    {
	return ( NULL );
    }

    ASSERT(left->type != 0);
    ASSERT(right->type != 0);
    ASSERT(left->rettype != 0);
    ASSERT(right->rettype != 0);

    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));
    expr->type = NX_EXPR_TYPE_BINOP;
    expr->binop.left = left;
    expr->binop.right = right;
    nx_expr_decl_init(&(expr->decl), parser, "binary operation");

    switch ( token )
    {
	case TOKEN_REGMATCH:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expr");
	    }
	    else if ( !(((left->rettype == NX_VALUE_TYPE_REGEXP) &&
			 (right->rettype == NX_VALUE_TYPE_STRING)) ||
			((right->rettype == NX_VALUE_TYPE_REGEXP) &&
			 (left->rettype == NX_VALUE_TYPE_STRING))) )
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s =~ %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    if ( (right->rettype == NX_VALUE_TYPE_REGEXP) &&
		 (right->value.regexp.replacement != NULL) &&
		 (left->type != NX_EXPR_TYPE_FIELD) )
	    {
		throw_msg("field required for left operand of regexp substitution");
	    }
	    if ( (left->rettype == NX_VALUE_TYPE_REGEXP) &&
		 (left->value.regexp.replacement != NULL) )
	    {
		throw_msg("regexp substitution operand must be the right in regexp substitution");
	    }

	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_regmatch;
	    break;
	case TOKEN_NOTREGMATCH:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( !(((left->rettype == NX_VALUE_TYPE_REGEXP) &&
			 (right->rettype == NX_VALUE_TYPE_STRING)) ||
			((right->rettype == NX_VALUE_TYPE_REGEXP) &&
			 (left->rettype == NX_VALUE_TYPE_STRING))) )
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s !~ %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    if ( (right->rettype == NX_VALUE_TYPE_REGEXP) &&
		 (right->value.regexp.replacement != NULL) &&
		 (left->type != NX_EXPR_TYPE_FIELD) )
	    {
		throw_msg("field required for left operand of regexp substitution");
	    }
	    if ( (left->rettype == NX_VALUE_TYPE_REGEXP) &&
		 (left->value.regexp.replacement != NULL) )
	    {
		throw_msg("regexp substitution operand must be the right in regexp substitution");
	    }

	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_notregmatch;
	    break;
	case TOKEN_EQUAL:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( left->rettype == right->rettype )
	    {
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s == %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_equal;
	    break;
	case TOKEN_NOTEQUAL:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( left->rettype == right->rettype )
	    {
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL, 
					 "invalid binary operation: %s != %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_notequal;
	    break;
	case TOKEN_LESS:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_DATETIME) &&
		      (left->rettype == NX_VALUE_TYPE_DATETIME) )
	    {
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s < %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_less;
	    break;
	case TOKEN_LE:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_DATETIME) &&
		      (left->rettype == NX_VALUE_TYPE_DATETIME) )
	    {
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s <= %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_le;
	    break;
	case TOKEN_GREATER:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_DATETIME) &&
		      (left->rettype == NX_VALUE_TYPE_DATETIME) )
	    {
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s > %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_greater;
	    break;
	case TOKEN_GE:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_DATETIME) &&
		      (left->rettype == NX_VALUE_TYPE_DATETIME) )
	    {
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s >= %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_greater_equal;
	    break;
	case TOKEN_AND:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_BOOLEAN) &&
		      (left->rettype == NX_VALUE_TYPE_BOOLEAN) )
	    {
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s AND %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    expr->binop.cb = &_and;
	    break;
	case TOKEN_OR:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_BOOLEAN) &&
		      (left->rettype == NX_VALUE_TYPE_BOOLEAN) )
	    {
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s OR %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->binop.cb = &_or;
	    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
	    break;
	case TOKEN_PLUS:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
		expr->rettype = NX_VALUE_TYPE_UNKNOWN;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
		expr->rettype = NX_VALUE_TYPE_INTEGER;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_DATETIME) )
	    {
		expr->rettype = NX_VALUE_TYPE_DATETIME;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_DATETIME) &&
		      (left->rettype == NX_VALUE_TYPE_DATETIME) )
	    {
		expr->rettype = NX_VALUE_TYPE_INTEGER;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_STRING) ||
		      (left->rettype == NX_VALUE_TYPE_STRING) )
	    {
		expr->rettype = NX_VALUE_TYPE_STRING;
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s + %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->binop.cb = &_plus;
	    break;
	case TOKEN_MINUS:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
		expr->rettype = NX_VALUE_TYPE_UNKNOWN;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
		expr->rettype = NX_VALUE_TYPE_INTEGER;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_DATETIME) )
	    {
		expr->rettype = NX_VALUE_TYPE_DATETIME;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_DATETIME) &&
		      (left->rettype == NX_VALUE_TYPE_DATETIME) )
	    {
		expr->rettype = NX_VALUE_TYPE_INTEGER;
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s - %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->binop.cb = &_minus;
	    break;
	case TOKEN_MUL:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
		expr->rettype = NX_VALUE_TYPE_UNKNOWN;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
		expr->rettype = NX_VALUE_TYPE_INTEGER;
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s * %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->binop.cb = &_mul;
	    break;
	case TOKEN_DIV:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
		expr->rettype = NX_VALUE_TYPE_UNKNOWN;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
		expr->rettype = NX_VALUE_TYPE_INTEGER;
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s / %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->binop.cb = &_div;
	    break;
	case TOKEN_MOD:
	    if ( (left->rettype == NX_VALUE_TYPE_UNKNOWN) ||
		 (right->rettype == NX_VALUE_TYPE_UNKNOWN) )
	    {
		//log_warn("unknown type in expression");
		expr->rettype = NX_VALUE_TYPE_UNKNOWN;
	    }
	    else if ( (right->rettype == NX_VALUE_TYPE_INTEGER) &&
		      (left->rettype == NX_VALUE_TYPE_INTEGER) )
	    {
		expr->rettype = NX_VALUE_TYPE_INTEGER;
	    }
	    else
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid binary operation: %s %% %s",
					 nx_value_type_to_string(left->rettype),
					 nx_value_type_to_string(right->rettype));
	    }
	    expr->binop.cb = &_mod;
	    break;
	default:
	    nx_panic("invalid/unhandled token");
    }

    return ( expr );
}



static void _neg(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *op)
{
    nx_value_t val;

    ASSERT(op != NULL);

    retval->type = NX_VALUE_TYPE_INTEGER;
    nx_expr_evaluate(eval_ctx, &val, op);

    if ( val.type != NX_VALUE_TYPE_INTEGER )
    {
	nx_value_type_t type;

	type = val.type;
	nx_value_kill(&val);

	throw_msg("integer type expected for unary negation operation, found %s",
		  nx_value_type_to_string(type));
    }

    if ( val.defined == TRUE )
    {
	retval->defined = TRUE;
	retval->integer = val.integer;
	return;
    }
    
    retval->defined = FALSE;
}



static void _not(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *op)
{
    nx_value_t val;

    ASSERT(op != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;
    nx_expr_evaluate(eval_ctx, &val, op);

    if ( val.defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    if ( val.type != NX_VALUE_TYPE_BOOLEAN )
    {
	nx_value_kill(&val);
	throw_msg("boolean value required for 'not'");
    }

    retval->defined = TRUE;
    retval->boolean = !val.boolean;
    nx_value_kill(&val);
}



static void _defined(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *op)
{
    nx_value_t val;

    ASSERT(op != NULL);

    retval->type = NX_VALUE_TYPE_BOOLEAN;
    retval->defined = TRUE;
    nx_expr_evaluate(eval_ctx, &val, op);

    if ( val.defined == TRUE )
    {
	retval->boolean = TRUE;
    }
    else
    {
	retval->boolean = FALSE;
    }
    nx_value_kill(&val);
}



nx_expr_t *nx_expr_new_unop(nx_expr_parser_t *parser,
			    int token,
			    const nx_expr_t *op)
{
    nx_expr_t *expr;
    
    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));
    expr->type = NX_EXPR_TYPE_UNOP;
    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
    expr->unop.op = op;
    nx_expr_decl_init(&(expr->decl), parser, "unary operation");

    switch ( token )
    {
	case TOKEN_MINUS:
	    if ( !((op->rettype == NX_VALUE_TYPE_BOOLEAN) ||
		   (op->rettype == NX_VALUE_TYPE_UNKNOWN)) )
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid unary operation: -%s",
					 nx_value_type_to_string(op->rettype));
	    }
	    expr->unop.cb = &_neg;
	    break;
	case TOKEN_NOT:
	    if ( !((op->rettype == NX_VALUE_TYPE_BOOLEAN) ||
		   (op->rettype == NX_VALUE_TYPE_UNKNOWN)) )
	    {
		nx_expr_parser_error_fmt(parser, NULL,
					 "invalid unary operation: not %s",
					 nx_value_type_to_string(op->rettype));
	    }
	    expr->unop.cb = &_not;
	    break;
	case TOKEN_DEFINED:
	    expr->unop.cb = &_defined;
	    break;    
	default:
	    nx_panic("invalid/unhandled token");
    }

    return ( expr );
}



nx_expr_t *nx_expr_new_inop(nx_expr_parser_t *parser,
			    const nx_expr_t *expr,
			    const nx_expr_list_t *exprs)
{
    nx_expr_t *retval;
    
    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_t));
    retval->type = NX_EXPR_TYPE_INOP;
    retval->rettype = NX_VALUE_TYPE_BOOLEAN;
    retval->inop.expr = expr;
    retval->inop.exprs = exprs;
    nx_expr_decl_init(&(retval->decl), parser, "in");

    return ( retval );
}



nx_expr_t *nx_expr_new_string(nx_expr_parser_t *parser, const char *str)
{
    nx_expr_t *expr;
    
    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));

    expr->type = NX_EXPR_TYPE_VALUE;
    expr->rettype = NX_VALUE_TYPE_STRING;
    expr->value.type = NX_VALUE_TYPE_STRING;
    if ( str == NULL )
    {
	expr->value.string = nx_string_new();
    }
    else
    {
	expr->value.string = nx_string_create(str, -1);
    }
    expr->value.defined = TRUE;
    nx_expr_decl_init(&(expr->decl), parser, "string literal");

    return ( expr );
}



nx_expr_t *nx_expr_new_undef(nx_expr_parser_t *parser)
{
    nx_expr_t *expr;
    
    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));

    expr->type = NX_EXPR_TYPE_VALUE;
    expr->rettype = NX_VALUE_TYPE_UNKNOWN;
    expr->value.type = NX_VALUE_TYPE_UNKNOWN;
    expr->value.defined = FALSE;
    nx_expr_decl_init(&(expr->decl), parser, "undef literal");

    return ( expr );
}



nx_expr_t *nx_expr_new_regexp(nx_expr_parser_t *parser,
			      const char *str,
			      const char *replacement,
			      const char *modifiers)
{
    nx_expr_t *expr;
    const char *error = NULL;
    int erroroffs = 0;
    pcre *regexp;
    pcre *tmppcre;
    int rc;
    size_t size;
    int capturecount;
    uint8_t mod = 0;
    size_t i;
    int options = 0;

    ASSERT(str != NULL);

    if ( replacement != NULL )
    {
	log_debug("new regexpreplace: /%s/%s/", str, replacement);
    }
    else
    {
	log_debug("new regexp: /%s/", str);
    }

    if ( modifiers != NULL )
    {
	for ( i = 0; i < strlen(modifiers); i++ )
	{
	    switch ( modifiers[i] )
	    {
		case 'g':
		    mod |= NX_EXPR_REGEXP_MODIFIER_MATCH_GLOBAL;
		    break;
		case 's':
		    mod |= NX_EXPR_REGEXP_MODIFIER_MATCH_DOTALL;
		    break;
		case 'm':
		    mod |= NX_EXPR_REGEXP_MODIFIER_MATCH_MULTILINE;
		    break;
		case 'i':
		    mod |= NX_EXPR_REGEXP_MODIFIER_MATCH_CASELESS;
		    break;
		default:
		    nx_expr_parser_error_fmt(parser, NULL,
					     "regular expression modifier is invalid: %c",
					     modifiers[i]);
	    }
	}
    }

    if ( mod & NX_EXPR_REGEXP_MODIFIER_MATCH_DOTALL )
    {
	options |= PCRE_DOTALL;
    }
    if ( mod & NX_EXPR_REGEXP_MODIFIER_MATCH_MULTILINE )
    {
	options |= PCRE_MULTILINE;
    }
    if ( mod & NX_EXPR_REGEXP_MODIFIER_MATCH_CASELESS )
    {
	options |= PCRE_CASELESS;
    }

    regexp = pcre_compile(str, options, &error,  &erroroffs, NULL);

    if ( regexp == NULL )
    {
	nx_expr_parser_error_fmt(parser, NULL,
				 "failed to compile regular expression '%s', error at position %d: %s",
				 str, erroroffs, error);
    }

    rc = pcre_fullinfo(regexp, NULL, PCRE_INFO_SIZE, &size); 
    if ( rc < 0 )
    {
	pcre_free(regexp);
	throw_msg("failed to get compiled regexp size");
    }
    ASSERT(size > 0);

    rc = pcre_fullinfo(regexp, NULL, PCRE_INFO_CAPTURECOUNT, &capturecount); 
    if ( rc < 0 )
    {
	pcre_free(regexp);
	throw_msg("failed to get regexp captured count");
    }
    if ( capturecount >= NX_EXPR_MAX_CAPTURED_FIELDS )
    {
	pcre_free(regexp);
	throw_msg("maximum number of captured substrings is limited to %d",
		  NX_EXPR_MAX_CAPTURED_FIELDS);
    }

    tmppcre = apr_palloc(parser->pool, (apr_size_t) size);
    memcpy(tmppcre, regexp, (apr_size_t) size);
    pcre_free(regexp);

    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));

    if ( replacement != NULL )
    { // s/a/b/
	expr->type = NX_EXPR_TYPE_VALUE;
	expr->rettype = NX_VALUE_TYPE_REGEXP;
	expr->value.type = NX_VALUE_TYPE_REGEXP;
	expr->value.defined = TRUE;
	expr->value.regexp.pcre = tmppcre;
	expr->value.regexp.pcre_size = size;
	expr->value.regexp.str = apr_pstrdup(parser->pool, str);
	expr->value.regexp.replacement = apr_pstrdup(parser->pool, replacement);
	expr->value.regexp.modifiers = mod;
	nx_expr_decl_init(&(expr->decl), parser, "regexpreplace");
    }
    else
    {
	expr->type = NX_EXPR_TYPE_VALUE;
	expr->rettype = NX_VALUE_TYPE_REGEXP;
	expr->value.type = NX_VALUE_TYPE_REGEXP;
	expr->value.defined = TRUE;
	expr->value.regexp.pcre = tmppcre;
	expr->value.regexp.pcre_size = size;
	expr->value.regexp.str = apr_pstrdup(parser->pool, str);
	expr->value.regexp.replacement = NULL;
	expr->value.regexp.modifiers = mod;
	nx_expr_decl_init(&(expr->decl), parser, "regexp");
    }

    return ( expr );
}



nx_expr_t *nx_expr_new_field(nx_expr_parser_t *parser, const char *str)
{
    nx_expr_t *expr;
    
    log_debug("nx_expr_new_field: %s", str);

    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));

    expr->type = NX_EXPR_TYPE_FIELD;
    expr->field = apr_pstrdup(parser->pool, str);
    expr->rettype = NX_VALUE_TYPE_UNKNOWN;
    nx_expr_decl_init(&(expr->decl), parser, "field");

    return ( expr );
}



nx_expr_t *nx_expr_new_captured(nx_expr_parser_t *parser, const char *str)
{
    nx_expr_t *expr;
    int val;
  
    log_debug("nx_expr_new_captured: %s", str);

    ASSERT(strlen(str) > 0);

    if ( sscanf(str, "%d", &val) != 1 )
    {
	throw_msg("couldn't parse integer: %s", str);
    }

    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));
    expr->type = NX_EXPR_TYPE_CAPTURED;
    expr->captured = val;
    expr->rettype = NX_VALUE_TYPE_STRING;
    nx_expr_decl_init(&(expr->decl), parser, "captured string");

    return ( expr );
}



nx_expr_t *nx_expr_new_boolean(nx_expr_parser_t *parser, boolean value)
{
    nx_expr_t *expr;
    
    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));

    expr->type = NX_EXPR_TYPE_VALUE;
    expr->value.type = NX_VALUE_TYPE_BOOLEAN;
    expr->value.defined = TRUE;
    expr->value.boolean = value;
    expr->rettype = NX_VALUE_TYPE_BOOLEAN;
    nx_expr_decl_init(&(expr->decl), parser, "boolean literal");

    return ( expr );
}



nx_expr_t *nx_expr_new_integer(nx_expr_parser_t *parser, const char *str)
{
    int64_t value;
    nx_expr_t *expr;
    
    value = nx_value_parse_int(str);

    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));

    expr->type = NX_EXPR_TYPE_VALUE;
    expr->value.type = NX_VALUE_TYPE_INTEGER;
    expr->value.defined = TRUE;
    expr->value.integer = value;
    expr->rettype = NX_VALUE_TYPE_INTEGER;
    nx_expr_decl_init(&(expr->decl), parser, "integer literal");

    return ( expr );
}



nx_expr_t *nx_expr_new_datetime(nx_expr_parser_t *parser, const char *str)
{
    nx_expr_t *expr;
    apr_time_t t;
    
    CHECKERR_MSG(nx_date_parse_iso(&t, str, NULL), "couldn't parse date: %s", str);

    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));
    expr->type = NX_EXPR_TYPE_VALUE;
    expr->value.type = NX_VALUE_TYPE_DATETIME;
    expr->value.defined = TRUE;
    expr->value.datetime = t;
    expr->rettype = NX_VALUE_TYPE_DATETIME;
    nx_expr_decl_init(&(expr->decl), parser, "datetime literal");

    return ( expr );
}



nx_expr_t *nx_expr_new_ip4addr(nx_expr_parser_t *parser, const char *str)
{
    nx_expr_t *expr;
    
    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));
    expr->type = NX_EXPR_TYPE_VALUE;
    nx_value_parse_ip4addr(&(expr->value), str);
    expr->value.type = NX_VALUE_TYPE_IP4ADDR;
    expr->value.defined = TRUE;
    expr->rettype = NX_VALUE_TYPE_IP4ADDR;
    nx_expr_decl_init(&(expr->decl), parser, "ip4addr literal");

    return ( expr );
}



static boolean nx_expr_list_elem_match(const nx_expr_list_t *args,
				       int num_arg,
				       nx_value_type_t *arg_types)
{
    boolean retval = TRUE;
    nx_expr_list_elem_t *arg;
    int i;

    if ( (args == NULL) || (NX_DLIST_FIRST(args) == NULL) )
    {
	if ( num_arg == 0 )
	{
	    return ( TRUE );
	}
	else
	{
	    return ( FALSE );
	}
    }

    for ( arg = NX_DLIST_FIRST(args), i = 0;
	  arg != NULL;
	  arg = NX_DLIST_NEXT(arg, link), i++ )
    {
	if ( (i == num_arg - 1) &&
	     (arg_types[i] == NX_EXPR_VALUE_TYPE_VARARGS) )
	{
	    break;
	}
	if ( i >= num_arg )
	{
	    retval = FALSE;
	    break;
	}
	if ( (arg->expr->rettype != NX_VALUE_TYPE_UNKNOWN) && 
	     (arg_types[i] != arg->expr->rettype) &&
	     (arg_types[i] != NX_VALUE_TYPE_UNKNOWN) )
	{
	    retval = FALSE;
	    break;
	}
    }

    return ( retval );
}



nx_expr_t *nx_expr_new_function(nx_expr_parser_t *parser,
				const char *fname,
				nx_expr_list_t *args)
{
    nx_expr_t *expr;
    nx_ctx_t *ctx;
    char modname[256];
    const char *tmp = NULL;
    size_t len;
    nx_module_t *module = NULL;
    boolean found = FALSE;
    nx_expr_func_t *func;

    ctx = nx_ctx_get();

    log_debug("nx_expr_new_function: %s", fname);

    if ( (tmp = strstr(fname, "->")) != NULL )
    {
	len = (size_t) (tmp - fname) + 1;
	if ( len > sizeof(modname) )
	{
	    len = sizeof(modname);
	}
	apr_cpystrn(modname, fname, len);
	fname = tmp + 2;
	log_debug("module function (%s)->(%s)", modname, fname);
	for ( module = NX_DLIST_FIRST(ctx->modules);
	      module != NULL;
	      module = NX_DLIST_NEXT(module, link) )
	{
	    if ( strcmp(module->name, modname) == 0 )
	    {
		found = TRUE;
		break;
	    }
	}
	if ( (found != TRUE) || (module == NULL) )
	{
	    throw_msg("module %s not found", modname);
	}
    }
    else
    {
	log_debug("global function: %s", fname);
    }
    
    found = FALSE;
    // first look for private functions called without namespace
    if ( module == NULL )
    {
	for ( func = NX_DLIST_FIRST(ctx->expr_funcs);
	      func != NULL;
	      func = NX_DLIST_NEXT(func, link) )
	{
	    //log_info("%s <=> %s", func->name, fname);
	    if ( (func->type == NX_EXPR_FUNCPROC_TYPE_PRIVATE) &&
		 (parser->module == func->module) &&
		 (strcmp(func->name, fname) == 0) &&
		 (nx_expr_list_elem_match(args, func->num_arg, func->arg_types) == TRUE) )
	    {
		ASSERT(func->module != NULL);
		found = TRUE;
		log_debug("found private module function %s without namespace in module %s "
			  "provided by module %s", 
			  func->name, parser->module->name, func->module->name);
		break;
	    }
	}
	//log_info("NO MATCH: %s <=> %s", func->name, fname);
    }

    if ( found != TRUE )
    {
	for ( func = NX_DLIST_FIRST(ctx->expr_funcs);
	      func != NULL;
	      func = NX_DLIST_NEXT(func, link) )
	{
	    //log_info("%s <=> %s", func->name, fname);
	    if ( (strcmp(func->name, fname) == 0) &&
		 (nx_expr_list_elem_match(args, func->num_arg, func->arg_types) == TRUE) )
	    {
		switch ( func->type )
		{
		    case NX_EXPR_FUNCPROC_TYPE_GLOBAL:
			if ( module != NULL )
			{
			    log_debug("function %s is global but called via module %s",
				      func->name, module->name);
			}
			else
			{
			    found = TRUE;
			}
		    break;
		    case NX_EXPR_FUNCPROC_TYPE_PUBLIC:
			ASSERT(func->module != NULL);
			if ( module == NULL )
			{
			    log_debug("public function %s not called from module namespace",
				      func->name);
			}
			else if ( func->module == module )
			{
			    found = TRUE;
			}
			break;
		    case NX_EXPR_FUNCPROC_TYPE_PRIVATE:
			ASSERT(func->module != NULL);
			if ( module != func->module )
			{
			    log_debug("private function %s not called from own module namespace %s",
				      func->name, func->module->name);
			}
			else
			{
			    found = TRUE;
			}
			break;
		    default:
			nx_panic("invalid function type: %d", func->type);
		}
		if ( found == TRUE )
		{
		    break;
		}
	    }
	    //log_info("NO MATCH: %s <=> %s", func->name, fname);
	}
    }
    if ( (found != TRUE) || (func == NULL) )
    {
	throw_msg("function '%s()' does not exist or takes different arguments", fname);
    }

    expr = apr_pcalloc(parser->pool, sizeof(nx_expr_t));
    expr->rettype = func->rettype;
    expr->type = NX_EXPR_TYPE_FUNCTION;
    expr->function.cb = func->cb;
    expr->function.module = func->module;
    expr->function.args = args;
    nx_expr_decl_init(&(expr->decl), parser, func->name);

    return ( expr );
}



nx_expr_func_t *nx_expr_func_lookup(nx_expr_func_list_t		*expr_funcs,
				    const nx_module_t 		*module,
				    const char 			*fname,
				    nx_expr_funcproc_type_t	type,
				    nx_value_type_t		rettype,
				    int32_t 			num_arg,
				    nx_value_type_t		*arg_types)
{
    nx_expr_func_t *func;

    ASSERT(expr_funcs != NULL);

    for ( func = NX_DLIST_FIRST(expr_funcs);
	  func != NULL;
	  func = NX_DLIST_NEXT(func, link) )
    {
	if ( (strcasecmp(fname, func->name) == 0) &&
	     (type == func->type) &&
	     (module == func->module) &&
	     (num_arg == func->num_arg) &&
	     (rettype == func->rettype) &&
	     (((num_arg > 0 ) && (memcmp(arg_types, func->arg_types, (size_t) num_arg * sizeof(nx_value_type_t)) == 0)) ||
	      (num_arg == 0)) )
	{
	    return ( func );
	}
    }

    return ( NULL );
}



void nx_expr_func_register(apr_pool_t		*pool,
			   nx_expr_func_list_t	*expr_funcs,
			   nx_module_t		*module,
			   const char 		*fname,
			   nx_expr_funcproc_type_t	type,
			   nx_expr_func_cb_t 	*cb,
			   nx_value_type_t	rettype,
			   int32_t 		num_arg,
			   nx_value_type_t	*arg_types)
{
    nx_expr_func_t *func;

    if ( nx_expr_func_lookup(expr_funcs, module, fname, type, rettype, num_arg, arg_types) != NULL )
    {
	throw_msg("function '%s' is already registered", fname);
    }

    // it is safer to create a memory copy, because the module dso can be unloaded
    func = apr_pcalloc(pool, sizeof(nx_expr_func_t));

    func->module = module;
    func->name = apr_pstrdup(pool, fname);
    func->type = type;
    func->cb = cb;
    func->rettype = rettype;
    func->num_arg = num_arg;
    func->arg_types = apr_palloc(pool, (size_t) num_arg * sizeof(nx_value_type_t));
    memcpy(func->arg_types, arg_types, (size_t) num_arg * sizeof(nx_value_type_t));
    NX_DLIST_INSERT_TAIL(expr_funcs, func, link);

    log_debug("function '%s' registered", fname);
}



nx_expr_proc_t *nx_expr_proc_lookup(nx_expr_proc_list_t		*expr_procs,
				    const nx_module_t 		*module,
				    const char 			*fname,
				    nx_expr_funcproc_type_t	type,
				    int32_t 			num_arg,
				    nx_value_type_t		*arg_types)
{
    nx_expr_proc_t *proc;

    ASSERT(expr_procs != NULL);

    for ( proc = NX_DLIST_FIRST(expr_procs);
	  proc != NULL;
	  proc = NX_DLIST_NEXT(proc, link) )
    {
	if ( (strcasecmp(fname, proc->name) == 0) &&
	     (type == proc->type) &&
	     (module == proc->module) &&
	     (num_arg == proc->num_arg) &&
	     (((num_arg > 0 ) && (memcmp(arg_types, proc->arg_types, (size_t) num_arg * sizeof(nx_value_type_t)) == 0)) ||
	      (num_arg == 0)) )
	{
	    return ( proc );
	}
    }

    return ( NULL );
}



void nx_expr_proc_register(apr_pool_t		*pool,
			   nx_expr_proc_list_t	*expr_procs,
			   nx_module_t		*module,
			   const char 		*fname,
			   nx_expr_funcproc_type_t	type,
			   nx_expr_proc_cb_t 	*cb,
			   int32_t 		num_arg,
			   nx_value_type_t	*arg_types)
{
    nx_expr_proc_t *proc;

    if ( nx_expr_proc_lookup(expr_procs, module, fname, type, num_arg, arg_types) != NULL )
    {
	throw_msg("procedure '%s' is already registered", fname);
    }

    proc = apr_pcalloc(pool, sizeof(nx_expr_proc_t));

    proc->module = module;
    proc->name = apr_pstrdup(pool, fname);
    proc->type = type;
    proc->cb = cb;
    proc->num_arg = num_arg;
    proc->arg_types = apr_palloc(pool, (size_t) num_arg * sizeof(nx_value_type_t));
    memcpy(proc->arg_types, arg_types, (size_t) num_arg * sizeof(nx_value_type_t));
    NX_DLIST_INSERT_TAIL(expr_procs, proc, link);

    log_debug("procedure '%s' registered", fname);
}



nx_expr_list_t *nx_expr_list_new(nx_expr_parser_t *parser,
				 nx_expr_t *expr)
{
    nx_expr_list_t *retval;
    nx_expr_list_elem_t *arg;

    ASSERT(expr != NULL);

    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_list_t));
    NX_DLIST_INIT(retval, nx_expr_list_elem_t, link);
    arg = apr_pcalloc(parser->pool, sizeof(nx_expr_list_elem_t));
    arg->expr = expr;
   
    NX_DLIST_INSERT_TAIL(retval, arg, link);

    return ( retval );
}



nx_expr_list_t *nx_expr_list_add(nx_expr_parser_t *parser,
				 nx_expr_list_t *args,
				 nx_expr_t *expr)
{
    nx_expr_list_elem_t *arg;

    ASSERT(args != NULL);
    ASSERT(expr != NULL);

    arg = apr_pcalloc(parser->pool, sizeof(nx_expr_list_elem_t));
    arg->expr = expr;
   
    NX_DLIST_INSERT_TAIL(args, arg, link);

    return ( args );
}



nx_expr_statement_t *nx_expr_statement_new_assignment(nx_expr_parser_t *parser,
						      const nx_expr_t	*lval,
						      const nx_expr_t	*rval)
{
    nx_expr_statement_t *retval;

    ASSERT(lval != NULL);
    ASSERT(rval != NULL);

    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_statement_t));
    retval->type = NX_EXPR_STATEMENT_TYPE_ASSIGNMENT;
    retval->assignment.lval = lval;
    retval->assignment.rval = rval;
    nx_expr_decl_init(&(retval->decl), parser, "assignment");

    return ( retval );
}



nx_expr_statement_t *nx_expr_statement_new_ifelse(nx_expr_parser_t *parser,
						  const nx_expr_t *cond,
						  const nx_expr_statement_t *cond_true,
						  const nx_expr_statement_t *cond_false) ///< NULL if there is no ELSE branch
{
    nx_expr_statement_t *retval;

    ASSERT(cond != NULL);

    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_statement_t));
    retval->type = NX_EXPR_STATEMENT_TYPE_IFELSE;
    retval->ifelse.cond = cond;
    retval->ifelse.cond_true = cond_true;
    retval->ifelse.cond_false = cond_false;
    nx_expr_decl_init(&(retval->decl), parser, "if-else");

    return ( retval );
}



nx_expr_statement_t *nx_expr_statement_new_block(nx_expr_parser_t *parser,
						 const nx_expr_statement_list_t *stmnts)
{
    nx_expr_statement_t *retval;

    ASSERT(stmnts != NULL);

    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_statement_t));
    retval->type = NX_EXPR_STATEMENT_TYPE_BLOCK;
    retval->block.statements = stmnts;
    nx_expr_decl_init(&(retval->decl), parser, "block");

    return ( retval );
}



nx_expr_statement_t *nx_expr_statement_new_procedure(nx_expr_parser_t *parser,
						     const char *name,
						     nx_expr_list_t *args)
{
    nx_ctx_t *ctx;
    const char *tmp = NULL;
    char modname[256];
    size_t len;
    nx_module_t *module = NULL;
    boolean found = FALSE;
    nx_expr_proc_t *proc;
    nx_expr_statement_t *retval;

    ASSERT(name != NULL);

    ctx = nx_ctx_get();

    log_debug("nx_expr_new_procedure: %s", name);

    if ( (tmp = strstr(name, "->")) != NULL )
    {
	len = (size_t) (tmp - name) + 1;
	if ( len > sizeof(modname) )
	{
	    len = sizeof(modname);
	}
	apr_cpystrn(modname, name, len);
	name = tmp + 2;
	log_debug("module procedure (%s)->(%s)", modname, name);
	for ( module = NX_DLIST_FIRST(ctx->modules);
	      module != NULL;
	      module = NX_DLIST_NEXT(module, link) )
	{
	    if ( strcmp(module->name, modname) == 0 )
	    {
		found = TRUE;
		break;
	    }
	}
	if ( (found != TRUE) || (module == NULL) )
	{
	    throw_msg("module %s not found", modname);
	}
    }
    else
    {
	log_debug("global procedure: %s", name);
    }
    
    found = FALSE;

    // first look for private proedures called without namespace
    if ( module == NULL )
    {
	for ( proc = NX_DLIST_FIRST(ctx->expr_procs);
	      proc != NULL;
	      proc = NX_DLIST_NEXT(proc, link) )
	{
	    //log_info("%s <=> %s", proc->name, name);
	    if ( (proc->type == NX_EXPR_FUNCPROC_TYPE_PRIVATE) &&
		 (parser->module == proc->module) &&
		 (strcmp(proc->name, name) == 0) &&
		 (nx_expr_list_elem_match(args, proc->num_arg, proc->arg_types) == TRUE) )
	    {
		ASSERT(proc->module != NULL);
		found = TRUE;
		log_debug("found private module procedure %s without namespace in module %s "
			  "provided by module %s", 
			  proc->name, parser->module->name, proc->module->name);
		break;
	    }
	}
	//log_info("NO MATCH: %s <=> %s", proc->name, name);
    }

    if ( found != TRUE )
    {
	for ( proc = NX_DLIST_FIRST(ctx->expr_procs);
	      proc != NULL;
	      proc = NX_DLIST_NEXT(proc, link) )
	{
	    log_debug("checking procedure %s (%s - %s)", proc->name,
		      proc->module == NULL ? "NULL" : proc->module->name,
		      module == NULL ? "NULL" : module->name);
	    if ( (strcmp(proc->name, name) == 0) &&
		 (nx_expr_list_elem_match(args, proc->num_arg, proc->arg_types) == TRUE) )
	    {
		switch ( proc->type )
		{
		    case NX_EXPR_FUNCPROC_TYPE_GLOBAL:
			if ( module != NULL )
			{
			    log_debug("procedure %s is global but called via module %s",
				      proc->name, module->name);
			}
			else
			{
			    found = TRUE;
			}
			break;
		    case NX_EXPR_FUNCPROC_TYPE_PUBLIC:
			ASSERT(proc->module != NULL);
			if ( module == NULL )
			{
			    log_debug("public procedure %s not called from module namespace",
				      proc->name);
			}
			else if ( proc->module == module )
			{
			    found = TRUE;
			}
			break;
		    case NX_EXPR_FUNCPROC_TYPE_PRIVATE:
			ASSERT(proc->module != NULL);
			if ( module != proc->module )
			{
			    log_debug("private procedure %s not called from own module namespace %s",
				      proc->name, proc->module->name);
			}
			else
			{
			    found = TRUE;
			}
			break;
		    default:
			nx_panic("invalid procedure type: %d", proc->type);
		}
		if ( found == TRUE )
		{
		    break;
		}
	    }
	    //log_info("NO MATCH: %s <=> %s", proc->name, name);
	}
    }
    if ( (found != TRUE) || (proc == NULL) )
    {
	throw_msg("procedure '%s()' does not exist or takes different arguments", name);
    }

    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_statement_t));
    retval->type = NX_EXPR_STATEMENT_TYPE_PROCEDURE;
    retval->procedure.cb = proc->cb;
    retval->procedure.module = proc->module;
    retval->procedure.args = args;
    nx_expr_decl_init(&(retval->decl), parser, proc->name);

    return ( retval );
}



nx_expr_statement_t *nx_expr_statement_new_regexpreplace(nx_expr_parser_t *parser,
							 const nx_expr_t *lval,
							 const nx_expr_t *rval)
{
    nx_expr_statement_t *retval;

    ASSERT(lval != NULL);
    ASSERT(rval != NULL);

    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_statement_t));
    retval->type = NX_EXPR_STATEMENT_TYPE_REGEXPREPLACE;
    retval->regexpreplace.lval = lval;
    retval->regexpreplace.rval = rval;
    nx_expr_decl_init(&(retval->decl), parser, "regexpreplace");

    return ( retval );
}



nx_expr_statement_t *nx_expr_statement_new_regexp(nx_expr_parser_t *parser,
						  const nx_expr_t *lval,
						  const nx_expr_t *rval)
{
    nx_expr_statement_t *retval;

    ASSERT(lval != NULL);
    ASSERT(rval != NULL);

    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_statement_t));
    retval->type = NX_EXPR_STATEMENT_TYPE_REGEXP;
    retval->regexpreplace.lval = lval;
    retval->regexpreplace.rval = rval;
    nx_expr_decl_init(&(retval->decl), parser, "regexp");

    return ( retval );
}



nx_expr_statement_list_t *nx_expr_statement_list_new(nx_expr_parser_t *parser,
						     nx_expr_statement_t *stmnt)
{
    nx_expr_statement_list_t *retval;

    retval = apr_pcalloc(parser->pool, sizeof(nx_expr_statement_list_t));
    NX_DLIST_INIT(retval, nx_expr_statement_t, link);
    if ( stmnt != NULL )
    {
	NX_DLIST_INSERT_TAIL(retval, stmnt, link);
    }
    return ( retval );
}



nx_expr_statement_list_t *nx_expr_statement_list_add(nx_expr_parser_t *parser UNUSED,
						     nx_expr_statement_list_t *list,
						     nx_expr_statement_t *stmnt)
{
    ASSERT(list != NULL);

    if ( stmnt != NULL )
    {
	NX_DLIST_INSERT_TAIL(list, stmnt, link);
    }

    return ( list );
}
