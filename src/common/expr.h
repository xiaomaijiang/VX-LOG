/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_EXPR_H
#define __NX_EXPR_H

#include "types.h"
#include "value.h"
#include "logdata.h"
#include "exception.h"

#define NX_EXPR_MAX_CAPTURED_FIELDS 100

typedef enum nx_expr_funcproc_type_t
{
    NX_EXPR_FUNCPROC_TYPE_GLOBAL = 1,
    NX_EXPR_FUNCPROC_TYPE_PRIVATE,
    NX_EXPR_FUNCPROC_TYPE_PUBLIC,
} nx_expr_funcproc_type_t;


typedef enum nx_expr_type_t
{
    NX_EXPR_TYPE_VALUE = 1,
    NX_EXPR_TYPE_FIELD,
    NX_EXPR_TYPE_CAPTURED,
    NX_EXPR_TYPE_UNOP,
    NX_EXPR_TYPE_BINOP,
    NX_EXPR_TYPE_FUNCTION,
    NX_EXPR_TYPE_INOP,
} nx_expr_type_t;


typedef enum nx_expr_regexp_modifier_t
{
    NX_EXPR_REGEXP_MODIFIER_NONE = 0,
    NX_EXPR_REGEXP_MODIFIER_MATCH_GLOBAL 	 = 1 << 1,
    NX_EXPR_REGEXP_MODIFIER_MATCH_DOTALL 	 = 1 << 2,
    NX_EXPR_REGEXP_MODIFIER_MATCH_MULTILINE 	 = 1 << 3,
    NX_EXPR_REGEXP_MODIFIER_MATCH_CASELESS 	 = 1 << 4,
} nx_expr_regexp_modifier_t;


typedef struct nx_expr_t nx_expr_t;
typedef struct nx_expr_parser_t nx_expr_parser_t;
typedef struct nx_expr_statement_t nx_expr_statement_t;
typedef struct nx_expr_statement_list_t nx_expr_statement_list_t;
NX_DLIST_HEAD(nx_expr_statement_list_t, nx_expr_statement_t);

typedef struct nx_expr_eval_ctx_t
{
//    apr_pool_t	*pool;
    nx_logdata_t	*logdata;
    nx_module_t		*module;	///< caller
    nx_module_input_t	*input;		///< source
    int			num_captured;	///< number of captured strings from the last regexp operation
    nx_string_t		**captured;
    boolean		dropped;	///< don't continue further
} nx_expr_eval_ctx_t;


#define NX_EXPR_VALUE_TYPE_VARARGS ((nx_value_type_t)-1)

typedef struct nx_expr_list_elem_t nx_expr_list_elem_t;
struct nx_expr_list_elem_t
{ 
    NX_DLIST_ENTRY(nx_expr_list_elem_t) link;
    nx_expr_t		*expr;
};
typedef struct nx_expr_list_t nx_expr_list_t;
NX_DLIST_HEAD(nx_expr_list_t, nx_expr_list_elem_t);


// Functions
typedef void (nx_expr_func_cb_t)(nx_expr_eval_ctx_t *eval_ctx,
				 nx_module_t *module,
				 nx_value_t *retval,
				 int32_t num_arg,
				 nx_value_t *args);
typedef struct nx_expr_func_t nx_expr_func_t;
typedef struct nx_expr_func_list_t nx_expr_func_list_t;
NX_DLIST_HEAD(nx_expr_func_list_t, nx_expr_func_t);
struct nx_expr_func_t
{
    NX_DLIST_ENTRY(nx_expr_func_t) link;
    nx_module_t 		*module;
    const char			*name;
    nx_expr_funcproc_type_t	type;
    nx_expr_func_cb_t		*cb;
    nx_value_type_t		rettype;
    int32_t 			num_arg;
    const char 			**arg_names;
    nx_value_type_t		*arg_types; ///< type is NX_EXPR_VALUE_TYPE_VARARGS for varargs
};


// Procedures
typedef void (nx_expr_proc_cb_t)(nx_expr_eval_ctx_t *eval_ctx,
				 nx_module_t *module,
				 nx_expr_list_t *args);
typedef struct nx_expr_proc_t nx_expr_proc_t;
typedef struct nx_expr_proc_list_t nx_expr_proc_list_t;
NX_DLIST_HEAD(nx_expr_proc_list_t, nx_expr_proc_t);
struct nx_expr_proc_t
{
    NX_DLIST_ENTRY(nx_expr_proc_t) link;
    nx_module_t 		*module;
    const char			*name;
    nx_expr_funcproc_type_t	type;
    nx_expr_proc_cb_t		*cb;
    int32_t 			num_arg;
    const char 			**arg_names;
    nx_value_type_t		*arg_types; ///< type is NX_EXPR_VALUE_TYPE_VARARGS for varargs
};



