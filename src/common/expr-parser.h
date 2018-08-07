/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_EXPR_PARSER_H
#define __NX_EXPR_PARSER_H

#include "expr.h"
#include "expr-grammar.h"
#include "module.h"

#define YYSTYPE nx_expr_parser_yystype
#define YY_EXTRA_TYPE nx_expr_parser_t *

/* This is a hack to get it built on darwin */
# ifdef __APPLE__
# undef bool
#endif

typedef union nx_expr_parser_yystype
{
    nx_expr_t	*expr;
    char	*string;
    char	*regexp[3]; // regexp, replacement, options
    boolean	bool;
    nx_expr_list_t *exprs;
    nx_expr_statement_list_t *statement_list;
    nx_expr_statement_t *statement;
} nx_expr_parser_yystype;

struct nx_expr_parser_t
{
    apr_pool_t	*pool;
    void	*yyscanner;
    const char	*buf;
    const char	*file;
    int		pos;
    int		linenum;
    int		linepos;
    int		length;
    boolean	parse_expression;
    nx_expr_statement_list_t *statements;
    nx_expr_t	*expression;
    nx_module_t *module;
};

void nx_expr_parser_error_fmt(nx_expr_parser_t	*parser,
			      void *scanner UNUSED,
			      const char *fmt,
			      ...) NORETURN;
void nx_expr_parser_error(nx_expr_parser_t *parser,
			  void *scanner UNUSED,
			  const char *msg) NORETURN;
void nx_expr_parser_append_string(nx_expr_parser_t *parser,
				  char **dst,
				  const char *src);
void nx_expr_parser_unescape_string(char *str);
const char *nx_expr_parser_new_string(nx_expr_parser_t *parser, const char *src);

nx_expr_statement_list_t *nx_expr_parse_statements(nx_module_t *module,
						   const char *str,
						   apr_pool_t *pool,
						   const char *filename,
						   int currline,
						   int currpos);
nx_expr_t *nx_expr_parse(nx_module_t *module,
			 const char *str,
			 apr_pool_t *pool,
			 const char *filename,
			 int currline,
			 int currpos);

#endif	/* __NX_EXPR_PARSER_H */
