/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         nx_expr_parser_parse
#define yylex           nx_expr_parser_lex
#define yyerror         nx_expr_parser_error
#define yydebug         nx_expr_parser_debug
#define yynerrs         nx_expr_parser_nerrs


/* Copy the first part of user declarations.  */
#line 8 "expr-grammar.y" /* yacc.c:339  */

#include "error_debug.h"
#include "expr.h"
#include "expr-parser.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE
extern int nx_expr_parser_lex();


#line 82 "expr-grammar.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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

/* Copy the second part of user declarations.  */

#line 217 "expr-grammar.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  42
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   525

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  47
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  19
/* YYNRULES -- Number of rules.  */
#define YYNRULES  73
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  127

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   300

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    46,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   110,   110,   111,   120,   131,   135,   141,   142,   145,
     150,   155,   160,   165,   170,   175,   182,   187,   195,   199,
     205,   212,   219,   226,   230,   237,   241,   248,   255,   256,
     259,   264,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   306,   309,   312,   313,   314,
     315,   316,   317,   318
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "\"=\"", "TOKEN_AND", "TOKEN_OR",
  "TOKEN_XOR", "\"&\"", "\"|\"", "\"^\"", "TOKEN_IF", "TOKEN_ELSE",
  "\"*\"", "\"/\"", "\"%\"", "\"+\"", "\"-\"", "\"=~\"", "\"!~\"",
  "\"==\"", "\"!=\"", "\">=\"", "\">\"", "\"<=\"", "\"<\"", "TOKEN_IN",
  "\"(\"", "\")\"", "\"{\"", "\"}\"", "\";\"", "\",\"", "TOKEN_NOT",
  "TOKEN_DEFINED", "TOKEN_FIELDNAME", "TOKEN_CAPTURED", "TOKEN_FUNCPROC",
  "TOKEN_STRING", "TOKEN_REGEXP", "TOKEN_REGEXPREPLACE", "TOKEN_BOOLEAN",
  "TOKEN_UNDEF", "TOKEN_INTEGER", "TOKEN_DATETIME", "TOKEN_IP4ADDR",
  "UNARY", "'-'", "$accept", "nxblock", "stmt_list", "stmt", "stmt_no_if",
  "stmt_if", "stmt_block", "procedure", "function", "assignment",
  "stmt_regexpreplace", "stmt_regexp", "left_value", "opt_exprs", "exprs",
  "expr", "regexpreplace", "regexp", "literal", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,    45
};
# endif

#define YYPACT_NINF -34

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-34)))

