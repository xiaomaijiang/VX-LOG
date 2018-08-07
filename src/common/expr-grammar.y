/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

%{
#include "error_debug.h"
#include "expr.h"
#include "expr-parser.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE
extern int nx_expr_parser_lex();

%}

%name-prefix="nx_expr_parser_"
%defines
%debug
%error-verbose
%pure_parser
%parse-param {nx_expr_parser_t *parser}
%parse-param {void *scanner}
%lex-param {yyscan_t *scanner}


// terminals/tokens:

%token 		TOKEN_ASSIGNMENT		"="
%token 		TOKEN_AND
%token 		TOKEN_OR
%token 		TOKEN_XOR
%token 		TOKEN_BINAND			"&"
%token 		TOKEN_BINOR			"|"
%token 		TOKEN_BINXOR			"^"
%token 		TOKEN_IF
%token 		TOKEN_ELSE
%token 		TOKEN_MUL			"*"
%token 		TOKEN_DIV			"/"
%token 		TOKEN_MOD			"%"
%token 		TOKEN_PLUS			"+"
%token 		TOKEN_MINUS			"-"
%token 		TOKEN_REGMATCH			"=~"
%token 		TOKEN_NOTREGMATCH		"!~"
%token 		TOKEN_EQUAL			"=="
%token 		TOKEN_NOTEQUAL			"!="
%token 		TOKEN_GE			">="
%token 		TOKEN_GREATER			">"
%token 		TOKEN_LE			"<="
%token 		TOKEN_LESS			"<"
%token 		TOKEN_IN
%token 		TOKEN_LEFTBRACKET		"("
%token 		TOKEN_RIGHTBRACKET		")"
%token 		TOKEN_LEFTBRACE			"{"
%token 		TOKEN_RIGHTBRACE		"}"
%token 		TOKEN_SEMICOLON			";"
%token 		TOKEN_COMMA			","
%token 		TOKEN_NOT
%token 		TOKEN_DEFINED

%token <string>	TOKEN_FIELDNAME
%token <string>	TOKEN_CAPTURED
%token <string>	TOKEN_FUNCPROC
%token <string>	TOKEN_STRING
%token <regexp>	TOKEN_REGEXP
%token <regexp>	TOKEN_REGEXPREPLACE
%token <bool>	TOKEN_BOOLEAN
%token <string>	TOKEN_UNDEF
%token <string>	TOKEN_INTEGER
%token <string>	TOKEN_DATETIME
%token <string>	TOKEN_IP4ADDR

 // types of non-terminals:
%type <expr>	expr
%type <expr>	literal
%type <expr>	regexpreplace
%type <expr>	regexp
%type <expr>	function
%type <exprs>	exprs
%type <exprs>	opt_exprs
%type <statement> stmt
%type <statement> stmt_if
%type <statement> stmt_no_if
%type <statement> assignment
%type <statement> procedure
%type <statement> stmt_block
%type <statement> stmt_regexpreplace
%type <statement> stmt_regexp
%type <statement_list> stmt_list
%type <expr>	left_value

// operator precedence:
%left TOKEN_OR
%left TOKEN_XOR
%left TOKEN_AND
%left TOKEN_EQUAL TOKEN_REGMATCH TOKEN_NOTREGMATCH TOKEN_NOTEQUAL TOKEN_LESS TOKEN_LE TOKEN_GREATER TOKEN_GE
%left TOKEN_BINOR
%left TOKEN_BINXOR
%left TOKEN_BINAND
%left TOKEN_PLUS TOKEN_MINUS
%left TOKEN_MUL TOKEN_DIV TOKEN_MOD
%right UNARY

%start nxblock
%%

// grammar rules:

nxblock		: { log_debug("empty block"); } // empty block
		| stmt_list 
                   {
		       if ( parser->parse_expression == TRUE )
		       {
			   nx_expr_parser_error(parser, NULL, "Expression required, found a statement");
		       }
		       parser->statements = $1;
		       log_debug("finished parsing statements"); 
		   }
		| expr 
		   {
		       if ( parser->parse_expression != TRUE )
		       {
			   nx_expr_parser_error(parser, NULL, "Statement required, expression found");
		       }
		       parser->expression = $1;
		       log_debug("parsed expression");
                   }
		;

