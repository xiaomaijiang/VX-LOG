/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "module.h"
#include "serialize.h"
#include "exception.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

nx_logdata_t *nx_module_input_func_linereader(nx_module_input_t *input,
					      void *data UNUSED)
{
    int i;
    nx_logdata_t *retval = NULL;
    int len;
    nx_logdata_t *incomplete_logdata;

    ASSERT(input != NULL);
    ASSERT(input->buflen >= 0);

    if ( input->buflen == 0 )
    {
	return ( NULL );
    }
    incomplete_logdata = input->ctx;
    if ( incomplete_logdata == NULL )
    {
	if ( (input->buf[input->bufstart + 0] == NX_LOGDATA_BINARY_HEADER[0]) &&
	     (input->buflen >= 4) &&
	     (input->buf[input->bufstart + 1] == NX_LOGDATA_BINARY_HEADER[1]) && 
	     (input->buf[input->bufstart + 2] == NX_LOGDATA_BINARY_HEADER[2]) &&
	     (input->buf[input->bufstart + 3] == NX_LOGDATA_BINARY_HEADER[3]) ) //possible NX_LOGDATA_BINARY_HEADER
	{
	    return ( nx_module_input_func_binaryreader(input, NULL) );
	}
	else
	{ // treat as normal textbased line
	    boolean foundlf = 0;
	    for ( i = 0; i < input->buflen; i++ )
	    {
		if ( input->buf[input->bufstart + i] == APR_ASCII_LF )
		{
		    foundlf = TRUE;
		    break;
		}
	    }
	    
	    len = i;

	    if ( foundlf == TRUE )
	    {   
		if ( (len < input->buflen) && (input->buf[input->bufstart + len + 1] == '\0') )
		{ // this is a hack against utf-16le double byte linebreak 0x0D00 0x0A00
		    i++;
		}

		// Check for trailing CR and nuke that by reducing length
		if ( (len > 0) && (input->buf[input->bufstart + len - 1] == APR_ASCII_CR) )
		{
		    len--;
		}
		else if ( (len > 1) && (input->buf[input->bufstart + len - 1] == '\0')
			  && (input->buf[input->bufstart + len - 2] == APR_ASCII_CR) )
		{ // this is a hack against utf-16le double byte linebreak 0x0D00 0x0A00
		    len -= 2;
		}
		if ( i < input->buflen )
		{
		    i++; // Start past the LF
		}
		retval = nx_logdata_new_logline(input->buf + input->bufstart, len);
		ASSERT(i <= input->buflen);
		input->buflen -= i;
	    }
	    else
	    { // not found, partial read
		incomplete_logdata = nx_logdata_new_logline(input->buf + input->bufstart, len);
		input->ctx = (void *) incomplete_logdata;
		ASSERT(len <= input->buflen);
		input->buflen -= len;
	    }
	    ASSERT(input->bufstart + i <= input->bufsize);
	    input->bufstart += i;
	}
    }
    else
    { // we have a partial read in incomplete_logdata
	boolean foundlf = FALSE;
	
	log_debug("appending to incomplete_logdata");
	for ( i = 0; i < input->buflen; i++ )
	{
	    if ( input->buf[input->bufstart + i] == APR_ASCII_LF )
	    {
		foundlf = TRUE;
		break;
	    }
	}
	
	len = i;

	if ( foundlf == TRUE )
	{
	    retval = incomplete_logdata;
	    input->ctx = NULL;

	    if ( (len < input->buflen) && (input->buf[input->bufstart + len + 1] == '\0') )
	    { // this is a hack against utf-16le double byte linebreak 0x0D00 0x0A00
		i++;
	    }

	    // Check for trailing CR and nuke that by reducing length
	    if ( (len > 0) && (input->buf[input->bufstart + len - 1] == APR_ASCII_CR) )
	    {
		len--;
	    }
	    else if ( (len > 1) && (input->buf[input->bufstart + len - 1] == '\0')
		      && (input->buf[input->bufstart + len - 2] == APR_ASCII_CR) )
	    { // this is a hack against utf-16le double byte linebreak 0x0D00 0x0A00
		len -= 2;
	    }
	    if ( i < input->buflen )
	    {
		i++; // Start past the LF
	    }

	    if ( len == 0 )
	    {   // Only LF in input buffer which was nuked, so we need to check for a trailing CR in incomplete_logdata
		// This can happen if CRLF falls on the buffer boundary.
		if ( (incomplete_logdata->raw_event->len > 0) &&
		     (incomplete_logdata->raw_event->buf[incomplete_logdata->raw_event->len - 1] ==  APR_ASCII_CR) )
		{
		    (incomplete_logdata->raw_event->len)--;
		}
	    }
	    else
	    {
		nx_logdata_append_logline(incomplete_logdata, input->buf + input->bufstart, len);
	    }
	    ASSERT(i <= input->buflen);
	    input->buflen -= i;
	}
	else
	{ // LF not found, append to buffer
	    nx_logdata_append_logline(incomplete_logdata, input->buf + input->bufstart, len);
	    ASSERT(i <= input->buflen);
	    input->buflen -= i;
	}
	ASSERT(input->bufstart + i <= input->bufsize);
	input->bufstart += i;
    }

    return ( retval );
}