#define YYTABLE_NINF -67

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     176,   420,   420,   134,   -34,   420,   420,   -34,   -34,   -19,
     -34,   -34,   -34,   -34,   -34,   -34,   -34,   420,    16,   176,
     -34,   -34,   -34,   -34,     3,   -34,     7,    14,    15,    33,
       6,   -34,    13,   115,   219,   -34,   155,   248,     9,     9,
     420,     9,   -34,   -34,   -34,   -34,   -34,   -34,   420,   420,
     420,   420,   420,   420,   420,   420,   420,   420,   420,   420,
     382,   382,   420,   420,   420,   420,   420,   420,   439,    21,
     420,   401,   401,   363,   -34,    36,   -34,   -34,    42,    46,
     270,   270,   313,    80,   292,   493,   348,   362,     9,     9,
       9,   340,   340,    50,   -34,   334,    51,   -34,   334,    52,
     -34,   334,   334,   334,   334,   334,   334,   420,    46,   458,
      63,   -34,   -34,   176,    61,   420,   -23,   219,   420,    46,
     -34,   -34,   -34,   270,   -34,    48,   -34
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     0,     0,    15,     0,     0,    33,    34,     0,
      67,    68,    69,    70,    71,    72,    73,     0,     0,     3,
       5,     8,     7,     9,     0,    63,     0,     0,     0,     0,
       4,    32,     0,     0,     0,    18,     0,    27,    37,    38,
      28,    35,     1,     6,    11,    12,    13,    14,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      28,     0,     0,     0,    16,     8,    64,    19,     0,    29,
      30,    22,    56,    58,    57,    43,    45,    44,    39,    40,
      41,    42,    36,    68,    65,    46,    48,    25,    47,    49,
      26,    50,    51,    55,    54,    53,    52,     0,    59,     0,
       0,    48,    49,     0,    21,     0,     0,    30,     0,    61,
      21,    10,    17,    31,    60,     0,    62
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -34,   -34,    75,   349,   -30,    -7,   -34,   -34,   -34,   -34,
     -34,   -34,   -34,    38,   -33,     0,   -29,    49,   -34
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    78,    79,    37,    96,    97,    31
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      30,    33,    34,    75,   124,    38,    39,    40,   115,   -27,
      49,    50,    51,    52,    53,    54,    42,    41,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    99,    44,    68,   108,    48,    45,    69,    70,
      80,    69,   111,   112,    46,    47,   109,   113,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      95,    98,   101,   102,   103,   104,   105,   106,    80,   114,
      80,    95,    98,    38,   116,   126,   119,   115,    36,   115,
     -66,   -23,   -24,   121,    49,   125,    51,    52,    53,    54,
     120,   -20,    55,    56,    57,    58,    59,    71,    72,    62,
      63,    64,    65,    66,    67,    68,   122,   117,   110,    80,
     100,     0,    69,     0,     0,   123,     0,     0,   117,    49,
      50,    51,    52,    53,    54,     1,     0,    55,    56,    57,
      58,    59,    71,    72,    62,    63,    64,    65,    66,    67,
      68,     2,     0,     3,     1,     4,     0,    73,     6,     7,
       8,     9,    10,    11,     0,    12,    13,    14,    15,    16,
       2,    17,     3,    35,     4,     1,     5,     6,     7,     8,
       9,    10,    11,     0,    12,    13,    14,    15,    16,     0,
      17,     2,     0,     3,    77,     4,     1,     5,     6,     7,
       8,     9,    10,    11,     0,    12,    13,    14,    15,    16,
       0,    17,     2,     0,     3,     0,     4,     0,     5,     6,
       7,     8,     9,    10,    11,     0,    12,    13,    14,    15,
      16,     0,    17,    49,    50,    51,    52,    53,    54,     0,
       0,    55,    56,    57,    58,    59,    71,    72,    62,    63,
      64,    65,    66,    67,    68,     0,    76,     0,     0,     0,
       0,    69,    49,    50,    51,    52,    53,    54,     0,     0,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    49,    50,    51,    52,    53,    54,
      69,     0,    55,    56,    57,    58,    59,    71,    72,    62,
      63,    64,    65,    66,    67,    68,    49,     0,     0,    52,
      53,    54,    69,     0,    55,    56,    57,    58,    59,    71,
      72,    62,    63,    64,    65,    66,    67,    68,     0,     0,
      52,    53,    54,     0,    69,    55,    56,    57,    58,    59,
      71,    72,    62,    63,    64,    65,    66,    67,    68,     0,
       0,    52,    53,    54,     0,    69,    55,    56,    57,    58,
      59,     0,    55,    56,    57,    52,     0,    54,     0,    68,
      55,    56,    57,    58,    59,    68,    69,     0,    43,    52,
       0,     0,    69,    68,    55,    56,    57,    58,    59,     0,
      69,     0,    74,     0,     0,    43,     0,    68,   109,     2,
       0,     0,     0,     0,    69,     5,     6,     7,     8,    32,
      10,    11,     0,    12,    13,    14,    15,    16,     2,    17,
       0,     0,     0,     0,     5,     6,     7,     8,    32,    10,
      93,    94,    12,    13,    14,    15,    16,     2,    17,     0,
       0,     0,     0,     5,     6,     7,     8,    32,    10,    11,
      94,    12,    13,    14,    15,    16,     2,    17,     0,     0,
       0,     0,     5,     6,     7,     8,    32,    10,    11,     0,
      12,    13,    14,    15,    16,   107,    17,     0,     0,     0,
       0,     5,     6,     7,     8,    32,    10,    11,     0,    12,
      13,    14,    15,    16,   118,    17,     0,     0,     0,     0,
       5,     6,     7,     8,    32,    10,    11,     0,    12,    13,
      14,    15,    16,     0,    17,    55,    56,    57,    58,    59,
       0,     0,     0,     0,     0,     0,     0,     0,    68,     0,
       0,     0,     0,     0,     0,    69
};

