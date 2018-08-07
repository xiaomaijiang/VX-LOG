/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/error_debug.h"
#include "../../../common/exception.h"
#include "csv.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

typedef enum nx_csv_state_t
{
    NX_CSV_STATE_START = 1,
    NX_CSV_STATE_INSIDE_FIELD, ///< inside quoted field
    NX_CSV_STATE_QUOTE,	///< quote inside quoted field
} nx_csv_state_t;



void nx_csv_ctx_set_quotechar(nx_csv_ctx_t *ctx, const char quotechar)
{
    ASSERT(ctx != NULL);
    
    ctx->quotechar = quotechar;
}



void nx_csv_ctx_set_delimiter(nx_csv_ctx_t *ctx, const char delimiter)
{
    ASSERT(ctx != NULL);
    
    ctx->delimiter = delimiter;
}



void nx_csv_ctx_set_escapechar(nx_csv_ctx_t *ctx, const char escapechar)
{
    ASSERT(ctx != NULL);
    
    ctx->escapechar = escapechar;
}



void nx_csv_ctx_init(nx_csv_ctx_t *ctx)
{
    ASSERT(ctx != NULL);

    ctx->quotechar = '\"';
    ctx->delimiter = ',';
    ctx->escapechar = '\\';
    ctx->escape_control = TRUE;
    ctx->quote_method = NX_CSV_QUOTE_METHOD_STRING;
    ctx->undefvalue = NULL;
}



char nx_csv_get_config_char(const char *str)
{
    char retval = '\0';
    size_t len;
    long int val;

    ASSERT(str != NULL);

    len = strlen(str);
    switch ( len )
    {
	case 3:
	    if ( (str[0] == '"') && (str[2] == '"') )
	    {
		retval = str[1];
	    }
	    else if ( (str[0] == '\'') && (str[2] == '\'') )
	    {
		retval = str[1];
	    }
	    break;
	case 2:
	    if ( str[0] == '\\' )
	    {
		switch ( str[1] )
		{
		    case 'a': //audible alert (bell)
			retval = '\a';
			break;
		    case 'b': // backspace
			retval = '\b';
			break;
		    case 't': // horizontal tab
			retval = '\t';
			break;
		    case 'n': // newline
			retval = '\n';
			break;
		    case 'v': // vertical tab
			retval = '\v';
			break;
		    case 'f': // formfeed
			retval = '\f';
			break;
		    case 'r': // carriage return
			retval = '\r';
			break;
		    default:
			break;
		}
	    }
	    break;
	case 1:
	    retval = str[0];
	    break;
	default:
	    if ( strncasecmp(str, "0X", 2) == 0 )
	    {
		if ( sscanf(str, "%lx", &val) != 1 )
		{
		    throw_msg("couldn't parse integer: %s", str);
		}
		else
		{
		    retval = (char) val;
		}
	    }
	    break;
    }

    return ( retval );
}



static void add_logdata_field(nx_csv_ctx_t *ctx,
			      nx_logdata_t *logdata, 
			      const char *key,
			      const char *strval,
			      nx_value_type_t type)
{
    nx_value_t *value;

    ASSERT(key != NULL);
    ASSERT(strval != NULL);

    //log_info("add field %s=%s", key, strval);
    if ( type == 0 )
    {
	type = NX_VALUE_TYPE_STRING;
    }

    if ( (ctx->undefvalue != NULL) && (strcmp(ctx->undefvalue, strval) == 0) )
    {
	value = nx_value_new(type);
	value->defined = FALSE;
    }
    else
    {
	value = nx_value_from_string(strval, type);
    }
    if ( value != NULL )
    {
	nx_logdata_set_field_value(logdata, key, value);
    }
}



