/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

%option prefix="nx_expr_parser_"
%option outfile="lex.yy.c"

%{
#include "expr.h"
#include "str.h"
#include "expr-grammar.h"
#include "expr-parser.h"
#include "error_debug.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define PARSER yyget_extra(yyscanner)
#define YY_INPUT(buffer, res, max_size)             	    \
    do {                                                    \
        if (PARSER->pos >= PARSER->length)                  \
            res = YY_NULL;                                  \
        else                                                \
        {                                                   \
            res = PARSER->length - PARSER->pos;             \
            res > (int)max_size ? res = max_size : 0;       \
            memcpy(buffer, PARSER->buf + PARSER->pos, res); \
            PARSER->pos += res;                             \
        }                                                   \
    } while (0)

#define YY_USER_ACTION PARSER->linepos += yyleng;

%}

COMMENT		"#".*
NAME		[[:alpha:]_][[:alnum:]\._]*
SPACE		[ \t]*
FIELDNAME	"$"{NAME}
CAPTURED	"$"[0-9]|"$"[1-9][0-9]+
DATETIME        [1-9][0-9]{3}"-"[0-9]{2}"-"[0-9]{2}[ ][0-9]{2}":"[0-9]{2}":"[0-9]{2}
IP4ADDR		[0-9]{1,3}"."[0-9]{1,3}"."[0-9]{1,3}"."[0-9]{1,3}
INTEGER         "-"?[0-9]+[KMG]?|"-"?"0X"[0-9A-Z]+[KMG]? 

%x DOUBLEQUOTE
%x SINGLEQUOTE
%x REGEXPQUOTE
%x REGEXPREPLACEQUOTE
%x REGEXPREPLACEQUOTE2
%x REGEXPREPLACEMODIFIERS
%x REGEXPMODIFIERS
%x REGEXP

%option case-insensitive
%option reentrant bison-bridge
%option noyywrap

%%

"\""			{ yylval->string = NULL; BEGIN(DOUBLEQUOTE); }
"'"			{ yylval->string = NULL; BEGIN(SINGLEQUOTE);  }
"TRUE"			{ yylval->bool = TRUE; return ( TOKEN_BOOLEAN ); }
"FALSE"			{ yylval->bool = FALSE; return ( TOKEN_BOOLEAN ); }
{IP4ADDR}		{ yylval->string = yytext; return ( TOKEN_IP4ADDR ); }
{INTEGER}		{ yylval->string = yytext; return ( TOKEN_INTEGER ); }
{DATETIME}		{ yylval->string = yytext; return ( TOKEN_DATETIME ); }

"UNDEF"			{ yylval->string = "UNDEF"; return ( TOKEN_UNDEF ); }
"=~"			{ BEGIN(REGEXP); return ( TOKEN_REGMATCH ); }
"!~"			{ BEGIN(REGEXP); return ( TOKEN_NOTREGMATCH ); }


