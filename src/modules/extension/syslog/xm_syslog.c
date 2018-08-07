/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "../../../common/module.h"
#include "../../../common/error_debug.h"
#include "xm_syslog.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

extern nx_module_exports_t nx_module_exports_xm_syslog;


static nx_logdata_t *xm_syslog_input_func_rfc5425(nx_module_input_t *input,
						  void *data UNUSED)
{
    int i;
    nx_logdata_t *retval = NULL;
    int msglen = 0;
    int msglenlen = 0;
    boolean got_space = FALSE;
    nx_string_t *incomplete_logdata = NULL;
    int appendlen;

    ASSERT(input != NULL);
    ASSERT(input->buflen >= 0);

    if ( input->ctx != NULL )
    { // we have incomplete data from a previous read
	incomplete_logdata = (nx_string_t *) input->ctx;

	// here we need to avoid buffering too much data into incomplete_logdata, otherwise it could exceed the string limit
	appendlen = input->buflen;
	if ( incomplete_logdata->len + (uint32_t) appendlen >= nx_string_get_limit() )
	{
	    appendlen = (int) nx_string_get_limit() - (int) incomplete_logdata->len - 1;
	}
	nx_string_append(incomplete_logdata, input->buf + input->bufstart, appendlen);
	if ( appendlen == input->buflen )
	{
	    input->bufstart = 0;
	    input->buflen = 0;
	}
	else
	{
	    input->bufstart += appendlen;
	    input->buflen -= appendlen;
	}

	for ( i = 0; i < (int) incomplete_logdata->len; i++ )
	{
	    msglenlen++;
	    if ( incomplete_logdata->buf[i] == ' ' )
	    {
		got_space = TRUE;
		break;
	    }
	    if ( ! apr_isdigit(incomplete_logdata->buf[i]) )
	    {
		throw_msg("invalid header received by Syslog_TLS input reader, input is not RFC 5425 compliant: [%s]", incomplete_logdata->buf);
	    }
	    msglen *= 10;
	    msglen += incomplete_logdata->buf[i] - '0';
	}
	if ( got_space != TRUE )
	{
	    return ( NULL );
	}

	// we are past MSG-LEN and know how many bytes we need
	if ( msglen == 0 )
	{
	    throw_msg("invalid 0 MSG-LEN received by Syslog_TLS input reader");
	}
	ASSERT(msglen >= 0);
	
	if ( msglen + msglenlen > (int) incomplete_logdata->len )
	{ // got less than indicated, need to wait for more input data
	    return ( NULL );
	}

	retval = nx_logdata_new_logline(incomplete_logdata->buf + msglenlen, msglen);

	msglen += msglenlen;
	if ( (int) incomplete_logdata->len > msglen )
	{
	    memmove(incomplete_logdata->buf, incomplete_logdata->buf + msglen, 
		    incomplete_logdata->len - (uint32_t) msglen);
	    incomplete_logdata->len -= (uint32_t) msglen;
	}
	else //if ( incomplete_logdata->len == msglen )
	{
	    nx_string_free(incomplete_logdata);
	    input->ctx = NULL;
	}
    }
    else
    {
	if ( input->buflen == 0 )
	{
	    return ( NULL );
	}

	for ( i = 0; i < input->buflen; i++ )
	{
	    msglenlen++;
	    if ( input->buf[input->bufstart + i] == ' ' )
	    {
		got_space = TRUE;
		ASSERT(input->buflen >= 0);
		break;
	    }
	    if ( ! apr_isdigit(input->buf[input->bufstart + i]) )
	    {
		throw_msg("invalid header received by Syslog_TLS input reader, input is not RFC 5425 compliant.");
	    }
	    msglen *= 10;
	    msglen += input->buf[input->bufstart + i] - '0';
	}

	if ( got_space != TRUE )
	{
	    ASSERT(input->buflen > 0);

	    input->ctx = (void * ) nx_string_create(input->buf + input->bufstart, input->buflen);
	    input->bufstart = 0;
	    input->buflen = 0;
	    return ( NULL );
	}

	// we are past MSG-LEN and know how many bytes we need
	if ( msglen == 0 )
	{
	    throw_msg("invalid 0 MSG-LEN received by Syslog_TLS input reader");
	}
	ASSERT(msglen >= 0);
	
	if ( msglen + msglenlen > input->buflen )
	{ // got less than indicated, need to wait for more input data
	    input->ctx = (void * ) nx_string_create(input->buf + input->bufstart, input->buflen);
	    input->bufstart = 0;
	    input->buflen = 0;
	    return ( NULL );
	}

	retval = nx_logdata_new_logline(input->buf + input->bufstart + msglenlen, msglen);

	msglen += msglenlen;
	input->bufstart += msglen;
	input->buflen -= msglen;

	if ( input->buflen > 0 )
	{ // got more than one event, store the remaining data
	    input->ctx = (void * ) nx_string_create(input->buf + input->bufstart, input->buflen);
	    input->bufstart = 0;
	    input->buflen = 0;
	}
    }

    // strip tailing newline
    while ( (retval->raw_event->len > 0) && 
	    ((retval->raw_event->buf[retval->raw_event->len - 1] == APR_ASCII_CR) ||
	     (retval->raw_event->buf[retval->raw_event->len - 1] == APR_ASCII_LF)) )
    {
	(retval->raw_event->len)--;
    }

    return ( retval );
}