static void unescape_string(nx_csv_ctx_t *ctx,
			    const char chr,
			    char **ptr)
{

    if ( ctx->escape_control == TRUE )
    {
	switch ( chr )
	{
	    case 'n':
		**ptr = '\n';
		(*ptr)++;
		break;
	    case 'r':
		**ptr = '\r';
		(*ptr)++;
		break;
	    case 't':
		**ptr = '\t';
		(*ptr)++;
		break;
	    case 'b':
		**ptr = '\b';
		(*ptr)++;
		break;
	    default:
		if ( !((chr == ctx->escapechar) || (chr == ctx->quotechar)) )
		{
		    **ptr = ctx->escapechar;
		    (*ptr)++;
		}
		**ptr = chr;
		(*ptr)++;
		break;
	}
    }
    else
    {
	if ( !((chr == ctx->escapechar) || (chr == ctx->quotechar)) )
	{
	    **ptr = ctx->escapechar;
	    (*ptr)++;
	}
	**ptr = chr;
	(*ptr)++;
    }
}



static void parse_fields(nx_logdata_t *logdata,
			 nx_csv_ctx_t *ctx,
			 char *dst,
			 const char *src,
			 size_t len)
{
    int pos = 0;
    nx_csv_state_t state;
    char *ptr;
    int currfield = 0;
    boolean unescape = FALSE;

    ptr = dst;
    state = NX_CSV_STATE_START;

    for ( pos = 0; pos < (int) len; pos++ )
    {
	//log_info("parse [%c]", src[pos]);

	if ( unescape == TRUE )
	{
	    unescape_string(ctx, src[pos], &ptr);
	    unescape = FALSE;
	    continue;
	}

	switch ( state )
	{
	    case NX_CSV_STATE_INSIDE_FIELD:
		if ( src[pos] == ctx->quotechar )
		{
		    state = NX_CSV_STATE_QUOTE;
		}
		else if ( src[pos] == ctx->escapechar )
		{
		    unescape = TRUE;
		}
		else
		{
		    *ptr = src[pos];
		    ptr++;
		}
		break;
	    case NX_CSV_STATE_QUOTE:
		if ( src[pos] == ctx->quotechar )
		{
		    state = NX_CSV_STATE_INSIDE_FIELD;
		    *ptr = src[pos];
		    ptr++;
		}
		else if ( src[pos] == ctx->delimiter )
		{
		    state = NX_CSV_STATE_START;
		    if ( currfield >= ctx->num_field )
		    {
			throw_msg("Too many fields in CSV input, expected %d, got %d in input '%s'",
				  ctx->num_field, currfield + 1, src);
		    }
		    *ptr = '\0';
		    add_logdata_field(ctx, logdata, ctx->fields[currfield],
				      dst, ctx->types[currfield]);
		    currfield++;
		    ptr = dst;
		}
		else if ( src[pos] == ctx->escapechar )
		{
		    unescape = TRUE;
		}
		else if ( src[pos] == ' ' )
		{
		    //skip space
		}
		else
		{
		    throw_msg("Invalid CSV input: '%s'", src);
		}
		break;
	    case NX_CSV_STATE_START:
		if ( src[pos] == ctx->escapechar )
		{
		    unescape = TRUE;
		    break;
		}
		if ( dst == ptr )
		{ // no data yet
		    if ( src[pos] == ' ' )
		    {
			//skip space
		    }
		    else if ( src[pos] == ctx->quotechar )
		    {
			state = NX_CSV_STATE_INSIDE_FIELD;
		    }
		    else if ( src[pos] == ctx->delimiter )
		    { // no value for field, not adding undef
			if ( currfield >= ctx->num_field )
			{
			    throw_msg("Too many fields in CSV input, expected %d, got %d in input '%s'",
				      ctx->num_field, currfield + 1, src);
			}
			currfield++;
		    }
		    else
		    {
			*ptr = src[pos];
			ptr++;
		    }
		}
		else
		{ // there is data in dst
		    if ( src[pos] == ctx->delimiter )
		    {
			if ( currfield >= ctx->num_field )
			{
			    throw_msg("Too many fields in CSV input, expected %d, got %d in input '%s'",
				      ctx->num_field, currfield + 1, src);
			}

			*ptr = '\0';
			add_logdata_field(ctx, logdata, ctx->fields[currfield],
					  dst, ctx->types[currfield]);
			currfield++;
			ptr = dst;
		    }
		    else
		    {
			*ptr = src[pos];
			ptr++;
		    }
		}
		break;
	    default:
		nx_panic("invalid state %d", state);
	}
    }

    switch ( state )
    {
	case NX_CSV_STATE_QUOTE:
	    if ( currfield >= ctx->num_field )
	    {
		throw_msg("Too many fields in CSV input, expected %d, got %d in input '%s'",
			  ctx->num_field, currfield + 1, src);
	    }
	    else
	    {
		*ptr = '\0';
		add_logdata_field(ctx, logdata, ctx->fields[currfield],
				  dst, ctx->types[currfield]);
		currfield++;
	    }
	    break;
	case NX_CSV_STATE_START:
	    if ( dst < ptr )
	    { // last data
		if ( currfield >= ctx->num_field )
		{
		    throw_msg("Too many fields in CSV input, expected %d, got %d in input '%s'",
			      ctx->num_field, currfield + 1, src);
		}
		else
		{
		    *ptr = '\0';
		    add_logdata_field(ctx, logdata, ctx->fields[currfield],
				      dst, ctx->types[currfield]);
		    currfield++;
		}
	    }
	    break;
	case NX_CSV_STATE_INSIDE_FIELD:
	    throw_msg("Invalid CSV input: '%s'", src);
	    break;
	default:
	    nx_panic("invalid state %d", state);
    }

    if ( currfield == ctx->num_field - 1 )
    { // assume last value is undef, don't add anything
    }
    else if ( currfield != ctx->num_field )
    {
	throw_msg("Not enough fields in CSV input, expected %d, got %d in input '%s'",
		  ctx->num_field, currfield, src);
    }
}