typedef struct nx_expr_decl_t
{
    const char			*file; ///< file where the expression is declared
    int32_t			line;  ///< line number of the declaration
    int32_t			pos;   ///< character position line of the declaration
    const char			*name; ///< name of the declared expression
} nx_expr_decl_t;



struct nx_expr_t
{
    nx_expr_type_t		type;
    nx_value_type_t		rettype;
    nx_expr_decl_t		decl;
    union
    {
	nx_value_t		value;
	const char		*field;
	int			captured;
	struct nx_expr_unop
	{
	    void		(*cb)(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *operand);
	    const nx_expr_t	*op;
	} unop;

	struct nx_exp_binop
	{
	    void		(*cb)(nx_expr_eval_ctx_t *eval_ctx, nx_value_t *retval, const nx_expr_t *left, const nx_expr_t *right);
	    const nx_expr_t	*left;
	    const nx_expr_t	*right;
	} binop;

	struct nx_exp_inop
	{
//	    void		(*cb)(nx_value_t *retval, const nx_expr_t *expr, const nx_expr_list_t *exprs);
	    const nx_expr_t	*expr;
	    const nx_expr_list_t *exprs;
	} inop;

	struct nx_exp_function
	{
	    nx_expr_func_cb_t	*cb;
	    nx_module_t 	*module;
	    nx_expr_list_t	*args;
	} function;
    };
};



typedef enum nx_expr_statement_type_t
{
    NX_EXPR_STATEMENT_TYPE_PROCEDURE = 1,
    NX_EXPR_STATEMENT_TYPE_IFELSE,
    NX_EXPR_STATEMENT_TYPE_ASSIGNMENT,
    NX_EXPR_STATEMENT_TYPE_BLOCK,
    NX_EXPR_STATEMENT_TYPE_REGEXPREPLACE,
    NX_EXPR_STATEMENT_TYPE_REGEXP,
} nx_expr_statement_type_t;



struct nx_expr_statement_t
{
    nx_expr_decl_t			decl;
    NX_DLIST_ENTRY(nx_expr_statement_t)	link;	///< linking field used during compilation
    nx_expr_statement_type_t   		type;
    union
    {
        struct nx_expr_statement_procedure	///< Procedure call: NX_EXPR_STATEMENT_TYPE_PROCEDURE_CALL
        {
	    nx_expr_proc_cb_t		*cb;
	    nx_module_t 		*module;
	    nx_expr_list_t		*args;
        } procedure;

        struct nx_expr_statement_ifelse		///< IF-ELSE
        {
            const nx_expr_t		*cond;
            const nx_expr_statement_t	*cond_true;
            const nx_expr_statement_t	*cond_false;
        } ifelse;

        struct nx_expr_statement_assignment	///< Assignment: NX_EXPR_STATEMENT_TYPE_ASSIGNMENT
        {
            const nx_expr_t		*lval;  ///< Left-value
            const nx_expr_t		*rval;  ///< Right-value
        } assignment;

        struct nx_expr_statement_block		///< NX_EXPR_STATEMENT_TYPE_BLOCK
        {
            const nx_expr_statement_list_t *statements;
        } block;

        struct nx_expr_statement_regexpreplace	///< Regexpreplace: NX_EXPR_STATEMENT_TYPE_REGEXPREPLACE
        {
            const nx_expr_t		*lval;  ///< Left-value (string)
            const nx_expr_t		*rval;  ///< Right-value (regexpreplace)
        } regexpreplace;

        struct nx_expr_statement_regexp		///< Regexpreplace: NX_EXPR_STATEMENT_TYPE_REGEXP
        {
            const nx_expr_t		*lval;  ///< Left-value (string)
            const nx_expr_t		*rval;  ///< Right-value (regexpreplace)
        } regexp;
    };
};



nx_expr_t *nx_expr_new_binop(nx_expr_parser_t *parser,
			     int token,
			     const nx_expr_t *left,
			     const nx_expr_t *right);
nx_expr_t *nx_expr_new_unop(nx_expr_parser_t *parser,
			    int token,
			    const nx_expr_t *op);
nx_expr_t *nx_expr_new_inop(nx_expr_parser_t *parser,
			    const nx_expr_t *expr,
			    const nx_expr_list_t *exprs);
nx_expr_t *nx_expr_new_field(nx_expr_parser_t *parser, const char *str);
nx_expr_t *nx_expr_new_captured(nx_expr_parser_t *parser, const char *str);
nx_expr_t *nx_expr_new_string(nx_expr_parser_t *parser, const char *str);
nx_expr_t *nx_expr_new_undef(nx_expr_parser_t *parser);
nx_expr_t *nx_expr_new_regexp(nx_expr_parser_t *parser,
			      const char *str,
			      const char *replacement,
			      const char *modifiers);
