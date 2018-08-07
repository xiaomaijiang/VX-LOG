/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "xml.h"

#include <expat.h>

#define NX_LOGMODULE NX_LOGMODULE_CORE


#define NS_SEPARATOR '|'


const char *xml_get_localname(const char *name)
{
    char *retval;

    retval = strchr(name, NS_SEPARATOR);
    if ( (retval != NULL) && (retval[0] == NS_SEPARATOR) )
    {
	return ( ++retval );
    }

    return ( name );
}


static void xml_start_handler(void *data, const char *el, const char **attr UNUSED)
{
    nx_xml_parser_ctx_t *ctx;

    ctx = (nx_xml_parser_ctx_t *) data;

    //log_info("start_handler: %s", xml_get_localname(el));

    (ctx->depth)++;
    if ( ctx->depth == 2 )
    {
	ctx->key = xml_get_localname(el);
    }
    else if ( ctx->depth > 2 )
    {
	if ( ctx->value == NULL )
	{
	    ctx->value = nx_string_create("<", 1);
	}
	else
	{
	    nx_string_append(ctx->value, "<", 1);
	}
	nx_string_append(ctx->value, el, -1);
	nx_string_append(ctx->value, ">", 1);
    }
}



static void xml_end_handler(void *data, const char *el)
{
    nx_xml_parser_ctx_t *ctx;
    nx_value_t *val;

    ctx = (nx_xml_parser_ctx_t *) data;

    //log_info("end_handler: %s", xml_get_localname(el));
    if ( ctx->depth == 2 )
    {
	ASSERT(ctx->key != NULL);
	if ( ctx->value == NULL )
	{
/* don't add empty values
	    val = nx_value_new(NX_VALUE_TYPE_STRING);
	    val->defined = FALSE;
	    nx_logdata_set_field_value(ctx->logdata, ctx->key, val);
*/
	}
	else
	{
	    val = nx_value_new(NX_VALUE_TYPE_STRING);
	    val->string = ctx->value;
	    //log_info("setting [%s:%s]", ctx->key, ctx->value->buf);
	    nx_logdata_set_field_value(ctx->logdata, ctx->key, val);
	    ctx->value = NULL;
	}
	ctx->key = NULL;
    }
    else if ( ctx->depth > 2 )
    {
	if ( ctx->value == NULL )
	{
	    ctx->value = nx_string_create("</", 2);
	}
	else
	{
	    nx_string_append(ctx->value, "</", 2);
	}
	nx_string_append(ctx->value, el, -1);
	nx_string_append(ctx->value, ">", 1);
    }

    (ctx->depth)--;
}



static void xml_char_data_handler(void *data, const char *s, int len)
{
    nx_xml_parser_ctx_t *ctx;
    ctx = (nx_xml_parser_ctx_t *) data;

    //log_info("char_data(%d): [%s]", len, s);
    if ( ctx->depth >= 2 )
    {
	if ( ctx->value == NULL )
	{
	    ctx->value = nx_string_create(s, len);
	}
	else
	{
	    nx_string_append(ctx->value, s, len);
	}
    }
}



void nx_xml_parse(nx_xml_parser_ctx_t *ctx,
		  const char *xml, size_t len)
{
    XML_Parser xp;
    nx_exception_t e;

    ASSERT(ctx->logdata != NULL);

    xp = XML_ParserCreateNS("UTF-8", NS_SEPARATOR);
    //xp = XML_ParserCreate("UTF-8");
    if ( xp == NULL )
    {
	throw_msg("XML_ParserCreate failed, memory allocation error?");
    }

    XML_SetElementHandler(xp, xml_start_handler, xml_end_handler);
    XML_SetCharacterDataHandler(xp, xml_char_data_handler);
    XML_SetUserData(xp, (void *) ctx);
    try
    {
	if ( !XML_Parse(xp, xml, (int) len, TRUE) )
	{
	    throw_msg("XML parse error at line %d: %s",
		      XML_GetCurrentLineNumber(xp),
		      XML_ErrorString(XML_GetErrorCode(xp)));
	}
    }
    catch(e)
    {
	if ( ctx->value != NULL )
	{
	    nx_string_free(ctx->value);
	    ctx->value = NULL;
	}
	XML_ParserFree(xp);
	rethrow(e);
    }
    XML_ParserFree(xp);

    if ( ctx->value != NULL )
    {
	nx_string_free(ctx->value);
	ctx->value = NULL;
    }

    if ( ctx->key != NULL )
    {
	ctx->key = NULL;
    }
}



