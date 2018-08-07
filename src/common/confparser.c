/*
 * Config parser is based on routines from apache2
 * License: Apache License, Version 2.0
 */

#include <apr_lib.h>
#include <apr_file_io.h>
#include <apr_file_info.h>
#include <apr_fnmatch.h>


#include "../common/types.h"
#include "../common/cfgfile.h"
#include "../common/error_debug.h"
#include "../common/exception.h"

#define NX_LOGMODULE NX_LOGMODULE_CONFIG

#define MAX_STRING_LEN 8192

const nx_directive_t *nx_cfg_get_root(const nx_directive_t *curr)
{
    const nx_directive_t *directive; 

    ASSERT(curr != NULL);

    for ( directive = curr;
	  directive->parent != NULL;
	  directive = directive->parent );
    return ( directive->first_child );
}



static char *substring_conf(apr_pool_t *p,
			    const char *start,
			    long int len,
                            char quote)
{
    char *result;
    char *resp;
    long int i;

    ASSERT(len >= 0);

    result = apr_palloc(p, (apr_size_t) len + 2);
    resp = result;

    for ( i = 0; i < len; ++i )
    {
        if ( (start[i] == '\\') && 
	     ((start[i + 1] == '\\') || (quote && (start[i + 1] == quote))) )
	{
            *resp++ = start[++i];
	}
        else
	{
            *resp++ = start[i];
	}
    }

    *resp++ = '\0';
    return ( result );
}



static const char *nx_cfg_getword(apr_pool_t *p, const char **line)
{
    const char *str = *line, *strend;
    char *res;
    char quote;

    while ( (*str != '\0') && apr_isspace(*str) )
    {
        ++str;
    }

    if ( *str == '\0' )
    {
        *line = str;
        return "";
    }

    quote = *str;
    if ( (quote == '"') || (quote == '\'') )
    {
        strend = str + 1;
        while ( *strend && (*strend != quote) ) 
	{
            if ( (*strend == '\\') && (strend[1] != '\0') &&
		 ( (strend[1] == quote) || (strend[1] == '\\')) )
	    {
                strend += 2;
            }
            else
	    {
                ++strend;
            }
        }
	ASSERT(strend > str - 1);
        res = substring_conf(p, str + 1, strend - str - 1, quote);

        if ( *strend == quote )
	{
            ++strend;
	}
    }
    else
    {
        strend = str;
        while ( (*strend != '\0') && !apr_isspace(*strend) )
	{
            ++strend;
	}
        res = substring_conf(p, str, strend - str, 0);
    }

    while ( (*strend != '\0') && apr_isspace(*strend) )
    {
        ++strend;
    }
    *line = strend;

    return res;
}



static void add_node(nx_cfg_parser_ctx_t *ctx, nx_directive_t *newdir)
{
    ASSERT(newdir != NULL);
    ASSERT(ctx != NULL);

    if ( ctx->next_is_child == TRUE )
    {
	ASSERT(ctx->current->first_child == NULL);
	ctx->current->first_child = newdir;
	ASSERT(ctx->current != NULL);
	newdir->parent = ctx->current;
	ctx->next_is_child = FALSE;
    }
    else
    {
	ASSERT(ctx->current->next == NULL);
	ctx->current->next = newdir;
	if ( ctx->current->parent == NULL )
	{ // first element after root
	    ASSERT(ctx->current == ctx->root);
	    newdir->parent = ctx->root;
	}
	else
	{
	    newdir->parent = ctx->current->parent;
	}
    }
    ctx->current = newdir;
}



/*
 * This function returns TRUE if the keyword is a special block
 * that should not be parsed so that we can have anything (e.g. XML)
 * inside. This is a ugly to hardcode keywords here but module dso is not loaded
 * at this stage so we can't put this there.
 */

static boolean need_body_parsed(boolean current_state, const char *tag)
{
    typedef struct parse_keywords
    {
	const char *name;
	boolean state;
    } parse_keywords;

    static const parse_keywords keywords[] =
    {
	{ "<Exec>", TRUE },
	{ "</Exec>", FALSE },
	{ "<QueryXML>", TRUE }, // this is for im_msvistalog
	{ "</QueryXML>", FALSE }, // this is for im_msvistalog
	{ NULL, FALSE }
    };
    int i;

    if ( (current_state == FALSE) && (tag[0] != '<') )
    {
	return ( FALSE );
    }

    for ( i = 0; keywords[i].name != NULL; i++ )
    {
	if ( (keywords[i].state != current_state) && (strcasecmp(tag, keywords[i].name) == 0) )
	{
	    return ( keywords[i].state );
	}
    }

    return ( current_state );
}



