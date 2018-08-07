/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "json.h"
#include "../../../common/exception.h"
#include "../../../common/date.h"

#include "yajl/api/yajl_gen.h"
#include "yajl/api/yajl_parse.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


static int yajl_parse_null_cb(void *data)  
{  
    nx_json_parser_ctx_t *ctx;
    nx_value_t *val;

    //log_info("null_cb");
    
    ctx = (nx_json_parser_ctx_t *) data;

    if ( (ctx->in_array > 0) || (ctx->in_map > 0) )
    {
	nx_string_append(ctx->tmpstr, "null,", 5);

	return ( 1 );
    }  

    if ( ctx->key == NULL )
    {
	throw_msg("map key name not found");
    }

    val = nx_value_new(NX_VALUE_TYPE_STRING);
    val->defined = FALSE;
    nx_logdata_set_field_value(ctx->logdata, ctx->key, val);
    free(ctx->key);
    ctx->key = NULL;

    return ( 1 );
}
 


static int yajl_parse_boolean_cb(void *data, int boolval)  
{
    nx_json_parser_ctx_t *ctx;
    nx_value_t *val;

    //log_info("boolean_cb");

    ctx = (nx_json_parser_ctx_t *) data;

    if ( (ctx->in_array > 0) || (ctx->in_map > 0) )
    {
	if ( boolval == 0 )
	{
	    nx_string_append(ctx->tmpstr, "FALSE,", 6);
	}
	else
	{
	    nx_string_append(ctx->tmpstr, "TRUE,", 6);
	}
	return ( 1 );
    }  
 
    if ( ctx->key == NULL )
    {
	throw_msg("map key name not found");
    }

    val = nx_value_new(NX_VALUE_TYPE_BOOLEAN);
    if ( boolval != 0 )
    {
	val->boolean = TRUE;
    }
    else
    {
	val->boolean = FALSE;
    }
    nx_logdata_set_field_value(ctx->logdata, ctx->key, val);
    free(ctx->key);
    ctx->key = NULL;

    return ( 1 );
}  



static int yajl_parse_integer_cb(void *data, long long integerval)
{
    nx_json_parser_ctx_t *ctx;
    char intstr[32];

    ctx = (nx_json_parser_ctx_t *) data;

    //log_info("integer_cb");

    if ( (ctx->in_array > 0) || (ctx->in_map > 0) )
    {
	apr_snprintf(intstr, sizeof(intstr), "%lld", integerval);
	nx_string_append(ctx->tmpstr, intstr, -1);
	nx_string_append(ctx->tmpstr, ",", 1);

	return ( 1 );
    }  

    if ( ctx->key == NULL )
    {
	throw_msg("map key name not found");
    }

    nx_logdata_set_integer(ctx->logdata, ctx->key, (int64_t) integerval);
    free(ctx->key);
    ctx->key = NULL;

    return ( 1 );
}



static int yajl_parse_double_cb(void *data, double doubleval)
{
    nx_json_parser_ctx_t *ctx;
    char intstr[32];

    //log_info("double_cb");

    ctx = (nx_json_parser_ctx_t *) data;

    if ( (ctx->in_array > 0) || (ctx->in_map > 0) )
    {
	apr_snprintf(intstr, sizeof(intstr), "%f", doubleval);
	nx_string_append(ctx->tmpstr, intstr, -1);
	nx_string_append(ctx->tmpstr, ",", 1);

	return ( 1 );
    }  

    if ( ctx->key == NULL )
    {
	throw_msg("map key name not found");
    }

    nx_logdata_set_integer(ctx->logdata, ctx->key, (int64_t) doubleval);
    free(ctx->key);
    ctx->key = NULL;

    return ( 1 );
}



static int yajl_parse_number_cb(void *data, const char *s, size_t l)  
{  
    nx_json_parser_ctx_t *ctx;
    long int intval;
    nx_value_t *val;

    //log_info("number_cb");

    ctx = (nx_json_parser_ctx_t *) data;

    if ( (ctx->in_array > 0) || (ctx->in_map > 0) )
    {
	nx_string_append(ctx->tmpstr, s, (int) l);
	nx_string_append(ctx->tmpstr, ",", 1);

	return ( 1 );
    }  

    if ( ctx->key == NULL )
    {
	throw_msg("map key name not found");
    }

    if ( sscanf(s, "%ld", &intval) != 1 )
    {
	val = nx_value_new(NX_VALUE_TYPE_STRING);
	val->string = nx_string_create(s, (int) l);
	nx_logdata_set_field_value(ctx->logdata, ctx->key, val);
    }
    else
    {
	nx_logdata_set_integer(ctx->logdata, ctx->key, (int64_t) intval);
    }
    free(ctx->key);
    ctx->key = NULL;

    return ( 1 );
}