nx_string_t *nx_logdata_to_xml(nx_xml_parser_ctx_t *ctx)
{
    nx_logdata_field_t *field;
    nx_string_t *retval;
    int i, start;
    char *value;
    char intstr[32];

    ASSERT(ctx->logdata != NULL);

    //retval = nx_string_create("<?xml version=\"1.0\" encoding=\"UTF-8\"?><event>", 45);
    retval = nx_string_create("<Event>", 7);

    for ( field = NX_DLIST_FIRST(&(ctx->logdata->fields));
	  field != NULL;
	  field = NX_DLIST_NEXT(field, link) )
    {
	if ( strcmp(field->key, "raw_event") == 0 )
	{
	    continue;
	}
	if ( (field->key[0] == '.') || (field->key[0] == '_') )
	{
	    continue;
	}

	if ( field->value->defined == FALSE )
	{
	    nx_string_append(retval, "<", 1);
	    nx_string_append(retval, field->key, -1);
	    nx_string_append(retval, "/>", 2);
	}
	else
	{
	    nx_string_append(retval, "<", 1);
	    nx_string_append(retval, field->key, -1);
	    nx_string_append(retval, ">", 1);

	    switch ( field->value->type )
	    {
		case NX_VALUE_TYPE_BOOLEAN:
		    if ( field->value->boolean == TRUE )
		    {
			nx_string_append(retval, "TRUE", 4);
		    }
		    else
		    {
			nx_string_append(retval, "FALSE", 5);
		    }
		    break;
		case NX_VALUE_TYPE_INTEGER:
		    nx_string_append(retval, intstr,
				     apr_snprintf(intstr, sizeof(intstr), "%"APR_INT64_T_FMT,
						  field->value->integer));
		    break;
		case NX_VALUE_TYPE_STRING:
		    // TODO: iterate properly over utf-8 characters
		    for ( i = 0, start = 0; i < (int) field->value->string->len; i++ )
		    {
			switch ( field->value->string->buf[i] )
			{
			    case '\"':
				nx_string_append(retval, field->value->string->buf + start,
						 i - start);
				nx_string_append(retval, "&quot;", 6);
				start = i + 1;
				break;
			    case '\'':
				nx_string_append(retval, field->value->string->buf + start,
						 i - start);
				nx_string_append(retval, "&apos;", 6);
				start = i + 1;
				break;
			    case '<':
				nx_string_append(retval, field->value->string->buf + start,
						 i - start);
				nx_string_append(retval, "&lt;", 4);
				start = i + 1;
				break;
			    case '>':
				nx_string_append(retval, field->value->string->buf + start,
						 i - start);
				nx_string_append(retval, "&gt;", 4);
				start = i + 1;
				break;
			    case '&':
				nx_string_append(retval, field->value->string->buf + start,
						 i - start);
				nx_string_append(retval, "&amp;", 5);
				start = i + 1;
				break;
			    case '\r':
				nx_string_append(retval, field->value->string->buf + start,
						 i - start);
				nx_string_append(retval, "&#xD;", 5);
				start = i + 1;
				break;
			    case '\n':
				nx_string_append(retval, field->value->string->buf + start,
						 i - start);
				nx_string_append(retval, "&#xA;", 5);
				start = i + 1;
				break;
			    default:
				break;
			}
		    }
		    if ( i > start )
		    {
			nx_string_append(retval, field->value->string->buf + start,
				     i - start);
		    }
		    break;
		default:
		    //TODO: escape?
		    value = nx_value_to_string(field->value);
		    nx_string_append(retval, value, -1);
		    free(value);
		    break;
	    }
	    nx_string_append(retval, "</", 2);
	    nx_string_append(retval, field->key, -1);
	    nx_string_append(retval, ">", 1);
	}
    }

    nx_string_append(retval, "</Event>", 8);

    return ( retval );
}