static const yytype_int8 yycheck[] =
{
       0,     1,     2,    33,    27,     5,     6,    26,    31,     3,
       4,     5,     6,     7,     8,     9,     0,    17,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    61,    30,    25,    68,     3,    30,    32,    26,
      40,    32,    71,    72,    30,    30,    25,    11,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    27,
      70,    71,    72,    73,   107,    27,   109,    31,     3,    31,
      30,    30,    30,   113,     4,   118,     6,     7,     8,     9,
      27,    30,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,   113,   107,    70,   109,
      61,    -1,    32,    -1,    -1,   115,    -1,    -1,   118,     4,
       5,     6,     7,     8,     9,    10,    -1,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    -1,    28,    10,    30,    -1,    32,    33,    34,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      26,    46,    28,    29,    30,    10,    32,    33,    34,    35,
      36,    37,    38,    -1,    40,    41,    42,    43,    44,    -1,
      46,    26,    -1,    28,    29,    30,    10,    32,    33,    34,
      35,    36,    37,    38,    -1,    40,    41,    42,    43,    44,
      -1,    46,    26,    -1,    28,    -1,    30,    -1,    32,    33,
      34,    35,    36,    37,    38,    -1,    40,    41,    42,    43,
      44,    -1,    46,     4,     5,     6,     7,     8,     9,    -1,
      -1,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    -1,    27,    -1,    -1,    -1,
      -1,    32,     4,     5,     6,     7,     8,     9,    -1,    -1,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,     4,     5,     6,     7,     8,     9,
      32,    -1,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,     4,    -1,    -1,     7,
       8,     9,    32,    -1,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    -1,    -1,
       7,     8,     9,    -1,    32,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    -1,
      -1,     7,     8,     9,    -1,    32,    12,    13,    14,    15,
      16,    -1,    12,    13,    14,     7,    -1,     9,    -1,    25,
      12,    13,    14,    15,    16,    25,    32,    -1,    19,     7,
      -1,    -1,    32,    25,    12,    13,    14,    15,    16,    -1,
      32,    -1,    33,    -1,    -1,    36,    -1,    25,    25,    26,
      -1,    -1,    -1,    -1,    32,    32,    33,    34,    35,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    26,    46,
      -1,    -1,    -1,    -1,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    26,    46,    -1,
      -1,    -1,    -1,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    26,    46,    -1,    -1,
      -1,    -1,    32,    33,    34,    35,    36,    37,    38,    -1,
      40,    41,    42,    43,    44,    26,    46,    -1,    -1,    -1,
      -1,    32,    33,    34,    35,    36,    37,    38,    -1,    40,
      41,    42,    43,    44,    26,    46,    -1,    -1,    -1,    -1,
      32,    33,    34,    35,    36,    37,    38,    -1,    40,    41,
      42,    43,    44,    -1,    46,    12,    13,    14,    15,    16,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    25,    -1,
      -1,    -1,    -1,    -1,    -1,    32
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    10,    26,    28,    30,    32,    33,    34,    35,    36,
      37,    38,    40,    41,    42,    43,    44,    46,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      62,    65,    36,    62,    62,    29,    49,    62,    62,    62,
      26,    62,     0,    50,    30,    30,    30,    30,     3,     4,
       5,     6,     7,     8,     9,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    32,
      26,    17,    18,    32,    50,    51,    27,    29,    60,    61,
      62,    62,    62,    62,    62,    62,    62,    62,    62,    62,
      62,    62,    62,    38,    39,    62,    63,    64,    62,    63,
      64,    62,    62,    62,    62,    62,    62,    26,    61,    25,
      60,    63,    63,    11,    27,    31,    61,    62,    26,    61,
      27,    51,    52,    62,    27,    61,    27
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    47,    48,    48,    48,    49,    49,    50,    50,    51,
      51,    51,    51,    51,    51,    51,    52,    52,    53,    53,
      54,    55,    56,    57,    57,    58,    58,    59,    60,    60,
      61,    61,    62,    62,    62,    62,    62,    62,    62,    62,
      62,    62,    62,    62,    62,    62,    62,    62,    62,    62,
      62,    62,    62,    62,    62,    62,    62,    62,    62,    62,
      62,    62,    62,    62,    62,    63,    64,    65,    65,    65,
      65,    65,    65,    65
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     1,     2,     1,     1,     1,
       5,     2,     2,     2,     2,     1,     3,     5,     2,     3,
       4,     4,     3,     3,     3,     3,     3,     1,     0,     1,
       1,     3,     1,     1,     1,     2,     3,     2,     2,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       5,     4,     6,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (parser, scanner, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, parser, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, nx_expr_parser_t *parser, void *scanner)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (parser);
  YYUSE (scanner);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, nx_expr_parser_t *parser, void *scanner)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, parser, scanner);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, nx_expr_parser_t *parser, void *scanner)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              , parser, scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, parser, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, nx_expr_parser_t *parser, void *scanner)
{
  YYUSE (yyvaluep);
  YYUSE (parser);
  YYUSE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (nx_expr_parser_t *parser, void *scanner)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 110 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("empty block"); }