/*
 * return the first element parsed
 */
static nx_directive_t *nx_cfg_parse_line(const char *l, int trimmedspace, nx_cfg_parser_ctx_t *ctx)
{
    const char *args;
    char *cmd_name;
    nx_directive_t *newdir = NULL;
    nx_cfgfile_t incfile;
    nx_cfg_parser_ctx_t cfg_ctx;
    nx_directive_t *retval = NULL;
    nx_exception_t e;
    boolean dont_parse = FALSE;
    boolean prev_dont_parse;

    ASSERT(ctx != NULL);

    if ( *l == '\0' )
    {
        return ( NULL );
    }

    args = l;

    cmd_name = nx_cfg_getword(ctx->pool, &args);
    if ( *cmd_name == '\0' )
    {
	return ( NULL );
    }

    prev_dont_parse = ctx->dont_parse;
    ctx->dont_parse = need_body_parsed(ctx->dont_parse, cmd_name);

    if ( (prev_dont_parse == TRUE) && (ctx->dont_parse == TRUE) )
    { // append 
	ASSERT(ctx->current != NULL);
	ASSERT(ctx->current->args != NULL);
	ctx->current->args = apr_pstrcat(ctx->pool, ctx->current->args, l, NULL);
	
	return ( NULL );
    }

    if ( *l == '#' )
    { // ignore comment lines 
        return ( NULL );
    }

    if ( strcasecmp(cmd_name, "include") == 0 )
    {
        const char *idx = NULL;
	const char * volatile fname = NULL;
	const char * volatile dirname = NULL;

	memset(&cfg_ctx, 0, sizeof(nx_cfg_parser_ctx_t));
	cfg_ctx.pool = ctx->pool;
	cfg_ctx.root = ctx->root;
	cfg_ctx.current = ctx->current;
	cfg_ctx.next_is_child = ctx->next_is_child;
	cfg_ctx.cfg = &incfile;
	cfg_ctx.defines = ctx->defines;

	if ( apr_fnmatch_test(args) != 0 )
	{ // wildcarded include
	    apr_dir_t *dir;

	    log_debug("Value specified for 'include' directive contains wildcards: '%s'", args);
	    idx = strrchr(args, NX_DIR_SEPARATOR[0]);
	    if ( idx == NULL )
	    { // relative path with filename only
		fname = args;
		dirname = "./";
		log_debug("A relative path was specified in 'include', checking directory entries under cwd");
	    }
	    else
	    {
		dirname = apr_pstrndup(ctx->pool, args, (apr_size_t) (idx - args));
		fname = idx + 1;
	    }

	    try
	    {
	        apr_finfo_t finfo;

		CHECKERR_MSG(apr_dir_open(&dir, dirname, ctx->pool),
			     "couldn't open directory '%s' specified in the 'include' directive", dirname);
		while ( apr_dir_read(&finfo, APR_FINFO_NAME | APR_FINFO_TYPE, dir) == APR_SUCCESS )
		{
		    log_debug("checking '%s' against wildcard '%s':", finfo.name, fname);
		    if ( finfo.filetype == APR_REG )
		    {
#ifdef WIN32
		        if ( apr_fnmatch(fname, finfo.name, APR_FNM_CASE_BLIND) == APR_SUCCESS )
#else
			if ( apr_fnmatch(fname, finfo.name, 0) == APR_SUCCESS )
#endif
			{
			    log_debug("'%s' matches include wildcard '%s'", finfo.name, fname);
			    incfile.name = apr_psprintf(ctx->pool, "%s"NX_DIR_SEPARATOR"%s", dirname, finfo.name);
			    
			    nx_cfg_open_file(&incfile, ctx->pool);
		    
			    if ( retval == NULL )
			    { // first element in the first include file
			        retval = nx_cfg_parse(&cfg_ctx);
			    }
			    else
			    {
			        nx_cfg_parse(&cfg_ctx);
			    }
			    ctx->current = cfg_ctx.current;
			    ctx->next_is_child = cfg_ctx.next_is_child;
			    ctx->defines = cfg_ctx.defines;
			}
		    }
		}
	    }
	    catch(e)
	    {
	        rethrow_msg(e, "Couldn't process 'include' directive at %s:%d",
			    ctx->cfg->name, ctx->cfg->line_num);
	    }
	}
	else
	{
	    incfile.name = apr_pstrdup(ctx->pool, args);

	    try
	    {
	        nx_cfg_open_file(&incfile, ctx->pool);
	    }
	    catch(e)
	    {
	        rethrow_msg(e, "Invalid 'include' directive at %s:%d",
			    ctx->cfg->name, ctx->cfg->line_num);
	    }
	    retval = nx_cfg_parse(&cfg_ctx);

	    ctx->current = cfg_ctx.current;
	    ctx->next_is_child = cfg_ctx.next_is_child;
	    ctx->defines = cfg_ctx.defines;
	}
	return ( retval );
    }

    newdir = apr_pcalloc(ctx->pool, sizeof(nx_directive_t));
    ASSERT(args > l);
	   
    newdir->argsstart = (int) (args - l) + trimmedspace;

    if ( cmd_name[1] != '/' )
    {
        char *lastc = cmd_name + strlen(cmd_name) - 1;
        if ( *lastc == '>' )
	{
            *lastc = '\0';
        }
        if ( (cmd_name[0] == '<') && (*args == '\0') )
	{
            args = ">";
        }
    }

    newdir->filename = ctx->cfg->name;
    newdir->line_num = ctx->cfg->line_num;
    newdir->directive = apr_pstrdup(ctx->pool, cmd_name);
    if ( ctx->dont_parse == TRUE )
    {
	newdir->args = apr_pstrcat(ctx->pool, "\n", args, NULL);
    }
    else
    {
	newdir->args = apr_pstrdup(ctx->pool, args);
    }

    if ( strcasecmp(cmd_name, "define") == 0 )
    {
	newdir->directive = nx_cfg_getword(ctx->pool, (const char **) &(newdir->args));
	if ( ctx->defines == NULL )
	{
	    ctx->defines = newdir;
	}
	else
	{
	    newdir->next = ctx->defines;
	    ctx->defines = newdir;
	}
	return ( NULL );
    }

    if ( cmd_name[0] == '<' )
    {
        if ( cmd_name[1] != '/' )
	{ // opening tag
	    if ( (newdir->args != NULL) && (strlen(newdir->args) > 0) )
	    { // remove trailing > from the argument
		size_t len = strlen(newdir->args);
		newdir->args[len - 1] = '\0';
	    }
	    apr_cpystrn(newdir->directive, cmd_name + 1, strlen(cmd_name)); // remove leading <

	    add_node(ctx, newdir);
	    ctx->next_is_child = TRUE;
	    if ( ctx->level >= NX_CFG_PARSER_MAX_DEPTH )
	    {
		throw_msg("config file tag nesting is too large at %s:%d",
			  ctx->cfg->name, ctx->cfg->line_num);
	    }
	    ctx->levels[ctx->level] = newdir;
	    (ctx->level)++;
	    return ( newdir );
        }
        else
	{ // closing tag </
	    char *bracket = cmd_name + strlen(cmd_name) - 1;

	    if ( ctx->level <= 0 )
	    {
		throw_msg("%s without matching <%s section at %s:%d",
			  cmd_name, cmd_name + 2, ctx->cfg->name, ctx->cfg->line_num);
	    }

	    if ( *bracket != '>' )
	    {
		throw_msg("%s> directive missing closing '>' at %s:%d",
			  cmd_name, ctx->cfg->name, ctx->cfg->line_num);
	    }

	    *bracket = '\0';

	    if ( strcasecmp(cmd_name + 2, (ctx->levels[ctx->level - 1])->directive) != 0)
	    {
		throw_msg("Expected </%s> but saw %s> at %s:%d",
			  (ctx->levels[ctx->level - 1])->directive,
			  cmd_name, ctx->cfg->name, ctx->cfg->line_num);
	    }

	    /* done with this section; move up a level */
	    if ( prev_dont_parse == FALSE )
	    {
		ctx->current = ctx->current->parent;
	    }
	    ASSERT(ctx->current != NULL);
	    for ( ; ctx->current->next != NULL; ctx->current = ctx->current->next );
	    (ctx->level)--;
	    ctx->levels[ctx->level] = NULL;
	    ctx->next_is_child = FALSE;
	}
    }
    else 
    {
	add_node(ctx, newdir);
	return ( newdir );
    }
    
    return ( NULL );
}



