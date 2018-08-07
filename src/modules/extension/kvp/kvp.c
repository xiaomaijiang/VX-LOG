/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/error_debug.h"
#include "../../../common/exception.h"
#include "kvp.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

typedef enum nx_kvp_state_t
{
    NX_KVP_STATE_KEY_START = 1,
    NX_KVP_STATE_KEY, ///< inside key
    NX_KVP_STATE_KEY_ESCAPE, ///< escape sequence inside key
    NX_KVP_STATE_KV_DELIMITER,
    NX_KVP_STATE_VALUE_START, ///< before value
    NX_KVP_STATE_VALUE, ///< inside value
    NX_KVP_STATE_VALUE_ESCAPE, ///< escape sequence inside value
    NX_KVP_STATE_KVP_DELIMITER,
} nx_kvp_state_t;



void nx_kvp_ctx_init(nx_kvp_ctx_t *ctx)
{
    ASSERT(ctx != NULL);

    ctx->keyquotechar = '\0';
    ctx->valquotechar = '\0';
    ctx->kvdelimiter = '\0';
    ctx->kvpdelimiter = '\0';
    ctx->escapechar = '\0';
//    ctx->quote_method = NX_KVP_QUOTE_METHOD_AUTO;
}



char nx_kvp_get_config_char(const char *str)
{
    char retval = '\0';
    size_t len;

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
	    break;
    }

    return ( retval );
}



static void add_logdata_field(nx_logdata_t *logdata, 
			      char *key,
			      int keylen,
			      boolean keyquoted,
			      nx_string_t *strval)
{
    nx_value_t *value;

    ASSERT(key != NULL);
    ASSERT(strval != NULL);

    if ( keyquoted == FALSE )
    { // trim trailing space from the key if it was unquoted
	while ( (keylen > 0) && (key[keylen - 1] == ' ') )
	{
	    keylen--;
	}
    }
    
    key[keylen] = '\0';
//    log_info("add kvp: %s=%s", key, strval->buf);
    value = nx_value_new(NX_VALUE_TYPE_STRING);
    value->string = strval;
    nx_logdata_set_field_value(logdata, key, value);
}



static void unescape_value(nx_kvp_ctx_t *ctx,
			   const char chr,
			   nx_string_t *dst,
			   boolean quoted)
{
    char tmp[2];

    ASSERT(dst != NULL);

    if ( ctx->escape_control == TRUE )
    {
	switch ( chr )
	{
	    case 'n':
		nx_string_append(dst, "\n", 1);
		return;
	    case 'r':
		nx_string_append(dst, "\r", 1);
		return;
	    case 't':
		nx_string_append(dst, "\t", 1);
		return;
	    case 'b':
		nx_string_append(dst, "\b", 1);
		return;
	    default:
		break;
	}
    }

    if ( chr == ctx->escapechar )
    {
	nx_string_append(dst, &ctx->escapechar, 1);
	return;
    }

    if ( chr == ctx->valquotechar )
    {
	nx_string_append(dst, &ctx->valquotechar, 1);
	return;
    }

    if ( (quoted == FALSE) && (chr == ctx->kvpdelimiter) )
    { // only unescape the kvpdelimiter in an unquoted string
	nx_string_append(dst, &ctx->kvpdelimiter, 1);
	return;
    }

    tmp[0] = ctx->escapechar;
    tmp[1] = chr;
    nx_string_append(dst, tmp, 2);
}



#define IS_QUOTECHAR(c) ( (c == '\'') || (c == '\"') )
#define IS_KVDELIMITERCHAR(c) ( (c == ':') || (c == '=') )
#define IS_KVPDELIMITERCHAR(c) ( (c == ',') || (c == ';') || (c == ' ') )

