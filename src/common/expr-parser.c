/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "error_debug.h"
#include "expr-parser.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


void nx_expr_parser_error_fmt(nx_expr_parser_t	*parser,
			      void *scanner UNUSED,
			      const char *fmt,
			      ...)
{
    va_list ap;
    char buf[NX_LOGBUF_SIZE];

    ASSERT(parser != NULL);

    va_start(ap, fmt);
    apr_vsnprintf(buf, NX_LOGBUF_SIZE, fmt, ap);
    va_end(ap);
    throw_msg("%s", buf);
}



void nx_expr_parser_error(nx_expr_parser_t *parser UNUSED,
			  void *scanner UNUSED,
			  const char *msg)
{
    throw_msg("%s", msg);
}



void nx_expr_parser_append_string(nx_expr_parser_t *parser,
				  char **dst,
				  const char *src)
{
    size_t len1, len2;

    ASSERT(parser != NULL);
    ASSERT(src != NULL);

    if ( *dst == NULL )
    {
	log_debug("adding string [%s]", src);
	len1 = strlen(src);
	*dst = apr_palloc(parser->pool, len1 + 1);
	apr_cpystrn(*dst, src, len1 + 1);
    }
    else
    {
	char *tmp;
	log_debug("appending [%s] to [%s]", src, *dst);
	len1 = strlen(*dst);
	len2 = strlen(src);

	tmp = apr_palloc(parser->pool, len1 + len2 + 1);
	apr_cpystrn(tmp, *dst, len1 + 1);
	apr_cpystrn(tmp + len1, src, len2 + 1);
	*dst = tmp;
    }
}



const char *nx_expr_parser_new_string(nx_expr_parser_t *parser, const char *src)
{
    ASSERT(src != NULL);

    return ( apr_pstrdup(parser->pool, src) );
}