static void *cfg_getstr(void *buf, int bufsiz, void *param)
{
    apr_file_t *cfp = (apr_file_t *) param;
    apr_status_t rv;
    
    ASSERT(param != NULL);

    rv = apr_file_gets(buf, bufsiz, cfp);
    if ( rv == APR_SUCCESS )
    {
        return buf;
    }
    return NULL;
}



static int cfg_getch(void *param)
{
    char ch;
    apr_file_t *cfp = (apr_file_t *) param;
    if ( apr_file_getc(&ch, cfp) == APR_SUCCESS )
    {
        return ch;
    }

    return (int)EOF;
}



static apr_status_t cfg_close(void *param)
{
    apr_file_t *cfp = (apr_file_t *) param;
    return ( apr_file_close(cfp) );
}



/* Read one line from opened nx_configfile_t, strip LF, increase line number */
/* If custom handler does not define a getstr() function, read char by char */

static int nx_cfg_getline(char *buf,
			  int *trimmedspace,
			  int bufsize,
			  nx_cfgfile_t *cfp,
			  boolean dont_parse)
{
    char *src, *dst;
    char *cp;
    char *cbuf = buf;
    int cbufsize = bufsize;

    ASSERT(buf != NULL);
    ASSERT(cfp->getstr != NULL);
    ASSERT(trimmedspace != NULL);
    
    *trimmedspace = 0;
    for ( ; ; )
    {
	(cfp->line_num)++;
	if ( cfp->getstr(cbuf, cbufsize, cfp->param) == NULL )
	{
	    return 1;
	}
	/*
	 *  check for line continuation,
	 *  i.e. match [^\\]\\[\r]\n only
	 */
	if ( dont_parse == FALSE )
	{
	    cp = cbuf;
	    while ( (cp < cbuf+cbufsize) && (*cp != '\0') )
	    {
		cp++;
	    }
	    if ( (cp > cbuf) && (cp[-1] == APR_ASCII_LF) )
	    {
		cp--;
		if ( (cp > cbuf) && (cp[-1] == APR_ASCII_CR) )
		{
		    cp--;
		}
		if ( (cp > cbuf) && (cp[-1] == '\\') )
		{
		    cp--;
		    if (! ((cp > cbuf) && (cp[-1] == '\\')) )
		    {
			/*
			 * line continuation requested -
			 * then remove backslash and continue
			 */
			cbufsize -= (int) (cp - cbuf);
			cbuf = cp;
			continue;
		    }
		    else
		    {
			/*
			 * no real continuation because escaped -
			 * then just remove escape character
			 */
			for ( ; (cp < cbuf + cbufsize) && (*cp != '\0'); cp++ )
			{
			    cp[0] = cp[1];
			}
		    }
		}
	    }
	}
	break;
    }

    /*
     * Leading and trailing white space is eliminated completely
     */
    src = buf;
    if ( dont_parse == FALSE )
    {
	/* Zap leading whitespace by shifting */
	while ( apr_isspace(*src) )
	{
	    src++;
	    (*trimmedspace)++;
	}

	/* blast trailing whitespace */
	dst = &src[strlen(src)];
	while ( (--dst >= src) && apr_isspace(*dst) )
	{
	    *dst = '\0';
	}
    }

    if ( src != buf )
    {
	for ( dst = buf; (*dst++ = *src++) != '\0'; );
    }
    //log_debug("Read config: %s", buf);
    return 0;
}



