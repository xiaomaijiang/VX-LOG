#ifndef __NX_CFGFILE_H
#define __NX_CFGFILE_H

/*
 * Config parser is based on routines from apache2
 * License: Apache License, Version 2.0
 */


#define NX_CFG_PARSER_MAX_DEPTH 10

typedef struct nx_cfgfile_t
{
    int (*getch) (void *param);     /**< a getc()-like function */
    void *(*getstr) (void *buf, int bufsiz, void *param);
                                    /**< a fgets()-like function */
    int (*close) (void *param);     /**< a close handler function */
    void *param;                    /**< the argument passed to getch/getstr/close */
    const char *name;               /**< the filename / description */
    int line_num;   		    /**< current line number, starting at 1 */
} nx_cfgfile_t;



typedef struct nx_directive_t nx_directive_t;

/**
 * @brief Structure used to build the config tree.  
 *
 * The config tree only stores
 * the directives that will be active in the running server.  Directives
 * that contain other directions, such as <Directory ...> cause a sub-level
 * to be created, where the included directives are stored.  The closing
 * directive (</Directory>) is not stored in the tree.
 */
struct nx_directive_t 
{
    /** The current directive */
    char *directive;
    /** The arguments for the current directive, stored as a space separated list */
    char *args;
    /** The start of the args in bytes from the beginning of the line */
    int argsstart;

    /** The next directive node in the tree
     *  @defvar nx_directive_t *next */
    struct nx_directive_t *next;
    /** The first child node of this directive 
     *  @defvar nx_directive_t *first_child */
    struct nx_directive_t *first_child;
    /** The parent node of this directive 
     *  @defvar nx_directive_t *parent */
    struct nx_directive_t *parent;

    /** The name of the file this directive was found in */
    const char *filename;
    /** The line number the directive was on */
    int line_num;
};



typedef struct nx_cfg_parser_ctx_t
{
    apr_pool_t *pool;
    nx_directive_t *root;
    nx_directive_t *current;
    nx_directive_t *defines;
    int level;
    boolean next_is_child;
    nx_cfgfile_t *cfg;
    nx_directive_t *levels[NX_CFG_PARSER_MAX_DEPTH];
    boolean dont_parse; ///< set to true if line needs simply appended to current->orig
} nx_cfg_parser_ctx_t;



nx_directive_t *nx_cfg_parse(nx_cfg_parser_ctx_t *ctx);
void nx_cfg_open_file(nx_cfgfile_t *cfg, apr_pool_t *p);
void nx_cfg_dump(const nx_directive_t *conftree, int level);
const char *nx_cfg_get_value(const nx_directive_t *conf, const char *key);
void nx_cfg_get_boolean(const nx_directive_t *conf,
			const char *key,
			boolean *value);
const nx_directive_t *nx_cfg_get_root(const nx_directive_t *curr);

#define nx_conf_error(conf, fmt, args...) _nx_conf_error(conf, __FILE__, __LINE__, __FUNCTION__, fmt , ## args )


void _nx_conf_error(const nx_directive_t *conf,
		    const char *file,
		    int line,
		    const char *func,
		    const char	*fmt,	///< message formatter
		    ...) PRINTF_FORMAT(5,6) NORETURN;

#endif	/* __NX_CFGFILE_H */