<SINGLEQUOTE>{
    "\n"		{ 
                          /* reset column counter on new line */
                          PARSER->linepos = 0; 
                          (PARSER->linenum)++;  
                          nx_expr_parser_append_string(PARSER, &(yylval->string), yytext);
                        }
    [^'\n]+		{ nx_expr_parser_append_string(PARSER, &(yylval->string), yytext); }
    "'"			{
			  BEGIN(INITIAL);
			  return ( TOKEN_STRING );
                        }
}

<REGEXP>{
    [ \t]+		{  }
    "s/"		{ log_debug("regexpreplacequote start"); yylval->regexp[0] = NULL; yylval->regexp[1] = NULL; yylval->regexp[2] = NULL; BEGIN(REGEXPREPLACEQUOTE); }
    [/]			{ log_debug("regexpquote start"); yylval->regexp[0] = NULL; yylval->regexp[1] = NULL; yylval->regexp[2] = NULL; BEGIN(REGEXPQUOTE); }
}

<REGEXPQUOTE>{
    "\\\\"		{ log_debug("regexp: append backslash"); nx_expr_parser_append_string(PARSER, &(yylval->string), yytext); }
    "\\/"		{ log_debug("regexp: append slash"); nx_expr_parser_append_string(PARSER, &(yylval->string), yytext); }
    [^/]		{ log_debug("regexp: append [%s]", yytext); nx_expr_parser_append_string(PARSER, &(yylval->string), yytext); }
    [/]			{
			  log_debug("regexpquote end");
			  BEGIN(REGEXPMODIFIERS);
                        }
}

<REGEXPREPLACEQUOTE>{
    "\\\\"		{ log_debug("regexpreplacequote: append backslash"); nx_expr_parser_append_string(PARSER, &(yylval->regexp[0]), yytext); }
    "\\/"		{ log_debug("regexpreplacequote: append slash"); nx_expr_parser_append_string(PARSER, &(yylval->regexp[0]), yytext); }
    [^/]		{ log_debug("regexpreplacequote: append [%s]", yytext); nx_expr_parser_append_string(PARSER, &(yylval->regexp[0]), yytext); }
    [/]			{
			  log_debug("regexpreplacequote2 begin"); 
			  BEGIN(REGEXPREPLACEQUOTE2);
                        }
}

<REGEXPREPLACEQUOTE2>{
    "\\\\"		{ log_debug("regexpreplacequote2: append backslash"); nx_expr_parser_append_string(PARSER, &(yylval->regexp[1]), yytext); }
    "\\/"		{ log_debug("regexpreplacequote2: append slash"); nx_expr_parser_append_string(PARSER, &(yylval->regexp[1]), yytext); }
    [^/]		{ log_debug("regexpreplacequote2: append [%s]", yytext); nx_expr_parser_append_string(PARSER, &(yylval->regexp[1]), yytext); }
    [/]			{
			  log_debug("regexpreplacequote2 end"); 
			  if ( yylval->regexp[1] == NULL ) yylval->regexp[1] = apr_pcalloc(PARSER->pool, 1); /*strdup("")*/;
			  BEGIN(REGEXPREPLACEMODIFIERS);
                        }
}

<REGEXPREPLACEMODIFIERS>{
    [gsmi]		{
			  log_debug("regexpreplacemodifiers begin [%s]", yytext); 
			  nx_expr_parser_append_string(PARSER, &(yylval->regexp[2]), yytext);
                        }
    <<EOF>>		{
			  log_debug("regexpreplacemodifiers end with eof"); 
			  BEGIN(INITIAL);
			  return ( TOKEN_REGEXPREPLACE );
                        }
    [^g^s^m^i]		{
			  log_debug("regexpreplacemodifiers end");
			  unput(yytext[0]);
			  BEGIN(INITIAL);
			  return ( TOKEN_REGEXPREPLACE );
                        }
}

<REGEXPMODIFIERS>{
    [smi]		{
			  log_debug("regexpmodifiers begin [%s]", yytext); 
			  nx_expr_parser_append_string(PARSER, &(yylval->regexp[2]), yytext);
                        }
    <<EOF>>		{
			  log_debug("regexpmodifiers end with eof"); 
			  BEGIN(INITIAL);
			  return ( TOKEN_REGEXP );
                        }
    [^s^m^i]		{
			  log_debug("regexpmodifiers end"); 
			  unput(yytext[0]);
			  BEGIN(INITIAL);
			  return ( TOKEN_REGEXP );
                        }
}

<DOUBLEQUOTE>{
    "\n"		{ 
                          /* reset column counter on new line */
                          PARSER->linepos = 0; 
                          (PARSER->linenum)++;  
                          nx_expr_parser_append_string(PARSER, &(yylval->string), yytext);
                        }
     [^\"\n]+          { nx_expr_parser_append_string(PARSER, &(yylval->string), yytext); }
     "\\\""             { nx_expr_parser_append_string(PARSER, &(yylval->string), yytext); }
     "\""               { 
			  BEGIN(INITIAL); 
                          if ( yylval->string != NULL ) nx_string_unescape_c(yylval->string);
			  return ( TOKEN_STRING );
		        }
}

"="	return ( TOKEN_ASSIGNMENT );
"not"	return ( TOKEN_NOT );
"defined" return ( TOKEN_DEFINED );
"and"	return ( TOKEN_AND );
"or"	return ( TOKEN_OR );
"xor"	return ( TOKEN_XOR );
"&"	return ( TOKEN_BINAND );
"|"	return ( TOKEN_BINOR );
"^"	return ( TOKEN_BINXOR );
"if"	return ( TOKEN_IF );
"else"	return ( TOKEN_ELSE );
"*"	return ( TOKEN_MUL );
"/"	return ( TOKEN_DIV );
"%"	return ( TOKEN_MOD );
"+"	return ( TOKEN_PLUS );
"-"	return ( TOKEN_MINUS );
"=="	return ( TOKEN_EQUAL );
"!="	return ( TOKEN_NOTEQUAL );
">="	return ( TOKEN_GE );
">"	return ( TOKEN_GREATER );
"<="	return ( TOKEN_LE );
"<"	return ( TOKEN_LESS );
"in"	return ( TOKEN_IN );
"("	return ( TOKEN_LEFTBRACKET );
")"	return ( TOKEN_RIGHTBRACKET );
"{"	return ( TOKEN_LEFTBRACE );
"}"	return ( TOKEN_RIGHTBRACE );
";"	return ( TOKEN_SEMICOLON );
","	return ( TOKEN_COMMA );

{FIELDNAME}		{ yylval->string = nx_expr_parser_new_string(PARSER, yytext + 1); return ( TOKEN_FIELDNAME ); }
{NAME}"->"{NAME}	{ yylval->string = nx_expr_parser_new_string(PARSER, yytext); return ( TOKEN_FUNCPROC ); }
{NAME}			{ yylval->string = nx_expr_parser_new_string(PARSER, yytext); return ( TOKEN_FUNCPROC ); }
{CAPTURED}		{ yylval->string = nx_expr_parser_new_string(PARSER, yytext + 1); return ( TOKEN_CAPTURED ); }


[[:blank:]]+		/* ignore blanks */
{COMMENT}		/* ignore comment */
"\\\n"                  /* ignore backslash newline escape */
"\\\r\n"                /* ignore backslash newline escape */
"\n"			PARSER->linepos = 0; (PARSER->linenum)++; /* reset column counter on new line */
"\r\n"			PARSER->linepos = 0; (PARSER->linenum)++; /* reset column counter on new line */
.			nx_expr_parser_error_fmt(PARSER, yyscanner, "invalid character: '%c' (0x%x)", yytext[0], yytext[0]);

<*>.			nx_expr_parser_error(PARSER, yyscanner, "syntax error");

%%