static void parse_kvp(nx_logdata_t *logdata,
		      nx_kvp_ctx_t *ctx,
		      const char *src,
		      size_t len)
{
    nx_string_t *valuestr = NULL;
    nx_exception_t e;
    nx_kvp_state_t state = NX_KVP_STATE_KEY_START;
    char keyname[256];
    int keylen = 0;
    volatile boolean got_keyquote = FALSE;

    try
    {
	int pos = 0;
	boolean got_valquote = FALSE;

	for ( pos = 0; pos < (int) len; pos++ )
	{
//	    log_info("parse [%c] state: %d", src[pos], state);
	    
	    switch ( state )
	    {
		case NX_KVP_STATE_KEY:
		    if ( src[pos] == ctx->keyquotechar )
		    {
			if ( got_keyquote == TRUE )
			{
			    state = NX_KVP_STATE_KV_DELIMITER;
			}
			else
			{
			    throw_msg("invalid key quotation in key-value pair");
			}
		    }
		    else if ( (ctx->kvdelimiter == '\0') &&
			      IS_KVDELIMITERCHAR(src[pos]) )
		    {
			ctx->kvdelimiter = src[pos];
			state = NX_KVP_STATE_VALUE_START;
		    }
		    else if ( src[pos] == ctx->kvdelimiter )
		    {
			state = NX_KVP_STATE_VALUE_START;
		    }
		    else if ( src[pos] == ctx->escapechar )
		    {
			state = NX_KVP_STATE_KEY_ESCAPE;
		    }
/* Unquoted keys containing space don't work with this, so instead we trim the space at the end
		    else if ( (src[pos] == ' ') && (got_keyquote != TRUE) )
		    {
			state = NX_KVP_STATE_KV_DELIMITER;
		    }
*/
		    else
		    {
			if ( keylen < (int) sizeof(keyname) - 1 )
			{
			    keyname[keylen] = src[pos];
			    keylen++;
			}
		    }
		    break;

		case NX_KVP_STATE_VALUE_START:
		    if ( src[pos] == ' ' )
		    { // skip leading space
			break;
		    }
		    got_valquote = FALSE;
		    ASSERT(valuestr == NULL);
		    valuestr = nx_string_new();

		    if ( (ctx->valquotechar == '\0') && 
			 IS_QUOTECHAR(src[pos]) )
		    { // auto-detected quotechar
			ctx->valquotechar = src[pos];
			got_valquote = TRUE;
		    }		    
		    else if ( src[pos] == ctx->valquotechar )
		    {
			got_valquote = TRUE;
		    }
		    else
		    {
			pos--; // handle character in STATE_VALUE
		    }
		    state = NX_KVP_STATE_VALUE;
		    break;

		case NX_KVP_STATE_VALUE:
		    if ( src[pos] == ctx->valquotechar )
		    {
			state = NX_KVP_STATE_KVP_DELIMITER;
			if ( got_valquote == TRUE )
			{
			}
			else
			{
			    if ( valuestr->len > 0)
			    {
				throw_msg("invalid value quotation in key-value pair: %s", valuestr->buf);
			    }
			}
		    }
		    else if ( (ctx->kvpdelimiter == '\0') &&
			      IS_KVPDELIMITERCHAR(src[pos]) &&
			      (got_valquote == FALSE) )
		    {
			ctx->kvpdelimiter = src[pos];
			// add field-value
			add_logdata_field(logdata, keyname, keylen, got_keyquote, valuestr);
			valuestr = NULL;
			state = NX_KVP_STATE_KEY_START;
		    }
		    else if ( (src[pos] == ctx->kvpdelimiter) && (got_valquote == FALSE) )
		    {
			// add field-value
			add_logdata_field(logdata, keyname, keylen, got_keyquote, valuestr);
			valuestr = NULL;
			state = NX_KVP_STATE_KEY_START;
		    }
		    else if ( src[pos] == ctx->escapechar )
		    {
			state = NX_KVP_STATE_VALUE_ESCAPE;
		    }
		    else
		    { // append character to value
			nx_string_append(valuestr, src + pos, 1);
		    }
		    break;

		case NX_KVP_STATE_VALUE_ESCAPE:
		    unescape_value(ctx, src[pos], valuestr, got_valquote);
		    state = NX_KVP_STATE_VALUE;
		    break;

		case NX_KVP_STATE_KV_DELIMITER:
		    if ( src[pos] == ' ' ) 
		    { // skip space
			break;
		    }

		    if ( (ctx->kvdelimiter == '\0') &&
			 IS_KVDELIMITERCHAR(src[pos]) )
		    {
			ctx->kvdelimiter = src[pos];
			state = NX_KVP_STATE_VALUE_START;
		    }
		    else if ( src[pos] == ctx->kvdelimiter )
		    {
			state = NX_KVP_STATE_VALUE_START;
		    }
		    break;
		    
		case NX_KVP_STATE_KVP_DELIMITER:
		    if ( (ctx->kvpdelimiter == '\0') &&
			 IS_KVPDELIMITERCHAR(src[pos]) )
		    {
			ctx->kvpdelimiter = src[pos];
			state = NX_KVP_STATE_KEY_START;
			add_logdata_field(logdata, keyname, keylen, got_keyquote, valuestr);
			valuestr = NULL;
		    }
		    else if ( src[pos] == ctx->kvpdelimiter )
		    {
			state = NX_KVP_STATE_KEY_START;
			add_logdata_field(logdata, keyname, keylen, got_keyquote, valuestr);
			valuestr = NULL;
		    }
		    break;
		    
		case NX_KVP_STATE_KEY_START:
		    keylen = 0;
		    got_keyquote = FALSE;
		    if ( src[pos] == ' ' )
		    { // skip space
			break;
		    }
		    else if ( (ctx->keyquotechar == '\0') && 
			      IS_QUOTECHAR(src[pos]) )
		    { // auto-detected quotechar
			ctx->keyquotechar = src[pos];
			got_keyquote = TRUE;
			state = NX_KVP_STATE_KEY;
		    }
		    else if ( src[pos] == ctx->keyquotechar )
		    {
			got_keyquote = TRUE;
			state = NX_KVP_STATE_KEY;
		    }
		    else if ( src[pos] == ctx->escapechar )
		    {
			state = NX_KVP_STATE_KEY_ESCAPE;
		    }
		    else if ( src[pos] == ctx->kvpdelimiter )
		    { // double delimiter, no value for kvp, skip
		    }
		    else
		    { // first character of unquoted key
			state = NX_KVP_STATE_KEY;
			if ( keylen < (int) sizeof(keyname) - 1 )
			{
			    keyname[keylen] = src[pos];
			    keylen++;
			}
		    }
		    break;
		case NX_KVP_STATE_KEY_ESCAPE:
		    if ( (src[pos] == ctx->escapechar) ||
			 (src[pos] == ctx->keyquotechar) )
		    {
			if ( keylen < (int) sizeof(keyname) - 1 )
			{
			    keyname[keylen] = src[pos];
			    keylen++;
			}
		    }
		    else
		    {
			if ( keylen < (int) sizeof(keyname) - 2 )
			{
			    keyname[keylen] = ctx->escapechar;
			    keylen++;

			    keyname[keylen] = src[pos];
			    keylen++;
			}
		    }
		    state = NX_KVP_STATE_KEY;
		    break;
		default:
		    nx_panic("invalid state %d", state);
	    }
	}
    }
    catch(e)
    {
	if ( valuestr != NULL )
	{
	    nx_string_free(valuestr);
	}
	rethrow(e);
    }

    switch ( state )
    {
	case NX_KVP_STATE_VALUE:
	case NX_KVP_STATE_KVP_DELIMITER:
	    add_logdata_field(logdata, keyname, keylen, got_keyquote, valuestr);
	    break;
	case NX_KVP_STATE_KEY_START:
	    ASSERT(valuestr == NULL);
	    break;
	case NX_KVP_STATE_KV_DELIMITER:
	case NX_KVP_STATE_KEY:
	case NX_KVP_STATE_KEY_ESCAPE:
	case NX_KVP_STATE_VALUE_START:
	case NX_KVP_STATE_VALUE_ESCAPE:
	    if ( valuestr != NULL )
	    {
		nx_string_free(valuestr);
	    }
	    throw_msg("invalid KVP input: '%s' [state: %d]", src, state);
	    break;
	default:
	    nx_panic("invalid state %d", state);
    }
}