nx_expr_t *nx_expr_new_boolean(nx_expr_parser_t *parser, boolean value);
nx_expr_t *nx_expr_new_integer(nx_expr_parser_t *parser, const char *str);
nx_expr_t *nx_expr_new_datetime(nx_expr_parser_t *parser, const char *str);
nx_expr_t *nx_expr_new_ip4addr(nx_expr_parser_t *parser, const char *str);

nx_expr_t *nx_expr_new_function(nx_expr_parser_t *parser,
				const char *fname,
				nx_expr_list_t *args);
void nx_expr_eval_ctx_init(nx_expr_eval_ctx_t *eval_ctx,
			   nx_logdata_t *logdata,
			   nx_module_t *module,
			   nx_module_input_t *input);
void nx_expr_eval_ctx_destroy(nx_expr_eval_ctx_t *eval_ctx);
void nx_expr_evaluate(nx_expr_eval_ctx_t *eval_ctx,
		      nx_value_t *retval,
		      const nx_expr_t *expr);
nx_expr_func_t *nx_expr_func_lookup(nx_expr_func_list_t		*expr_funcs,
				    const nx_module_t 		*module,
				    const char 			*fname,
				    nx_expr_funcproc_type_t	type,
				    nx_value_type_t		rettype,
				    int32_t 			num_arg,
				    nx_value_type_t		*arg_types);
void nx_expr_func_register(apr_pool_t		*pool,
			   nx_expr_func_list_t	*expr_funcs,
			   nx_module_t		*module,
			   const char 		*fname,
			   nx_expr_funcproc_type_t	type,
			   nx_expr_func_cb_t 	*cb,
			   nx_value_type_t	rettype,
			   int32_t 		num_arg,
			   nx_value_type_t	*arg_types);
nx_expr_proc_t *nx_expr_proc_lookup(nx_expr_proc_list_t		*expr_procs,
				    const nx_module_t 		*module,
				    const char 			*fname,
				    nx_expr_funcproc_type_t	type,
				    int32_t 			num_arg,
				    nx_value_type_t		*arg_types);
void nx_expr_proc_register(apr_pool_t		*pool,
			   nx_expr_proc_list_t	*expr_procs,
			   nx_module_t		*module,
			   const char 		*fname,
			   nx_expr_funcproc_type_t	type,
			   nx_expr_proc_cb_t 	*cb,
			   int32_t 		num_arg,
			   nx_value_type_t	*arg_types);
nx_expr_list_t *nx_expr_list_new(nx_expr_parser_t *parser,
				 nx_expr_t *expr);
nx_expr_list_t *nx_expr_list_add(nx_expr_parser_t *parser,
				 nx_expr_list_t *args,
				 nx_expr_t *expr);
nx_expr_statement_t *nx_expr_statement_new_assignment(nx_expr_parser_t *parser,
						      const nx_expr_t *lval,
						      const nx_expr_t *rval);
nx_expr_statement_t *nx_expr_statement_new_block(nx_expr_parser_t *parser,
						 const nx_expr_statement_list_t *stmnts);
nx_expr_statement_t *nx_expr_statement_new_procedure(nx_expr_parser_t *parser,
						     const char *name,
						     nx_expr_list_t *args);
nx_expr_statement_t *nx_expr_statement_new_regexpreplace(nx_expr_parser_t *parser,
							 const nx_expr_t *lval,
							 const nx_expr_t *rval);
nx_expr_statement_t *nx_expr_statement_new_regexp(nx_expr_parser_t *parser,
						  const nx_expr_t *lval,
						  const nx_expr_t *rval);
nx_expr_statement_list_t *nx_expr_statement_list_new(nx_expr_parser_t *parser,
						     nx_expr_statement_t *stmnt);
nx_expr_statement_list_t *nx_expr_statement_list_add(nx_expr_parser_t *parser,
						     nx_expr_statement_list_t *list,
						     nx_expr_statement_t *stmnt);
void nx_expr_statement_list_execute(nx_expr_eval_ctx_t *eval_ctx,
				    const nx_expr_statement_list_t *stmnts);
void nx_expr_statement_execute(nx_expr_eval_ctx_t *eval_ctx,
			       const nx_expr_statement_t *stmnt);
nx_expr_statement_t *nx_expr_statement_new_ifelse(nx_expr_parser_t *parser,
						  const nx_expr_t *cond,
						  const nx_expr_statement_t *cond_true,
						  const nx_expr_statement_t *cond_false);
#endif	/* __NX_EXPR_H */