stmt_list	: stmt
		    {
			$$ = nx_expr_statement_list_new(parser, $1);
		    }
    		| stmt_list stmt
		    {
			$$ = nx_expr_statement_list_add(parser, $1, $2);
		    }
 		;

stmt		: stmt_if 
		| stmt_no_if
		;

stmt_no_if 	: stmt_block 
		    {
			$$ = $1;
			log_debug("statement: block");
		    }
		| TOKEN_IF expr stmt_no_if TOKEN_ELSE stmt_no_if
		    {
			$$ = nx_expr_statement_new_ifelse(parser, $2, $3, $5);
			log_debug("if-else");
		    }
		| procedure TOKEN_SEMICOLON
		    {
			$$ = $1;
			log_debug("statement: procedure");
		    }
		| assignment TOKEN_SEMICOLON
		    {
			$$ = $1;
			log_debug("statement: assignment");
		    }
		| stmt_regexpreplace TOKEN_SEMICOLON
		    {
			$$ = $1;
			log_debug("statement: regexpreplace");
		    }
		| stmt_regexp TOKEN_SEMICOLON
		    {
			$$ = $1;
			log_debug("statement: regexp");
		    }
		| TOKEN_SEMICOLON
		    {
			$$ = NULL;
			log_debug("empty statement - single semicolon");
		    }
        	;        	

stmt_if		: TOKEN_IF expr stmt
                    {
			$$ = nx_expr_statement_new_ifelse(parser, $2, $3, NULL);
			log_debug("if");
		    }
		| TOKEN_IF expr stmt_no_if TOKEN_ELSE stmt_if
		    {
			$$ = nx_expr_statement_new_ifelse(parser, $2, $3, $5);
			log_debug("if-else");
		    }
        	;

stmt_block
        	: TOKEN_LEFTBRACE TOKEN_RIGHTBRACE
		    {
			$$ = NULL;
		    }
        	| TOKEN_LEFTBRACE stmt_list TOKEN_RIGHTBRACE
		    {
			$$ = nx_expr_statement_new_block(parser, $2);
		    }
        	;

procedure	: TOKEN_FUNCPROC TOKEN_LEFTBRACKET opt_exprs TOKEN_RIGHTBRACKET
		   {
		       $$ = nx_expr_statement_new_procedure(parser, $1, $3);
		       log_debug("procedure");
		   }
		;

function	: TOKEN_FUNCPROC TOKEN_LEFTBRACKET opt_exprs TOKEN_RIGHTBRACKET
		   {
		       $$ = nx_expr_new_function(parser, $1, $3);
		       log_debug("new function: %s", $1);
		   }
		;

assignment	: left_value TOKEN_ASSIGNMENT expr
		    {
			$$ = nx_expr_statement_new_assignment(parser, $1, $3);
			log_debug("assignment: left_value = expr");
		    }
		;

stmt_regexpreplace : expr TOKEN_REGMATCH regexpreplace 
			{
			    $$ = nx_expr_statement_new_regexpreplace(parser, $1, $3); 
			}
		   | expr TOKEN_NOTREGMATCH regexpreplace 
			{
			    log_warn("useless use of negative pattern binding (!~) in regexp replacement");
			    $$ = nx_expr_statement_new_regexpreplace(parser, $1, $3); 
			}
		;

stmt_regexp	: expr TOKEN_REGMATCH regexp 
		    {
			$$ = nx_expr_statement_new_regexp(parser, $1, $3); 
		    }
		| expr TOKEN_NOTREGMATCH regexp 
		    {
			log_warn("useless use of negative pattern binding (!~) in regexp match");
			$$ = nx_expr_statement_new_regexp(parser, $1, $3); 
		    }
		;

left_value	: expr
		    {
			$$ = $1;
			log_debug("left_value expr");
		    }
		;

opt_exprs	: { $$ = NULL; } // empty expression
		| exprs { $$ = $1; }
		;

exprs		: expr
		    {
			$$ = nx_expr_list_new(parser, $1);
			//log_debug("expr");
		    }
		| exprs TOKEN_COMMA expr
		    {
			$$ = nx_expr_list_add(parser, $1, $3);
			//log_debug("exprs, expr");
		    }
		;