static int yajl_parse_string_cb(void *data, const unsigned char *stringval,  
				size_t stringlen)  
{  
    nx_json_parser_ctx_t *ctx;
    nx_value_t *val;
    
    //log_info("string_cb: %s [%d]", stringval, stringlen);

    ctx = (nx_json_parser_ctx_t *) data;

    if ( (ctx->in_array > 0) || (ctx->in_map > 0) )
    {
	nx_string_append(ctx->tmpstr, (const char *) stringval, (int) stringlen);
	nx_string_append(ctx->tmpstr, ",", 1);

	return ( 1 );
    }  

    if ( ctx->key == NULL )
    {
	throw_msg("map key name not found");
    }

    if ( strcmp(ctx->key, "raw_event") != 0 )
    { // setting raw event would cause a free() on our buffer being parsed, ignore it

	// look at the string and check if it starts with a 4 digit year
	// if it looks like it, try to parse it as a datatime
	if ( ((stringval[0] == '1') || (stringval[0] == '2')) &&
	     apr_isdigit(stringval[1]) && apr_isdigit(stringval[2]) && 
	     apr_isdigit(stringval[3]) && (stringval[4] == '-') )
	{
	    apr_time_t t;
	    const char *dateend = NULL;
	    
	    if ( (nx_date_parse_iso(&t, (const char *) stringval, &dateend) == APR_SUCCESS) &&
		 (((const char *) stringval + stringlen) == dateend) )
	    { // we got a datetime string
		nx_logdata_set_datetime(ctx->logdata, ctx->key, t);
		goto done; // goto is not nice but is cleaner
	    }
	}

	val = nx_value_new(NX_VALUE_TYPE_STRING);
	val->string = nx_string_create((const char *) stringval, (int) stringlen);
    	nx_logdata_set_field_value(ctx->logdata, ctx->key, val);
    }
  done:

    free(ctx->key);
    ctx->key = NULL;

    return ( 1 );
}  

  
static int yajl_parse_map_key_cb(void *data, const unsigned char *stringval,  
				 size_t stringlen)  
{  
    nx_json_parser_ctx_t *ctx;

    //log_info("map_key_cb: %s", stringval);

    ctx = (nx_json_parser_ctx_t *) data;

    if ( ctx->in_map > 0 )
    {
	if ( ctx->tmpstr == NULL )
	{
	    throw_msg("tmpstr missing");
	}
	nx_string_append(ctx->tmpstr, "\"", 1);
	nx_string_append(ctx->tmpstr, (const char *) stringval, (int) stringlen);
	nx_string_append(ctx->tmpstr, "\":", 2);
    }
    else
    {
	if ( ctx->key != NULL )
	{
	    throw_msg("already got map key");
	}

	ctx->key = malloc(stringlen + 1);
	memcpy(ctx->key, stringval, stringlen);
	ctx->key[stringlen] = '\0';
    }

    return ( 1 );
}  



static int yajl_parse_start_map_cb(void *data)  
{  
    nx_json_parser_ctx_t *ctx;

    //log_info("start_map_cb");

    ctx = (nx_json_parser_ctx_t *) data;

    if ( ctx->in_array != 0 )
    {
	throw_msg("unexpected in_array");
    }

    if ( ctx->key != NULL )
    {
	(ctx->in_map)++;
	if ( ctx->tmpstr == NULL )
	{
	    ctx->tmpstr = nx_string_new();
	}
	nx_string_append(ctx->tmpstr, "{", 1);
    }

    return ( 1 );
}



static int yajl_parse_end_map_cb(void *data)  
{  
    nx_json_parser_ctx_t *ctx;

    //log_info("end_map_cb");

    ctx = (nx_json_parser_ctx_t *) data;

    if ( ctx->in_array != 0 )
    {
	throw_msg("unexpected in_array");
    }

    if ( ctx->key != NULL )
    {
	(ctx->in_map)--;
	if ( ctx->in_map < 0 )
	{
	    throw_msg("in_map is negative");
	}

	if ( ctx->tmpstr == NULL )
	{
	    throw_msg("tmpstr missing");
	}
	if ( ctx->tmpstr->len > 2 )
	{ // chop trailing comma
	    (ctx->tmpstr->len)--;
	}
	nx_string_append(ctx->tmpstr, "}", 1);
    
	if ( ctx->in_map == 0 )
	{
	    nx_logdata_set_string(ctx->logdata, ctx->key, ctx->tmpstr->buf);
	    nx_string_free(ctx->tmpstr);
	    ctx->tmpstr = NULL;
	    free(ctx->key);
	    ctx->key = NULL;
	}
    }

    return ( 1 );
}  