void nx_csv_parse(nx_logdata_t *logdata,
		  nx_csv_ctx_t *ctx,
		  const char *src,
		  size_t volatile len)
{
    nx_exception_t e;
    char *tmpstr = NULL;

    ASSERT(ctx != NULL);
    ASSERT(logdata != NULL);
    ASSERT(src != NULL);

    if ( len == 0 )
    {
	len = strlen(src);
    }

    if ( ctx->num_field == 0 )
    {
	throw_msg("Cannot parse CSV input, csv parser was not properly initialized (config error?)");
    }

    ASSERT(ctx->fields[0] != NULL);

    //TODO: use a static memory in ctx that is realloced as needed, see w3c.c ctx->buffer
    tmpstr = malloc((size_t)len + 1);

    try
    {
	parse_fields(logdata, ctx, tmpstr, src, len);
    }
    catch(e)
    {
	if ( tmpstr != NULL )
	{
	    free(tmpstr);
	}
	rethrow(e);
    }
    free(tmpstr);
}



/*
 * If ctx.escapechar is not zero, it is used to prefix the following characters:
 * - The ctx.escapechar character
 * - The ctx.quotechar if it is not zero
 * - The ctx.delimiter character
 * If ctx.escape_control is true, the characters \n,\r,\t,\b are escaped
 */