void nx_cfg_open_file(nx_cfgfile_t *cfg, apr_pool_t *p)
{
    apr_file_t *file = NULL;
    apr_finfo_t finfo;

    ASSERT(cfg != NULL);
    ASSERT(cfg->name != NULL);

    if ( strcmp(cfg->name, "-") == 0 )
    { // open STDIN
	CHECKERR_MSG(apr_file_open_stdin(&file, p), "failed to open STDIN");
    }
    else
    {
	CHECKERR_MSG(apr_file_open(&file, cfg->name, APR_READ | APR_BUFFERED, APR_OS_DEFAULT, p),
		     "Failed to open config file %s", cfg->name);
    }
    //log_debug("Config file %s opened", cfg->name);

    CHECKERR_MSG(apr_file_info_get(&finfo, APR_FINFO_TYPE, file),
		 "Failed to open config file %s", cfg->name);

    cfg->param = file;
    cfg->getch = (int (*)(void *)) cfg_getch;
    cfg->getstr = (void *(*)(void *, int, void *)) cfg_getstr;
    cfg->close = (int (*)(void *)) cfg_close;
    cfg->line_num = 0;
}



static void nx_cfg_subst_defines(char *buf, int bufsize, nx_directive_t *defines)
{
    nx_directive_t *curr;
    char varname[bufsize];
    char tmpbuf[bufsize];
    char *ptr;

    ASSERT(buf != NULL);
    for ( curr = defines; curr != NULL; curr = curr->next )
    {
	ASSERT(bufsize - 3 >= (int) strlen(curr->directive));
	apr_snprintf(varname, (apr_size_t) bufsize, "%%%s%%", curr->directive); // %VAR%
	ptr = buf;
	while ( (ptr = strstr(ptr, varname)) != NULL )
	{
	    *ptr = '\0';
	    if ( apr_snprintf(tmpbuf, (apr_size_t) bufsize, "%s%s%s", buf, curr->args,
			      ptr + strlen(varname)) >= bufsize )
	    {
		throw_msg("config line is over the limit at %s:%d", curr->filename,
			  curr->line_num);
	    }
	    apr_cpystrn(buf, tmpbuf, (apr_size_t) bufsize);
	}
    }
}