expr		: literal { log_debug("literal"); $$ = $1; }
		| TOKEN_FIELDNAME { $$ = nx_expr_new_field(parser, $1); }
		| TOKEN_CAPTURED { $$ = nx_expr_new_captured(parser, $1); }
		| '-' expr %prec UNARY { $$ = nx_expr_new_unop(parser, TOKEN_MINUS, $2); }
		| expr TOKEN_MINUS expr { $$ = nx_expr_new_binop(parser, TOKEN_MINUS, $1, $3); }
		| TOKEN_NOT expr %prec UNARY { $$ = nx_expr_new_unop(parser, TOKEN_NOT, $2); }
		| TOKEN_DEFINED expr %prec UNARY { $$ = nx_expr_new_unop(parser, TOKEN_DEFINED, $2); }
		| expr TOKEN_MUL expr { $$ = nx_expr_new_binop(parser, TOKEN_MUL, $1, $3); }
		| expr TOKEN_DIV expr { log_debug("division"); $$ = nx_expr_new_binop(parser, TOKEN_DIV, $1, $3); }
		| expr TOKEN_MOD expr { $$ = nx_expr_new_binop(parser, TOKEN_MOD, $1, $3); }
		| expr TOKEN_PLUS expr { $$ = nx_expr_new_binop(parser, TOKEN_PLUS, $1, $3); }
		| expr TOKEN_BINAND expr { $$ = nx_expr_new_binop(parser, TOKEN_BINAND, $1, $3); }
		| expr TOKEN_BINXOR expr { $$ = nx_expr_new_binop(parser, TOKEN_BINXOR, $1, $3); }
		| expr TOKEN_BINOR expr { $$ = nx_expr_new_binop(parser, TOKEN_BINOR, $1, $3); }
		| expr TOKEN_REGMATCH expr { $$ = nx_expr_new_binop(parser, TOKEN_REGMATCH, $1, $3); }
		| expr TOKEN_NOTREGMATCH expr { $$ = nx_expr_new_binop(parser, TOKEN_NOTREGMATCH, $1, $3); }
		| expr TOKEN_REGMATCH regexpreplace { $$ = nx_expr_new_binop(parser, TOKEN_REGMATCH, $1, $3); }
		| expr TOKEN_NOTREGMATCH regexpreplace { $$ = nx_expr_new_binop(parser, TOKEN_NOTREGMATCH, $1, $3); }
		| expr TOKEN_EQUAL expr { $$ = nx_expr_new_binop(parser, TOKEN_EQUAL, $1, $3); }
		| expr TOKEN_NOTEQUAL expr { $$ = nx_expr_new_binop(parser, TOKEN_NOTEQUAL, $1, $3); }
		| expr TOKEN_LESS expr { $$ = nx_expr_new_binop(parser, TOKEN_LESS, $1, $3); }
		| expr TOKEN_LE expr { $$ = nx_expr_new_binop(parser, TOKEN_LE, $1, $3); }
		| expr TOKEN_GREATER expr { $$ = nx_expr_new_binop(parser, TOKEN_GREATER, $1, $3); }
		| expr TOKEN_GE expr { $$ = nx_expr_new_binop(parser, TOKEN_GE, $1, $3); }
		| expr TOKEN_AND expr { $$ = nx_expr_new_binop(parser, TOKEN_AND, $1, $3); }
		| expr TOKEN_XOR expr { $$ = nx_expr_new_binop(parser, TOKEN_XOR, $1, $3); }
		| expr TOKEN_OR expr { $$ = nx_expr_new_binop(parser, TOKEN_OR, $1, $3); }
		| expr TOKEN_IN exprs { $$ = nx_expr_new_inop(parser, $1, $3); }
		| expr TOKEN_IN TOKEN_LEFTBRACKET exprs TOKEN_RIGHTBRACKET { $$ = nx_expr_new_inop(parser, $1, $4); }
		| expr TOKEN_NOT TOKEN_IN exprs { $$ = nx_expr_new_unop(parser, TOKEN_NOT, nx_expr_new_inop(parser, $1, $4)); }
		| expr TOKEN_NOT TOKEN_IN TOKEN_LEFTBRACKET exprs TOKEN_RIGHTBRACKET { $$ = nx_expr_new_unop(parser, TOKEN_NOT, nx_expr_new_inop(parser, $1, $5)); }
		| function { $$ = $1; }
		| TOKEN_LEFTBRACKET expr TOKEN_RIGHTBRACKET { log_debug("( expr:%d )", $2->type); $$ = $2; }
		;