static void xm_syslog_output_func_rfc5425(nx_module_output_t *output,
					  void *data UNUSED)
{
    int len;

    ASSERT(output != NULL);
    ASSERT(output->buf != NULL);
    ASSERT(output->buflen == 0);
    ASSERT(output->logdata != NULL);
    ASSERT(output->logdata->raw_event != NULL);

    //len = logdata->raw_event->len;
    len = apr_snprintf(output->buf, (apr_size_t) output->bufsize, "%u ",
		       (unsigned int) output->logdata->raw_event->len);

    if ( (apr_size_t) len + (apr_size_t) output->logdata->raw_event->len > output->bufsize )
    {
	log_error("Syslog_TLS output is over the limit of %d, will be truncated",
		  (int) output->bufsize);
	len = apr_snprintf(output->buf, (apr_size_t) output->bufsize, "%u ",
			   (unsigned int) output->bufsize - 10);
	memcpy(output->buf + len, output->logdata->raw_event->buf, output->bufsize - 10);
	output->buflen = (apr_size_t) len + output->bufsize - 10;
    }
    else
    {
	memcpy(output->buf + len, output->logdata->raw_event->buf,
	       output->logdata->raw_event->len);
	output->buflen = (apr_size_t) len + output->logdata->raw_event->len;
    }
    output->bufstart = 0;
}



static char _get_config_char(const char *str)
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



static void xm_syslog_config(nx_module_t *module UNUSED)
{
    const char *fname = "Syslog_TLS";
    nx_xm_syslog_conf_t *modconf;
    const nx_directive_t *curr;
    char tmpchar;

    modconf = apr_pcalloc(module->pool, sizeof(nx_xm_syslog_conf_t));
    module->config = modconf;

    curr = module->directives;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "SnareDelimiter") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "SnareDelimiter needs a parameter");
	    }
	    if ( modconf->snaredelimiter != '\0' )
	    {
		nx_conf_error(curr, "SnareDelimiter is already defined");
	    }
	    tmpchar = _get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid SnareDelimiter parameter: %s", curr->args);
	    }
	    modconf->snaredelimiter = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "SnareReplacement") == 0 )
	{
	    if ( (curr->args == NULL) || (curr->args[0] == '\0') )
	    {
		nx_conf_error(curr, "SnareReplacement needs a parameter");
	    }
	    if ( modconf->snarereplacement != '\0' )
	    {
		nx_conf_error(curr, "SnareReplacement is already defined");
	    }
	    tmpchar = _get_config_char(curr->args);
	    if ( tmpchar == '\0' )
	    {
		nx_conf_error(curr, "invalid SnareReplacement parameter: %s", curr->args);
	    }
	    modconf->snarereplacement = tmpchar;
	}
	else if ( strcasecmp(curr->directive, "IETFTimestampInGMT") == 0 )
	{
	    nx_cfg_get_boolean(curr, "IETFTimestampInGMT", &(modconf->ietftimestampingmt));
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( modconf->snaredelimiter == '\0' )
    {
	modconf->snaredelimiter = '\t';
    }

    if ( modconf->snarereplacement == '\0' )
    {
	modconf->snarereplacement = ' ';
    }

    if ( nx_module_input_func_lookup(fname) == NULL )
    {
	nx_module_input_func_register(NULL, fname, &xm_syslog_input_func_rfc5425, NULL, NULL);
	log_debug("Inputreader '%s' registered", fname);
    }

    if ( nx_module_output_func_lookup(fname) == NULL )
    {
	nx_module_output_func_register(NULL, fname, &xm_syslog_output_func_rfc5425, NULL);
	log_debug("Outputwriter '%s' registered", fname);
    }
}



NX_MODULE_DECLARATION nx_xm_syslog_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_syslog_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    NULL,			// event
    NULL,			// info
    &nx_module_exports_xm_syslog, //exports
};