static int yajl_parse_start_array_cb(void *data)  
{  
    nx_json_parser_ctx_t *ctx;

    //log_info("start_array_cb");

    ctx = (nx_json_parser_ctx_t *) data;
    (ctx->in_array)++;

    if ( ctx->key == NULL )
    {
	throw_msg("map key name not found");
    }

    if ( ctx->tmpstr == NULL )
    {
	ctx->tmpstr = nx_string_new();
    }
    nx_string_append(ctx->tmpstr, "[", 1);

    return ( 1 );
}



static int yajl_parse_end_array_cb(void *data)  
{  
    nx_json_parser_ctx_t *ctx;

    //log_info("end_array_cb");

    ctx = (nx_json_parser_ctx_t *) data;

    if ( ctx->in_array == 0 )
    {
	throw_msg("in_array expected");
    }
    if ( ctx->tmpstr == NULL )
    {
	throw_msg("tmpstr missing");
    }

    if ( ctx->tmpstr->len > 2 )
    { // chop trailing comma
	(ctx->tmpstr->len)--;
    }
    nx_string_append(ctx->tmpstr, "]", 1);

    (ctx->in_array)--;
    if ( ctx->in_array == 0 )
    {
	if ( ctx->key == NULL )
	{
	    throw_msg("map key name not found");
	}

	nx_logdata_set_string(ctx->logdata, ctx->key, ctx->tmpstr->buf);
	nx_string_free(ctx->tmpstr);
	ctx->tmpstr = NULL;
	free(ctx->key);
	ctx->key = NULL;
    }

    return ( 1 );
}


  
static yajl_callbacks callbacks = {  
    yajl_parse_null_cb,  
    yajl_parse_boolean_cb,  
    yajl_parse_integer_cb,  
    yajl_parse_double_cb,  
    yajl_parse_number_cb,  
    yajl_parse_string_cb,  
    yajl_parse_start_map_cb,  
    yajl_parse_map_key_cb,  
    yajl_parse_end_map_cb,  
    yajl_parse_start_array_cb,  
    yajl_parse_end_array_cb,  
};  


  
void nx_json_parse(nx_json_parser_ctx_t *ctx,
		   const char *json, size_t len)
{
    yajl_handle hand;
    yajl_gen g;

    g = yajl_gen_alloc(NULL);  
    //yajl_gen_config(g, yajl_gen_validate_utf8, 1);  
  
    hand = yajl_alloc(&callbacks, NULL, (void *) ctx);  
    yajl_config(hand, yajl_allow_comments, 1);  

    if ( (yajl_parse(hand, (const unsigned char *) json, len) != yajl_status_ok) ||
	 (yajl_complete_parse(hand) != yajl_status_ok) )
    {
	unsigned char *errstr = yajl_get_error(hand, 1, (const unsigned char *) json, len);  

	log_error("failed to parse json string, %s [%s]", errstr, json);
        yajl_free_error(hand, errstr);  
    }  

    yajl_gen_free(g);
    yajl_free(hand);  
}



nx_string_t *nx_logdata_to_json(nx_json_parser_ctx_t *ctx)
{

    const unsigned char *json;
    size_t jsonlen;
    yajl_gen gen;
    nx_logdata_field_t *field;
    nx_string_t *retval;
    char *value;

    gen = yajl_gen_alloc(NULL);
    yajl_gen_map_open(gen);

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

	ASSERT(yajl_gen_string(gen, (const unsigned char *) field->key,
			       strlen(field->key)) == yajl_gen_status_ok);
	if ( field->value->defined == FALSE )
	{
	    ASSERT(yajl_gen_null(gen) == yajl_gen_status_ok);
	}
	else
	{
	    switch ( field->value->type )
	    {
		case NX_VALUE_TYPE_BOOLEAN:
		    ASSERT(yajl_gen_bool(gen, (int) field->value->boolean) == yajl_gen_status_ok);
		    break;
		case NX_VALUE_TYPE_INTEGER:
		    ASSERT(yajl_gen_integer(gen, (long long) field->value->integer) == yajl_gen_status_ok);
		    break;
		case NX_VALUE_TYPE_STRING:
		    ASSERT(yajl_gen_string(gen, (const unsigned char *) field->value->string->buf,
					   field->value->string->len) == yajl_gen_status_ok);
		    break;
		default:
		    value = nx_value_to_string(field->value);
		    ASSERT(yajl_gen_string(gen, (const unsigned char *) value,
					   strlen(value)) == yajl_gen_status_ok);
		    free(value);
		    break;
	    }
	}
    }
    yajl_gen_map_close(gen);
    yajl_gen_get_buf(gen, &json, &jsonlen);
    
    retval = nx_string_create((const char *) json, (int) jsonlen);
    yajl_gen_free(gen);

    return ( retval );
}