nx_logdata_t *nx_module_input_func_dgramreader(nx_module_input_t *input,
					       void *data UNUSED)
{
    nx_logdata_t *retval = NULL;

    ASSERT(input != NULL);
    ASSERT(input->buflen >= 0);
    ASSERT(input->ctx == NULL);
    ASSERT(input->bufstart == 0);

    if ( input->buflen == 0 )
    {
	return ( NULL );
    }

    if ( input->buf[input->bufstart] < 10 )
    {
	retval = nx_module_input_func_binaryreader(input, NULL);
	input->buflen = 0;
	input->bufstart = 0;
	return ( retval );
    }
    if ( input->bufsize > input->buflen )
    {
	input->buf[input->buflen] = '\0';
	retval = nx_logdata_new_logline(input->buf, (int) strlen(input->buf));
    }
    else
    {
	retval = nx_logdata_new_logline(input->buf, input->buflen);
    }
    input->buflen = 0;
    input->bufstart = 0;

    return ( retval );
}



nx_logdata_t *nx_module_input_func_binaryreader(nx_module_input_t *input,
						void *data UNUSED)
{
    nx_logdata_t *retval = NULL;
    apr_size_t len;
    apr_uint32_t datalen;
    nx_exception_t e;

    ASSERT(input != NULL);
    ASSERT(input->buflen >= 0);
    ASSERT(input->ctx == NULL);

    if ( input->buflen < 8 )
    {
	if ( input->bufstart + input->buflen == input->bufsize )
	{
	    memmove(input->buf, input->buf + input->bufstart, (size_t) input->buflen);
	    input->bufstart = 0;
	}
	return ( NULL );
    }

    if ( memcmp(input->buf + input->bufstart, NX_LOGDATA_BINARY_HEADER, 4) != 0 )
    {
	log_error("binary header not found at position %d in data received from %s, is input really binary?",
		  input->bufstart, nx_module_input_name_get(input));
	for ( ; input->buflen > 0; (input->buflen)--, (input->bufstart)++ )
	{
	    if ( (input->buflen >= 4) &&
		 (memcmp(input->buf + input->bufstart, NX_LOGDATA_BINARY_HEADER, 4) == 0) )
	    {
		break;
	    }
	}

	if ( input->buflen == 0 )
	{
	    return ( NULL );
	}
    }

    datalen = nx_int32_from_le(input->buf + input->bufstart + 4);
    //log_info("datalen: %d", datalen);

    if ( datalen + 8 > (apr_uint32_t) input->bufsize )
    {
	log_error("binary logdata is larger (%d) than BufferSize (%d), discarding", datalen, input->bufsize);
	input->bufstart = 0;
	input->buflen = 0;
	return ( NULL );
    }

    if ( datalen + 8 <= (apr_uint32_t) input->buflen )
    {
	//log_info("logdata (size: %d) all in buffer, reading from %d", (int) datalen + 8, input->bufstart);
	input->bufstart += 8;
	input->buflen -= 8;

	try
	{
	    retval = nx_logdata_from_membuf(input->buf + input->bufstart,
					    (apr_size_t) input->buflen, &len);
	    input->bufstart += (int) len;
	    ASSERT(len <= (apr_size_t) input->buflen);
	    input->buflen -= (int) len;
	}
	catch(e)
	{
	    log_exception_msg(e, "failed to read binary logdata, discarding");
	    for ( ; input->buflen > 0; (input->buflen)--, (input->bufstart)++ )
	    {
		if ( (input->buflen >= 4) &&
		     (memcmp(input->buf + input->bufstart, NX_LOGDATA_BINARY_HEADER, 4) == 0) )
		{
		    break;
		}
	    }
	}
    }
    else
    {
	log_debug("binary logdata is larger (%d) than buffer (%d)", datalen + 8, input->buflen);
	if ( input->bufstart > 0 )
	{   // possible partial buffer
	    memmove(input->buf, input->buf + input->bufstart, (size_t) input->buflen);
	    input->bufstart = 0;
	}
    }
    return ( retval );
}