#line 1472 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 3:
#line 112 "expr-grammar.y" /* yacc.c:1646  */
    {
		       if ( parser->parse_expression == TRUE )
		       {
			   nx_expr_parser_error(parser, NULL, "Expression required, found a statement");
		       }
		       parser->statements = (yyvsp[0].statement_list);
		       log_debug("finished parsing statements"); 
		   }
#line 1485 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 4:
#line 121 "expr-grammar.y" /* yacc.c:1646  */
    {
		       if ( parser->parse_expression != TRUE )
		       {
			   nx_expr_parser_error(parser, NULL, "Statement required, expression found");
		       }
		       parser->expression = (yyvsp[0].expr);
		       log_debug("parsed expression");
                   }
#line 1498 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 5:
#line 132 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement_list) = nx_expr_statement_list_new(parser, (yyvsp[0].statement));
		    }
#line 1506 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 6:
#line 136 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement_list) = nx_expr_statement_list_add(parser, (yyvsp[-1].statement_list), (yyvsp[0].statement));
		    }
#line 1514 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 9:
#line 146 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = (yyvsp[0].statement);
			log_debug("statement: block");
		    }
#line 1523 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 10:
#line 151 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = nx_expr_statement_new_ifelse(parser, (yyvsp[-3].expr), (yyvsp[-2].statement), (yyvsp[0].statement));
			log_debug("if-else");
		    }
#line 1532 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 11:
#line 156 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = (yyvsp[-1].statement);
			log_debug("statement: procedure");
		    }
#line 1541 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 12:
#line 161 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = (yyvsp[-1].statement);
			log_debug("statement: assignment");
		    }
#line 1550 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 13:
#line 166 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = (yyvsp[-1].statement);
			log_debug("statement: regexpreplace");
		    }
#line 1559 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 14:
#line 171 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = (yyvsp[-1].statement);
			log_debug("statement: regexp");
		    }
#line 1568 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 15:
#line 176 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = NULL;
			log_debug("empty statement - single semicolon");
		    }
#line 1577 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 16:
#line 183 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = nx_expr_statement_new_ifelse(parser, (yyvsp[-1].expr), (yyvsp[0].statement), NULL);
			log_debug("if");
		    }