void nx_kvp_parse(nx_logdata_t *logdata,
		  nx_kvp_ctx_t *ctx,
		  const char *src,
		  size_t volatile len)
{
    ASSERT(ctx != NULL);
    ASSERT(logdata != NULL);
    ASSERT(src != NULL);

    if ( len == 0 )
    {
	len = strlen(src);
    }

    parse_kvp(logdata, ctx, src, len);
}



/*
 * If ctx.escapechar is not zero, it is used to prefix the following characters:
 * - The ctx.escapechar character
 * - The ctx.quotechar if it is not zero
 * - The ctx.kvdelimiter character
 * - The ctx.kvpdelimiter character
 * If ctx.escape_control is true, the characters \n,\r,\t,\b are escaped
 */


static void escape_value(nx_string_t *str, nx_kvp_ctx_t *ctx)
{
    size_t i;
    char quotechar = '\'';
    char escapechar = '\\';
    boolean needquote = FALSE;

    if ( ctx->valquotechar != '\0' )
    {
	quotechar = ctx->valquotechar;
    }
    if ( ctx->escapechar != '\0' )
    {
	escapechar = ctx->escapechar;
    }

    for ( i = 0; i < str->len; i++ )
    {
	if ( !(str->buf[i] & 0x80) )
	{
	    if ( str->buf[i] == escapechar )
	    {
		if ( str->bufsize <= str->len + 1 )
		{
		    nx_string_ensure_size(str, str->len + 2);
		}
		memmove(str->buf + i + 1, str->buf + i, str->len - i);
		str->buf[i] = escapechar;
		(str->len)++;
		str->buf[str->len] = '\0';
		i++;
	    }
	    else if ( str->buf[i] == quotechar )
	    {
		if ( str->bufsize <= str->len + 1 )
		{
		    nx_string_ensure_size(str, str->len + 2);
		}
		memmove(str->buf + i + 2, str->buf + i + 1, str->len - i - 1);
		str->buf[i] = escapechar;
		str->buf[i + 1] = quotechar;
		(str->len)++;
		str->buf[str->len] = '\0';
		i++;
	    }
	    else if ( str->buf[i] == ' ' )
	    {
		needquote = TRUE;
	    }
	    else if ( (str->buf[i] == ctx->kvpdelimiter) )
	    {
		needquote = TRUE;
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
    if ( needquote == TRUE )
    {
	nx_string_prepend(str, &quotechar, 1);
	nx_string_append(str, &quotechar, 1);
    }
}



nx_string_t *nx_logdata_to_kvp(nx_kvp_ctx_t *ctx, nx_logdata_t *logdata)
{
    nx_string_t *retval = NULL;
    char *string = NULL;
    nx_string_t *tmp = NULL;
    nx_exception_t e;
    nx_logdata_field_t *field = NULL;
    char kvpdelimiter = ';';
    char kvdelimiter = '=';

    ASSERT(ctx != NULL);
    ASSERT(logdata != NULL);

    if ( ctx->kvdelimiter != '\0' )
    {
	kvdelimiter = ctx->kvdelimiter;
    }
    if ( ctx->kvpdelimiter != '\0' )
    {
	kvpdelimiter = ctx->kvpdelimiter;
    }

    retval = nx_string_new_size(150);

    try
    {
	for ( field = NX_DLIST_FIRST(&(logdata->fields));
	      field != NULL;
	      field = NX_DLIST_NEXT(field, link) )
	{
	    if ( strcmp(field->key, "raw_event") == 0 )
	    { // don't write raw event
		continue;
	    }

	    if ( (field->key[0] == '.') || (field->key[0] == '_') )
	    {
		continue;
	    }

	    nx_string_append(retval, field->key, -1);
	    nx_string_append(retval, &kvdelimiter, 1);

	    ASSERT(field->value != NULL);
	    if ( field->value->defined == FALSE )
	    { // don't write undef
	    }
	    else
	    {
		if ( field->value->type == NX_VALUE_TYPE_STRING )
		{
		    if ( field->value->string->len > 0 )
		    {
			tmp = nx_string_clone(field->value->string);
			escape_value(tmp, ctx);
			nx_string_append(retval, tmp->buf, (int) tmp->len);
			nx_string_free(tmp);
		    }
		}
		else
		{
		    string = nx_value_to_string(field->value);
		    if ( string != NULL )
		    {
			nx_string_append(retval, string, -1);
			free(string);
			string = NULL;
		    }
		}
	    }
	    nx_string_append(retval, &kvpdelimiter, 1);
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