static void escape_string(nx_string_t *str, nx_csv_ctx_t *ctx)
{
    size_t i;

    if ( ctx->escapechar == '\0' )
    {
	return;
    }
    for ( i = 0; i < str->len; i++ )
    {
	if ( !(str->buf[i] & 0x80) )
	{
	    if ( (str->buf[i] == ctx->escapechar) ||
		 ((str->buf[i] == ctx->quotechar) && (ctx->quotechar != '\0')) ||
		 (str->buf[i] == ctx->delimiter) )
	    {
		if ( str->bufsize <= str->len + 1 )
		{
		    nx_string_ensure_size(str, str->len + 2);
		}
		memmove(str->buf + i + 1, str->buf + i, str->len - i);
		str->buf[i] = ctx->escapechar;
		(str->len)++;
		str->buf[str->len] = '\0';
		i++;
	    }
	    else if (ctx->escape_control == TRUE )
	    {
		if ( str->buf[i] == '\n' )
		{
		    if ( str->bufsize <= str->len + 1 )
		    {
			nx_string_ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		    str->buf[i] = ctx->escapechar;
		    str->buf[i + 1] = 'n';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		}
		else if ( str->buf[i] == '\r' )
		{
		    if ( str->bufsize <= str->len + 1 )
		    {
			nx_string_ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		    str->buf[i] = ctx->escapechar;
		    str->buf[i + 1] = 'r';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		}
		else if ( str->buf[i] == '\t' )
		{
		    if ( str->bufsize <= str->len + 1 )
		    {
			nx_string_ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		    str->buf[i] = ctx->escapechar;
		    str->buf[i + 1] = 't';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		}
		else if ( str->buf[i] == '\b' )
		{
		    if ( str->bufsize <= str->len + 1 )
		    {
			nx_string_ensure_size(str, str->len + 2);
		    }
		    memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		    str->buf[i] = ctx->escapechar;
		    str->buf[i + 1] = 'b';
		    (str->len)++;
		    str->buf[str->len] = '\0';
		    i++;
		}
	    }
	}
    }
}



nx_string_t *nx_logdata_to_csv(nx_csv_ctx_t *ctx, nx_logdata_t *logdata)
{
    nx_string_t *retval = NULL;
    char *string = NULL;
    nx_string_t *tmp = NULL;
    nx_value_t value;
    nx_exception_t e;
    int currfield;

    ASSERT(ctx != NULL);
    ASSERT(logdata != NULL);
    ASSERT(ctx->num_field > 0);
    ASSERT(ctx->fields[0] != NULL);

    retval = nx_string_new_size(150);

    try
    {
	for ( currfield = 0; currfield < ctx->num_field; currfield++ )
	{
	    if ( currfield > 0 )
	    { // add delimiter
		nx_string_append(retval, &(ctx->delimiter), 1);
	    }
	    if ( nx_logdata_get_field_value(logdata, ctx->fields[currfield],
					    &value) != TRUE )
	    {
		// value not found, don't write anything
		continue;
	    }
	    if ( value.defined == FALSE )
	    { // don't write undef
		continue;
	    }

	    if ( value.type == NX_VALUE_TYPE_STRING )
	    {
		if ( value.string->len > 0 )
		{
		    // opening-quote escaped-string closing-quote
		    if ( ctx->quote_method != NX_CSV_QUOTE_METHOD_NONE )
		    {
			nx_string_append(retval, &(ctx->quotechar), 1);
		    }
		    tmp = nx_string_clone(value.string);
		    escape_string(tmp, ctx);
		    nx_string_append(retval, tmp->buf, (int) tmp->len);
		    nx_string_free(tmp);
		    if ( ctx->quote_method != NX_CSV_QUOTE_METHOD_NONE )
		    {
			nx_string_append(retval, &(ctx->quotechar), 1);
		    }
		}
	    }
	    else
	    {
		string = nx_value_to_string(&value);
		if ( string != NULL )
		{
		    if ( ctx->quote_method == NX_CSV_QUOTE_METHOD_ALL )
		    { // quote everything
			nx_string_append(retval, &(ctx->quotechar), 1);
			tmp = nx_string_create(string, -1);
			escape_string(tmp, ctx);
			nx_string_append(retval, tmp->buf, (int) tmp->len);
			nx_string_free(tmp);
			nx_string_append(retval, &(ctx->quotechar), 1);
		    }
		    else
		    {
			tmp = nx_string_create(string, -1);
			escape_string(tmp, ctx);
			nx_string_append(retval, tmp->buf, (int) tmp->len);
			nx_string_free(tmp);
		    }
		    free(string);
		}
	    }
	}
    }
    catch(e)
    {
	if ( string != NULL )
	{
	    free(string);
	}
	if ( tmp != NULL )
	{
	    nx_string_free(tmp);
	}
	if ( retval != NULL )
	{
	    nx_string_free(retval);
	}
	rethrow(e);
    }

    return ( retval );
}
