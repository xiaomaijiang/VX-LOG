/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_NX_EXPR_PARSER_EXPR_GRAMMAR_H_INCLUDED
# define YY_NX_EXPR_PARSER_EXPR_GRAMMAR_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int nx_expr_parser_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOKEN_ASSIGNMENT = 258,
    TOKEN_AND = 259,
    TOKEN_OR = 260,
    TOKEN_XOR = 261,
    TOKEN_BINAND = 262,
    TOKEN_BINOR = 263,
    TOKEN_BINXOR = 264,
    TOKEN_IF = 265,
    TOKEN_ELSE = 266,
    TOKEN_MUL = 267,
    TOKEN_DIV = 268,
    TOKEN_MOD = 269,
    TOKEN_PLUS = 270,
    TOKEN_MINUS = 271,
    TOKEN_REGMATCH = 272,
    TOKEN_NOTREGMATCH = 273,
    TOKEN_EQUAL = 274,
    TOKEN_NOTEQUAL = 275,
    TOKEN_GE = 276,
    TOKEN_GREATER = 277,
    TOKEN_LE = 278,
    TOKEN_LESS = 279,
    TOKEN_IN = 280,
    TOKEN_LEFTBRACKET = 281,
    TOKEN_RIGHTBRACKET = 282,
    TOKEN_LEFTBRACE = 283,
    TOKEN_RIGHTBRACE = 284,
    TOKEN_SEMICOLON = 285,
    TOKEN_COMMA = 286,
    TOKEN_NOT = 287,
    TOKEN_DEFINED = 288,
    TOKEN_FIELDNAME = 289,
    TOKEN_CAPTURED = 290,
    TOKEN_FUNCPROC = 291,
    TOKEN_STRING = 292,
    TOKEN_REGEXP = 293,
    TOKEN_REGEXPREPLACE = 294,
    TOKEN_BOOLEAN = 295,
    TOKEN_UNDEF = 296,
    TOKEN_INTEGER = 297,
    TOKEN_DATETIME = 298,
    TOKEN_IP4ADDR = 299,
    UNARY = 300
  };
#endif
/* Tokens.  */
#define TOKEN_ASSIGNMENT 258
#define TOKEN_AND 259
#define TOKEN_OR 260
#define TOKEN_XOR 261
#define TOKEN_BINAND 262
#define TOKEN_BINOR 263
#define TOKEN_BINXOR 264
#define TOKEN_IF 265
#define TOKEN_ELSE 266
#define TOKEN_MUL 267
#define TOKEN_DIV 268
#define TOKEN_MOD 269
#define TOKEN_PLUS 270
#define TOKEN_MINUS 271
#define TOKEN_REGMATCH 272
#define TOKEN_NOTREGMATCH 273
#define TOKEN_EQUAL 274
#define TOKEN_NOTEQUAL 275
#define TOKEN_GE 276
#define TOKEN_GREATER 277
#define TOKEN_LE 278
#define TOKEN_LESS 279
#define TOKEN_IN 280
#define TOKEN_LEFTBRACKET 281
#define TOKEN_RIGHTBRACKET 282
#define TOKEN_LEFTBRACE 283
#define TOKEN_RIGHTBRACE 284
#define TOKEN_SEMICOLON 285
#define TOKEN_COMMA 286
#define TOKEN_NOT 287
#define TOKEN_DEFINED 288
#define TOKEN_FIELDNAME 289
#define TOKEN_CAPTURED 290
#define TOKEN_FUNCPROC 291
#define TOKEN_STRING 292
#define TOKEN_REGEXP 293
#define TOKEN_REGEXPREPLACE 294
#define TOKEN_BOOLEAN 295
#define TOKEN_UNDEF 296
#define TOKEN_INTEGER 297
#define TOKEN_DATETIME 298
#define TOKEN_IP4ADDR 299
#define UNARY 300

/* Value type.  */



int nx_expr_parser_parse (nx_expr_parser_t *parser, void *scanner);

#endif /* !YY_NX_EXPR_PARSER_EXPR_GRAMMAR_H_INCLUDED  */