/*
 * parse configuration tree and return the first element parsed
 */
nx_directive_t *nx_cfg_parse(nx_cfg_parser_ctx_t *ctx)
{
    char *l;
    nx_directive_t * volatile curr = NULL;
    nx_directive_t * volatile retval = NULL;
    nx_exception_t e;
    int trimmedspace = 0;

    ASSERT(ctx != NULL);

    l = apr_palloc(ctx->pool, MAX_STRING_LEN);

    while ( ! (nx_cfg_getline(l, &trimmedspace, MAX_STRING_LEN, ctx->cfg, ctx->dont_parse)) )
    {
	try
	{
	    nx_cfg_subst_defines(l, MAX_STRING_LEN, ctx->defines);
	    curr = nx_cfg_parse_line(l, trimmedspace, ctx);
	}
	catch(e)
	{
	    ctx->cfg->close(ctx->cfg->param);
            rethrow(e);
	}
	if ( retval == NULL )
	{
	    retval = curr;
	}
    }
    if ( ctx->level != 0 )
    {
	throw_msg("Expected </%s> at %s:%d", (ctx->levels[ctx->level - 1])->directive,
		  ctx->cfg->name, ctx->cfg->line_num);
    }
    ctx->cfg->close(ctx->cfg->param);

    return ( retval );
}



void nx_cfg_dump(const nx_directive_t *conftree, int level)
{
    const nx_directive_t *curr;
    int i;
    char buf[1024];

    curr = conftree;

    for ( i = 0; i < level; i++ )
    {
	ASSERT( i < (int)sizeof(buf));
	buf[i] = '\t';
	buf[i + 1] = '\0';
    }

    while ( curr != NULL )
    {
	apr_snprintf(buf + i, sizeof(buf) - (unsigned int)i, "%s %s", curr->directive,
		     curr->args == NULL ? "" : curr->args);

	printf("%s\n", buf);

	if ( curr->first_child != NULL )
	{
	    nx_cfg_dump(curr->first_child, level + 1);
	}
	curr = curr->next;
    }
}



void _nx_conf_error(const nx_directive_t *conf,
		    const char *file,
		    int line,
		    const char *func,
		    const char *fmt,	///< message formatter
		    ...)
{
    char buf[NX_LOGBUF_SIZE];
    va_list ap;
    nx_exception_t e;

    va_start(ap, fmt);
    apr_vsnprintf(buf, NX_LOGBUF_SIZE, fmt, ap);
    va_end(ap);

    if ( conf != NULL )
    {
	nx_exception_init(&e, NULL, file, line, func, APR_SUCCESS,
			  "%s at %s:%d", buf, conf->filename, conf->line_num);
    }
    else
    {
	nx_exception_init(&e, NULL, file, line, func, APR_SUCCESS,
			  "%s at ???", buf);
    }

    Throw(e);
}



const char *nx_cfg_get_value(const nx_directive_t *conf, const char *key)
{
    const nx_directive_t *curr = conf;

    ASSERT(conf != NULL);
    ASSERT(key != NULL);

    while ( curr != NULL )
    {
	if ( strcasecmp(curr->directive, key) == 0 )
	{
	    return ( curr->args );
	}
	curr = curr->next;
    }

    return ( NULL );
}



/**
 * Will only set the bool value if the key exists
 */

void nx_cfg_get_boolean(const nx_directive_t *conf,
			const char *key,
			boolean *value)
{
    const char *val;
    
    val = nx_cfg_get_value(conf, key);
    if ( val != NULL )
    {
	if ( strcasecmp(val, "true") == 0 )
	{
	    *value = TRUE;
	}
	else if ( strcasecmp(val, "yes") == 0 )
	{
	    *value = TRUE;
	}
	else if ( strcasecmp(val, "1") == 0 )
	{
	    *value = TRUE;
	}
	else if ( strcasecmp(val, "false") == 0 )
	{
	    *value = FALSE;
	}
	else if ( strcasecmp(val, "no") == 0 )
	{
	    *value = FALSE;
	}
	else if ( strcasecmp(val, "0") == 0 )
	{
	    *value = FALSE;
	}
	else
	{
	    nx_conf_error(conf, "Invalid boolean value '%s'", val);
	}
    }
}

    