regexpreplace   : TOKEN_REGEXPREPLACE { log_debug("regexpreplace"); $$ = nx_expr_new_regexp(parser, $1[0], $1[1], $1[2]); }
		;

regexp		: TOKEN_REGEXP { log_debug("regexp"); $$ = nx_expr_new_regexp(parser, $1[0], NULL, $1[2]); }
		;

literal		: TOKEN_STRING { $$ = nx_expr_new_string(parser, $1); }
		| TOKEN_REGEXP { log_debug("regexp literal"); $$ = nx_expr_new_regexp(parser, $1[0], NULL, $1[2]); }
		| TOKEN_BOOLEAN { log_debug("boolean literal"); $$ = nx_expr_new_boolean(parser, $1); }
		| TOKEN_UNDEF { $$ = nx_expr_new_undef(parser); }
		| TOKEN_INTEGER { log_debug("integer literal: %s", $1); $$ = nx_expr_new_integer(parser, $1); }
		| TOKEN_DATETIME { log_debug("datetime literal: %s", $1); $$ = nx_expr_new_datetime(parser, $1); }
		| TOKEN_IP4ADDR { log_debug("ip4addr literal: %s", $1); $$ = nx_expr_new_ip4addr(parser, $1); }
		;
%%

int nx_expr_parser_lex_init(void **);
int nx_expr_parser_lex_destroy(void *);
void nx_expr_parser_set_extra(YY_EXTRA_TYPE, void *);


static void parser_do(nx_expr_parser_t *parser,
		      nx_module_t *module,
		      const char *str,
		      boolean parse_expression,
		      apr_pool_t *pool,
		      const char *filename,
		      int currline,
		      int currpos)
{
    nx_exception_t e;

    ASSERT(parser != NULL);

    memset(parser, 0, sizeof(nx_expr_parser_t));
    parser->buf = str;
    parser->length = (int) strlen(str);
    parser->pos = 0;
    parser->pool = pool;
    parser->linenum = currline;
    parser->linepos = currpos;

    if ( filename == NULL )
    {
	parser->file = NULL;
    }
    else
    {
	parser->file = apr_pstrdup(pool, filename);
    }
    parser->parse_expression = parse_expression;
    parser->module = module;

    try
    {
	nx_expr_parser_lex_init(&(parser->yyscanner));
	nx_expr_parser_set_extra(parser, parser->yyscanner);
	yyparse(parser, parser->yyscanner);
    }
    catch(e)
    {
	if ( parser->file == NULL )
	{
	    rethrow_msg(e, "couldn't parse %s at line %d, character %d",
			parse_expression == TRUE ? "expression" : "statement",
			parser->linenum, parser->linepos);
	}
	else
	{
	    rethrow_msg(e, "couldn't parse %s at line %d, character %d in %s",
			parse_expression == TRUE ? "expression" : "statement",
			parser->linenum, parser->linepos, parser->file);
	}
    }

    nx_expr_parser_lex_destroy(parser->yyscanner);
}



nx_expr_statement_list_t *nx_expr_parse_statements(nx_module_t *module,
						   const char *str,
						   apr_pool_t *pool,
						   const char *filename,
						   int currline,
						   int currpos)
{
    nx_expr_parser_t parser;

    parser_do(&parser, module, str, FALSE, pool, filename, currline, currpos);

    return ( parser.statements );
}



nx_expr_t *nx_expr_parse(nx_module_t *module,
			 const char *str,
			 apr_pool_t *pool,
			 const char *filename,
			 int currline,
			 int currpos)
{
    nx_expr_parser_t parser;

    parser_do(&parser, module, str, TRUE, pool, filename, currline, currpos);

    return ( parser.expression );
}

