/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "module.h"
#include "serialize.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static void nx_module_output_fill_buffer(nx_module_output_t *output,
					 boolean addlf)
{
    apr_size_t len;

    ASSERT(output != NULL);
    ASSERT(output->buf != NULL);
    ASSERT(output->buflen == 0);
    ASSERT(output->logdata != NULL);
    ASSERT(output->logdata->raw_event != NULL);

    len = output->logdata->raw_event->len;
    if ( len > output->bufsize - sizeof(NX_LINEFEED) )
    {
	log_error("data size (%lu) is over the limit (%lu), will be truncated",
		  len, output->bufsize);
	len = output->bufsize - sizeof(NX_LINEFEED);
    }
    ASSERT(output->logdata->raw_event->buf != NULL);
    memcpy(output->buf, output->logdata->raw_event->buf, len);
    if ( addlf == TRUE )
    {
	memcpy(output->buf + len, NX_LINEFEED, sizeof(NX_LINEFEED) - 1);
	len += sizeof(NX_LINEFEED) - 1;
    }
    output->buflen = len;
    output->bufstart = 0;
}



void nx_module_output_func_linewriter(nx_module_output_t *output,
				      void *data UNUSED)
{
    nx_module_output_fill_buffer(output, TRUE);
}



void nx_module_output_func_dgramwriter(nx_module_output_t *output,
				       void *data UNUSED)
{
    nx_module_output_fill_buffer(output, FALSE);
}



void nx_module_output_func_binarywriter(nx_module_output_t *output,
					void *data UNUSED)
{
    apr_size_t memsize, memsize_got;
    apr_uint32_t size32;

    ASSERT(output != NULL);
    ASSERT(output->buf != NULL);
    ASSERT(output->buflen == 0);
    ASSERT(output->logdata != NULL);

    memsize = nx_logdata_serialized_size(output->logdata);
    if ( memsize + 8 > output->bufsize )
    {
	log_error("binary logdata (%d bytes) does not fit in output buffer (size: %d bytes), dropping",
		  (int) memsize, (int) output->bufsize);
	nx_module_logqueue_drop(output->module, output->logdata);
	output->logdata = NULL;
	return;
    }
    memcpy(output->buf, NX_LOGDATA_BINARY_HEADER, 4);

    size32 = (apr_uint32_t) memsize;
    nx_int32_to_le(output->buf + 4, &size32);
    //log_info("datalen: %d", (int) size32);

    memsize_got = nx_logdata_to_membuf(output->logdata, output->buf + 8, memsize);
    ASSERT(memsize_got == memsize);
    output->buflen = memsize + 8;
    output->bufstart = 0;
}
