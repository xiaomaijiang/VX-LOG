/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "backtrace.h"
#include "../core/ctx.h"

#ifdef HAVE_EXECINFO_H
# include <execinfo.h>
#endif

#define NX_LOGMODULE NX_LOGMODULE_CORE

boolean nx_append_backtrace(nx_string_t *str)
{
#ifdef HAVE_BACKTRACE_SYMBOLS
    void **frames = NULL;
    int num_frame;
    char **symbols = NULL;
    int i;

    ASSERT(str != NULL);

    frames = malloc(sizeof(void*) * NX_BACKTRACE_SIZE);
    num_frame = backtrace(frames, NX_BACKTRACE_SIZE);
    symbols = backtrace_symbols(frames, num_frame);
    if ( symbols != NULL )
    {
	if ( str->len > 0 )
	{
	    nx_string_append(str, NX_LINEFEED, -1);
	}
	nx_string_append(str, "backtrace:", -1);

	for ( i = 0; i < num_frame; i++ )
	{
	    nx_string_append(str, NX_LINEFEED, -1);
	    nx_string_append(str, symbols[i], -1);
	}
	free(symbols);
    }

    free(frames);
    
    return ( TRUE );
#else
    nx_string_append(str, NX_LINEFEED, -1);
    nx_string_append(str, "[cannot dump backtrace on this platform]", -1);

    return ( FALSE );
#endif
}