#line 1586 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 17:
#line 188 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = nx_expr_statement_new_ifelse(parser, (yyvsp[-3].expr), (yyvsp[-2].statement), (yyvsp[0].statement));
			log_debug("if-else");
		    }
#line 1595 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 18:
#line 196 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = NULL;
		    }
#line 1603 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 19:
#line 200 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = nx_expr_statement_new_block(parser, (yyvsp[-1].statement_list));
		    }
#line 1611 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 20:
#line 206 "expr-grammar.y" /* yacc.c:1646  */
    {
		       (yyval.statement) = nx_expr_statement_new_procedure(parser, (yyvsp[-3].string), (yyvsp[-1].exprs));
		       log_debug("procedure");
		   }
#line 1620 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 21:
#line 213 "expr-grammar.y" /* yacc.c:1646  */
    {
		       (yyval.expr) = nx_expr_new_function(parser, (yyvsp[-3].string), (yyvsp[-1].exprs));
		       log_debug("new function: %s", (yyvsp[-3].string));
		   }
#line 1629 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 22:
#line 220 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = nx_expr_statement_new_assignment(parser, (yyvsp[-2].expr), (yyvsp[0].expr));
			log_debug("assignment: left_value = expr");
		    }
#line 1638 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 23:
#line 227 "expr-grammar.y" /* yacc.c:1646  */
    {
			    (yyval.statement) = nx_expr_statement_new_regexpreplace(parser, (yyvsp[-2].expr), (yyvsp[0].expr)); 
			}
#line 1646 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 24:
#line 231 "expr-grammar.y" /* yacc.c:1646  */
    {
			    log_warn("useless use of negative pattern binding (!~) in regexp replacement");
			    (yyval.statement) = nx_expr_statement_new_regexpreplace(parser, (yyvsp[-2].expr), (yyvsp[0].expr)); 
			}
#line 1655 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 25:
#line 238 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.statement) = nx_expr_statement_new_regexp(parser, (yyvsp[-2].expr), (yyvsp[0].expr)); 
		    }
#line 1663 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 26:
#line 242 "expr-grammar.y" /* yacc.c:1646  */
    {
			log_warn("useless use of negative pattern binding (!~) in regexp match");
			(yyval.statement) = nx_expr_statement_new_regexp(parser, (yyvsp[-2].expr), (yyvsp[0].expr)); 
		    }
#line 1672 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 27:
#line 249 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.expr) = (yyvsp[0].expr);
			log_debug("left_value expr");
		    }
#line 1681 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 28:
#line 255 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.exprs) = NULL; }
#line 1687 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 29:
#line 256 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.exprs) = (yyvsp[0].exprs); }
#line 1693 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 30:
#line 260 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.exprs) = nx_expr_list_new(parser, (yyvsp[0].expr));
			//log_debug("expr");
		    }
#line 1702 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 31:
#line 265 "expr-grammar.y" /* yacc.c:1646  */
    {
			(yyval.exprs) = nx_expr_list_add(parser, (yyvsp[-2].exprs), (yyvsp[0].expr));
			//log_debug("exprs, expr");
		    }
#line 1711 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 32:
#line 271 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("literal"); (yyval.expr) = (yyvsp[0].expr); }
#line 1717 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 33:
#line 272 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_field(parser, (yyvsp[0].string)); }
#line 1723 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 34:
#line 273 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_captured(parser, (yyvsp[0].string)); }
#line 1729 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 35:
#line 274 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_unop(parser, TOKEN_MINUS, (yyvsp[0].expr)); }
#line 1735 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 36:
#line 275 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_MINUS, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1741 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 37:
#line 276 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_unop(parser, TOKEN_NOT, (yyvsp[0].expr)); }
#line 1747 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 38:
#line 277 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_unop(parser, TOKEN_DEFINED, (yyvsp[0].expr)); }
#line 1753 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 39:
#line 278 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_MUL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1759 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 40:
#line 279 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("division"); (yyval.expr) = nx_expr_new_binop(parser, TOKEN_DIV, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1765 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 41:
#line 280 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_MOD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1771 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 42:
#line 281 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_PLUS, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1777 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 43:
#line 282 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_BINAND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1783 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 44:
#line 283 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_BINXOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1789 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 45:
#line 284 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_BINOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1795 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 46:
#line 285 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_REGMATCH, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1801 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 47:
#line 286 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_NOTREGMATCH, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1807 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 48:
#line 287 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_REGMATCH, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1813 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 49:
#line 288 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_NOTREGMATCH, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1819 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 50:
#line 289 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_EQUAL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1825 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 51:
#line 290 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_NOTEQUAL, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1831 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 52:
#line 291 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_LESS, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1837 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 53:
#line 292 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_LE, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1843 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 54:
#line 293 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_GREATER, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1849 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 55:
#line 294 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_GE, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1855 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 56:
#line 295 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_AND, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1861 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 57:
#line 296 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_XOR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1867 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 58:
#line 297 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_binop(parser, TOKEN_OR, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 1873 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 59:
#line 298 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_inop(parser, (yyvsp[-2].expr), (yyvsp[0].exprs)); }
#line 1879 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 60:
#line 299 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_inop(parser, (yyvsp[-4].expr), (yyvsp[-1].exprs)); }
#line 1885 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 61:
#line 300 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_unop(parser, TOKEN_NOT, nx_expr_new_inop(parser, (yyvsp[-3].expr), (yyvsp[0].exprs))); }
#line 1891 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 62:
#line 301 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_unop(parser, TOKEN_NOT, nx_expr_new_inop(parser, (yyvsp[-5].expr), (yyvsp[-1].exprs))); }
#line 1897 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 63:
#line 302 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = (yyvsp[0].expr); }
#line 1903 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 64:
#line 303 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("( expr:%d )", (yyvsp[-1].expr)->type); (yyval.expr) = (yyvsp[-1].expr); }
#line 1909 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 65:
#line 306 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("regexpreplace"); (yyval.expr) = nx_expr_new_regexp(parser, (yyvsp[0].regexp)[0], (yyvsp[0].regexp)[1], (yyvsp[0].regexp)[2]); }
#line 1915 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 66:
#line 309 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("regexp"); (yyval.expr) = nx_expr_new_regexp(parser, (yyvsp[0].regexp)[0], NULL, (yyvsp[0].regexp)[2]); }
#line 1921 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 67:
#line 312 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_string(parser, (yyvsp[0].string)); }
#line 1927 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 68:
#line 313 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("regexp literal"); (yyval.expr) = nx_expr_new_regexp(parser, (yyvsp[0].regexp)[0], NULL, (yyvsp[0].regexp)[2]); }
#line 1933 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 69:
#line 314 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("boolean literal"); (yyval.expr) = nx_expr_new_boolean(parser, (yyvsp[0].bool)); }
#line 1939 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 70:
#line 315 "expr-grammar.y" /* yacc.c:1646  */
    { (yyval.expr) = nx_expr_new_undef(parser); }
#line 1945 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 71:
#line 316 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("integer literal: %s", (yyvsp[0].string)); (yyval.expr) = nx_expr_new_integer(parser, (yyvsp[0].string)); }
#line 1951 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 72:
#line 317 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("datetime literal: %s", (yyvsp[0].string)); (yyval.expr) = nx_expr_new_datetime(parser, (yyvsp[0].string)); }
#line 1957 "expr-grammar.c" /* yacc.c:1646  */
    break;

  case 73:
#line 318 "expr-grammar.y" /* yacc.c:1646  */
    { log_debug("ip4addr literal: %s", (yyvsp[0].string)); (yyval.expr) = nx_expr_new_ip4addr(parser, (yyvsp[0].string)); }
#line 1963 "expr-grammar.c" /* yacc.c:1646  */
    break;


#line 1967 "expr-grammar.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (parser, scanner, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (parser, scanner, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, parser, scanner);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, parser, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (parser, scanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, parser, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, parser, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 320 "expr-grammar.y" /* yacc.c:1906  */


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

