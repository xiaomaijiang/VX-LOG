/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef WIN32

#include <unistd.h>
#include <apr_getopt.h>
#include <apr_signal.h>
#include <apr_file_info.h>
#include <apr_lib.h>
#include <apr_portable.h>
#include <grp.h>

#include "../common/error_debug.h"
#include "../common/event.h"
#include "../common/alloc.h"
#include "job.h"
#include "nxlog.h"
#include "modules.h"
#include "router.h"
#include "ctx.h"
#include "core.h"

#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif

#ifdef HAVE_SYS_CAPABILITY_H
# include <sys/capability.h>
#endif

#define NX_LOGMODULE NX_LOGMODULE_CORE

#define NX_MAX_PIDFILE_LOCK_TRIES 15

extern void nx_logger_disable_foreground();
static boolean _nxlog_initialized = FALSE;

static void print_usage()
{
    printf(PACKAGE "-" VERSION_STRING "\n"
	   " usage: "
	   " nxlog [-h/help] [-c/conf conffile] [-f] [-s/stop] [-v/verify]\n"
	   "   [-h] print help\n"
	   "   [-f] run in foreground, do not daemonize\n"
	   "   [-c conffile] specify an alternate config file\n"
	   "   [-r] reload configuration of a running instance\n"
	   "   [-s] send stop signal to a running nxlog\n"
	   "   [-v] verify configuration file syntax\n"
	   );
}



static void parse_cmd_line(nxlog_t *nxlog, int argc, const char * const *argv)
{
    const char *opt_arg;
    apr_status_t rv;
    apr_getopt_t *opt;
    int ch;

    static const apr_getopt_option_t options[] = {
	{ "help", 'h', 0, "print help" }, 
	{ "foreground", 'f', 0, "run in foreground" },
	{ "stop", 's', 0, "stop a running instance" },
	{ "reload", 'r', 0, "reload configuration of a running instance" },
	{ "conf", 'c', 1, "configuration file" }, 
	{ "verify", 'v', 0, "verify configuration file syntax" },
	{ NULL, 0, 1, NULL }, 
    };
	
    apr_getopt_init(&opt, nxlog->pool, argc, argv);
    while ( (rv = apr_getopt_long(opt, options, &ch, &opt_arg)) == APR_SUCCESS )
    {
	switch ( ch )
	{
	    case 'c':	/* configuration file */
		nxlog->cfgfile = apr_pstrdup(nxlog->pool, opt_arg);
		break;
	    case 'f':	/* foreground */
		nxlog->foreground = TRUE;
		break;
	    case 'h':	/* help */
		print_usage();
		exit(-1);
	    case 's':	/* stop */
		nxlog->do_stop = TRUE;
		break;
	    case 'r':	/* reload */
		nxlog->do_restart = TRUE;
		break;
	    case 'v':	/* verify */
		nxlog->verify_conf = TRUE;
		nxlog->ctx->ignoreerrors = FALSE;
		break;
	    default:
		print_usage();
		exit(-1);
	}
    }

    if ( nxlog->cfgfile == NULL )
    {
	nxlog->cfgfile = apr_pstrdup(nxlog->pool, NX_CONFIGFILE);
    }

    if ( (rv != APR_SUCCESS) && (rv != APR_EOF) )
    {
        throw(rv, "Could not parse options");
    }
}



static void nxlog_send_sighup(nxlog_t *nxlog)
{
    apr_proc_t proc;
    apr_status_t rv;
    pid_t pid = 0;
    char buf[20];
    apr_size_t nbytes;

    nxlog->pidfile = nx_cfg_get_value(nxlog->ctx->cfgtree, "pidfile");
    if ( nxlog->pidfile == NULL )
    {
	nxlog->pidfile = NX_PIDFILE;
    }

    CHECKERR_MSG(apr_file_open(&(nxlog->pidf), nxlog->pidfile, APR_READ, APR_OS_DEFAULT,
			       nxlog->pool), "Couldn't open pidfile %s", nxlog->pidfile);

    apr_file_close(nxlog->pidf);
    CHECKERR_MSG(apr_file_open(&(nxlog->pidf), nxlog->pidfile,
			       APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT,
			       nxlog->pool),  "Couldn't open pidfile %s", nxlog->pidfile);

    if ( (rv = apr_file_lock(nxlog->pidf, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK)) == APR_SUCCESS )
    { // stale pidfile?
	log_debug("pidfile %s is not locked", nxlog->pidfile);
	log_warn("removing stale pidfile %s", nxlog->pidfile);
	apr_file_close(nxlog->pidf);
	apr_file_remove(nxlog->pidfile, nxlog->pool);
	nxlog->pidf = NULL;
	return;
    }

    memset(buf, 0, sizeof(buf));
    nbytes = sizeof(buf);
    if ( (rv = apr_file_read(nxlog->pidf, buf, &nbytes)) != APR_SUCCESS )
    {
	if ( rv != APR_EOF )
	{
	    apr_file_remove(nxlog->pidfile, nxlog->pool);
	    nxlog->pidf = NULL;
	    throw(rv, "couldn't read from pidfile %s", nxlog->pidfile);
	}
    }
    apr_file_close(nxlog->pidf);
    nxlog->pidf = NULL;

    if ( (sscanf(buf, "%d\n", &pid) != 1) || (pid == 0) )
    {
	throw_msg("Invalid pidfile %s", nxlog->pidfile);
    }

    proc.pid = pid;
    CHECKERR_MSG(apr_proc_kill(&proc, SIGHUP), "Couldn't stop process %d", pid);

    log_info("SIGHUP sent to pid %d", pid);
}



static void nxlog_stop(nxlog_t *nxlog)
{
    apr_proc_t proc;
    apr_status_t rv;
    pid_t pid = 0;
    char buf[20];
    apr_size_t nbytes;
    int tries;

    nxlog->pidfile = nx_cfg_get_value(nxlog->ctx->cfgtree, "pidfile");
    if ( nxlog->pidfile == NULL )
    {
	nxlog->pidfile = NX_PIDFILE;
    }

    CHECKERR_MSG(apr_file_open(&(nxlog->pidf), nxlog->pidfile, APR_READ, APR_OS_DEFAULT,
			       nxlog->pool), "Couldn't open pidfile %s", nxlog->pidfile);
    apr_file_close(nxlog->pidf);
    CHECKERR_MSG(apr_file_open(&(nxlog->pidf), nxlog->pidfile,
			       APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT,
			       nxlog->pool), "Couldn't open pidfile %s", nxlog->pidfile);

    if ( (rv = apr_file_lock(nxlog->pidf, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK)) == APR_SUCCESS )
    {
	log_warn("removing stale pidfile %s", nxlog->pidfile);
	apr_file_close(nxlog->pidf);
	apr_file_remove(nxlog->pidfile, nxlog->pool);
	nxlog->pidf = NULL;
	return;
    }

    memset(buf, 0, sizeof(buf));
    nbytes = sizeof(buf);
    if ( (rv = apr_file_read(nxlog->pidf, buf, &nbytes)) != APR_SUCCESS )
    {
	if ( rv != APR_EOF )
	{
	    apr_file_remove(nxlog->pidfile, nxlog->pool);
	    apr_file_close(nxlog->pidf);
	    nxlog->pidf = NULL;
	    throw(rv, "couldn't read from pidfile %s", nxlog->pidfile);
	}
    }

    if ( (sscanf(buf, "%d\n", &pid) != 1) || (pid == 0) )
    {
	apr_file_close(nxlog->pidf);
	nxlog->pidf = NULL;
	throw_msg("Invalid pidfile %s", nxlog->pidfile);
    }

    proc.pid = pid;
    CHECKERR_MSG(apr_proc_kill(&proc, SIGTERM),  "Couldn't stop process %d", pid);

    for ( tries = 0; tries < NX_MAX_PIDFILE_LOCK_TRIES; tries++ )
    {
	if ( (rv = apr_file_lock(nxlog->pidf, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK)) == APR_SUCCESS )
	{
	    break;
	}
	else
	{ // couldn't lock pidfile
	    apr_sleep(APR_USEC_PER_SEC * 1);
	}
    }
    if ( rv != APR_SUCCESS )
    {
	throw(rv, "Couldn't stop process, %d did not exit after %d seconds",
	      pid, NX_MAX_PIDFILE_LOCK_TRIES);
    }

    log_info("process %d stopped", pid);
    apr_file_close(nxlog->pidf);
    nxlog->pidf = NULL;
}



void nxlog_remove_pidfile(nxlog_t *nxlog)
{
    apr_status_t rv;

    if ( nxlog->pidf != NULL )
    {
	if ( (rv = apr_file_lock(nxlog->pidf, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK)) != APR_SUCCESS )
	{
	    //log_aprerror(rv, "Couldn't lock pidfile %s", nxlog->pidfile);
	}
	else
	{
	    if ( (rv = apr_file_remove(nxlog->pidfile, nxlog->pool)) != APR_SUCCESS )
	    {
		log_aprerror(rv, "Couldn't remove pidfile %s", nxlog->pidfile);
	    }
	}
    	apr_file_close(nxlog->pidf);
	nxlog->pidf = NULL;
	log_debug("pidfile %s removed", nxlog->pidfile);
    }
}



static void nxlog_check_pidfile(nxlog_t *nxlog)
{
    apr_status_t rv;
    char buf[20];
    apr_size_t nbytes;
    int pid = 0;
    char *root = NULL;

    if ( nxlog->verify_conf == TRUE )
    { // no need to check pid
	return;
    }

    nxlog->pidfile = nx_cfg_get_value(nxlog->ctx->cfgtree, "pidfile");
    if ( nxlog->pidfile == NULL )
    {
	nxlog->pidfile = NX_PIDFILE;
    }
    else
    {
	nxlog->pidfile = apr_pstrdup(nxlog->pool, nxlog->pidfile);
    }

    if ( (nxlog->pidfile)[0] != '/' )
    {
	CHECKERR_MSG(apr_filepath_get(&root, 0, nxlog->pool),
		     "couldn't get work directory for relative pidfile");
	nxlog->pidfile = apr_psprintf(nxlog->pool, "%s/%s", root, nxlog->pidfile);
    }

    memset(buf, 0, sizeof(buf));
    if ( apr_file_open(&(nxlog->pidf), nxlog->pidfile, APR_READ, APR_OS_DEFAULT,
		       nxlog->pool) == APR_SUCCESS )
    {
	nbytes = sizeof(buf);
	rv = apr_file_read(nxlog->pidf, buf, &nbytes);
	switch ( rv )
	{
	    case APR_SUCCESS:
	    case APR_EOF:
		break;
	    default:
		throw(rv, "couldn't read from pidfile");
	}
	apr_file_close(nxlog->pidf);
	nxlog->pidf = NULL;

	if ( (sscanf(buf, "%d\n", &pid) != 1) || (pid == 0) )
	{
	    log_warn("removing invalid pidfile %s", nxlog->pidfile);
	    nxlog_remove_pidfile(nxlog);
	}
    }

    CHECKERR_MSG(apr_file_open(&(nxlog->pidf), nxlog->pidfile,
			       APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT,
			       nxlog->pool), "couldn't open pidfile %s", nxlog->pidfile);

    if ( (pid > 0) &&
	 ((rv = apr_file_lock(nxlog->pidf, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK)) != APR_SUCCESS) )
    {
	throw(rv, "Another instance is already running (pid %d)", pid);
    }

    apr_file_printf(nxlog->pidf, "%d\n", (int) getpid());
}



static void nxlog_create_pidfile(nxlog_t *nxlog)
{
    apr_status_t rv;
    int tries;

    if ( nxlog->pidf == NULL )
    {
	CHECKERR_MSG(apr_file_open(&(nxlog->pidf), nxlog->pidfile,
				   APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT,
				   nxlog->pool), "Couldn't create pidfile %s", nxlog->pidfile);
    }

    for ( tries = 0; tries < NX_MAX_PIDFILE_LOCK_TRIES; tries++ )
    {
	if ( (rv = apr_file_lock(nxlog->pidf, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK)) == APR_SUCCESS )
	{
	    break;
	}
	else
	{ // couldn't lock pidfile
	    apr_sleep(APR_USEC_PER_SEC * 1);
	}
    }
    if ( rv != APR_SUCCESS )
    {
	throw(rv, "Couldn't lock pidfile %s, another process is using it", nxlog->pidfile);
    }

    CHECKERR_MSG(apr_file_trunc(nxlog->pidf, (off_t) 0),
		 "Error truncating pidfile, %s", nxlog->pidfile);

    apr_file_printf(nxlog->pidf, "%d\n", (int) getpid());
    (void) apr_file_flush(nxlog->pidf);
    log_debug("pidfile %s created", nxlog->pidfile);
}



static void nxlog_daemonize(nxlog_t *nxlog)
{
    if ( nxlog->foreground == FALSE )
    {
	nxlog->pid = -1;
	ASSERT(nxlog->pidf != NULL);
	apr_file_unlock(nxlog->pidf);
	nxlog_remove_pidfile(nxlog);
	log_debug("daemonizing...");
	CHECKERR_MSG(apr_proc_detach(APR_PROC_DETACH_DAEMONIZE),
		     "Could not daemonize process");
	nx_logger_disable_foreground();
	nxlog->daemonized = TRUE;
	nxlog->pid = (int) getpid();
    }
    else
    {
	log_debug("running in foreground");
    }
}



static void nxlog_set_capabilities(nxlog_t *nxlog)
{
#ifdef HAVE_SYS_CAPABILITY_H
    cap_t cap;
    char capstr[100];
    const char *ptr;
    unsigned int i;
    cap_value_t capval;
    nx_module_t *module;

    ASSERT(nxlog != NULL);

    // set capabilities defined by each module

    log_debug("setting capabilities");

    cap = cap_init();

    for ( module = NX_DLIST_FIRST(nxlog->ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	if ( module->decl->capabilities != NULL )
	{
	    ptr = module->decl->capabilities;
	    while ( *ptr != '\0' )
	    {
		for ( ; *ptr == ' '; ptr++ ); //skip space
		for ( i = 0; (ptr[i] != '\0') && (ptr[i] != ' '); i++ )
		{
		    ASSERT(i < sizeof(capstr));
		    capstr[i] = ptr[i];
		}
		capstr[i] = '\0';
		capval = 0;
		if ( cap_from_name(capstr, &capval) != 0 )
		{
		    log_error("invalid capability '%s' ignored", capstr);
		}
		else
		{
		    log_debug("adding '%s' defined by %s", capstr,
			      module->dsoname);

		    if ( cap_set_flag(cap, CAP_PERMITTED, 1, &capval, CAP_SET) != 0 )
		    {
			log_errno("couldn't set flag for permitted capability '%s'", capstr);
		    }
		    if ( cap_set_flag(cap, CAP_EFFECTIVE, 1, &capval, CAP_SET) != 0 )
		    {
			log_errno("couldn't set flag for effective capability '%s'", capstr);
		    }
		}
		ptr += i;
	    };
	}
    }

    if ( cap_set_proc(cap) != 0 )
    {
	log_errno("cap_set_proc failed");
    }
    cap_free(cap);

    cap = cap_get_proc();
    log_debug("Now running with capabilities: %s", cap_to_text(cap, NULL));
    cap_free(cap);
#endif

/*
 This is commented out because if a new module is loaded during reload,
 new capabilities cannot be added. This weakens our security though :(
#ifdef HAVE_PRCTL
    // Now we remove our ability to modify capabilities
    if ( prctl(PR_SET_KEEPCAPS, 0, 0, 0, 0) < 0 )
    {
	log_errno("cannot remove PR_SET_KEEPCAPS");
    }
#endif
*/
}



static void nxlog_set_groups(nxlog_t *nxlog, apr_uid_t uid)
{
    gid_t grplist[100];
    int ngroups = sizeof(grplist);
    char *user;
    apr_status_t rv;

    if ( (rv = apr_uid_name_get(&user, uid, nxlog->pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "couldn't resolve uid %d to name", uid);
	return;
    }
#ifdef HAVE_GETGROUPLIST 
# ifdef HAVE_SETGROUPS
    if ( getgrouplist(user, getgid(), grplist, &ngroups) == -1 )
    {
	log_error("couldn't get group membership for user %s (uid: %d), too many groups?", user, uid);
	return;
    }
    if ( setgroups((size_t) ngroups, grplist) != 0 )
    {
	log_errno("couldn't get group membership for user %s (uid: %d), setgroups() failed",
		  user, uid);
	return;
    }
# else
    log_warn("additional group memberships couldn't be set because getgrouplist()"
	     "and setgroups() are not available on this platform");
# endif
#else
    log_warn("additional group memberships couldn't be set because getgrouplist()"
	     "and setgroups() are not available on this platform");
#endif

}



static void nxlog_drop_privs(nxlog_t *nxlog)
{
    const char *user;
    const char *group;
    apr_uid_t uid;
    apr_gid_t gid, tmpgid;
    size_t i;
    boolean digit = FALSE;
    boolean started_as_root = FALSE;

    ASSERT(nxlog != NULL);

    user = nx_cfg_get_value(nxlog->ctx->cfgtree, "user");
    group = nx_cfg_get_value(nxlog->ctx->cfgtree, "group");

    if ( (user == NULL) && (group == NULL) )
    {
	return;
    }

    if ( (getuid() == 0) && (getgid() == 0) )
    {
	started_as_root = TRUE;
    }

#ifndef APR_HAS_USER
    throw_msg("APR_HAS_USER not defined: libapr was compiled without user support, cannot drop privileges");
#endif

    // Parse group and drop to the specified GID
    if ( group != NULL )
    {
	digit = TRUE;
	for ( i = 0; i < strlen(group); i++ )
	{
	    if ( !apr_isdigit(group[i]) )
	    {
		digit = FALSE;
		break;
	    }
	}
	if ( digit == FALSE )
	{
	    CHECKERR_MSG(apr_gid_get(&gid, group, nxlog->pool),
			 "couldn't resolve groupname '%s' to gid", group);
	}
	else
	{
	    ASSERT(sscanf(group, "%d", &gid) == 1);
	}

	if ( (gid > 0) && (getgid() != gid) )
	{
	    if ( setgroups(0, NULL) != 0 )
	    {
		throw_errno("couldn't clear groups");
	    }
	    if ( setgid(gid) != 0 )
	    {
		throw_errno("couldn't drop to group id %d", gid);
	    }
	}
	else
	{
	    log_warn("already running as gid %d", gid);
	}
	log_debug("now running as group %s", group);
    }

#ifdef HAVE_PRCTL
    // On linux systems without this flag the kernel clears the capabilities upon uid change.
    if ( prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) < 0 )
    {
	log_errno("cannot set PR_SET_KEEPCAPS");
    }

    // enable core dumps on linux
    prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);

#endif
#ifdef HAVE_SETPFLAGS
    if ( setpflags(__PROC_PROTECT, 0) != 0 )
    {
	log_errno("setpflags failed");
    }
#endif

    // Parse user and drop to the specified UID
    if ( user != NULL )
    {
	digit = TRUE;
	for ( i = 0; i < strlen(user); i++ )
	{
	    if ( !apr_isdigit(user[i]) )
	    {
		digit = FALSE;
		break;
	    }
	}
	if ( digit == FALSE )
	{
	    CHECKERR_MSG(apr_uid_get(&uid, &tmpgid, user, nxlog->pool),
			 "couldn't resolve username '%s' to uid", user);
	}
	else
	{
	    ASSERT(sscanf(user, "%d", &uid) == 1);
	}
	if ( (uid > 0) && (getuid() != uid) )
	{
	    if ( (started_as_root == TRUE) && (nxlog->ctx->logfile != NULL) )
	    {
		apr_os_file_t fd;

		CHECKERR_MSG(apr_os_file_get(&fd, nxlog->ctx->logfile),
			     "failed to get fd for our logfile");

		// need to change ownership on our logfile, 
		// otherwise we cannot open it again on restart
		if ( fchown(fd, uid, gid) != 0 )
		{
		    log_errno("Couldn't change logfile ownership to %s:%s", user, group);
		}
	    }
	    nxlog_set_groups(nxlog, uid);

	    if ( setuid(uid) != 0 )
	    {
		throw_errno("couldn't drop to uid %d", uid);
	    }
	}
	else
	{
	    log_warn("already running as uid %d", uid);
	}
	log_debug("now running as user %s", user);
    }

    log_debug("running as uid: %d, euid: %d, gid: %d, egid: %d", 
	      getuid(), geteuid(), getgid(), getegid());
    
    if ( started_as_root == TRUE )
    {
	nxlog_set_capabilities(nxlog);
    }
}



//static void _got_sigterm() NORETURN;
static void _got_sigterm()
{
    static boolean got = FALSE;
    nxlog_t *nxlog;

    nxlog = nxlog_get();

    if ( got == TRUE )
    {
	//log_warn("double termination request received, aborting");
	abort();
    }
    nxlog->terminate_request = TRUE;

    got = TRUE;
}



static void _got_sighup()
{
    nxlog_t *nxlog;

    nxlog = nxlog_get();

    if ( _nxlog_initialized == FALSE )
    {
	return;
    }

    if ( nxlog->pid != (int) getpid() )
    {
	return;
    }

    nxlog->reload_request = TRUE;
}



static void _got_sigusr1()
{
    log_info("got SIGUSR1, dumping debug information");

    nxlog_dump_info();
}



static void _got_sigusr2()
{
    nx_ctx_t *ctx;

    log_info("got SIGUSR2, switching to debug loglevel");

    ctx = nx_ctx_get();
    ctx->loglevel = NX_LOGLEVEL_DEBUG;
}



static void nxlog_setup_signals()
{
    apr_signal(SIGTERM, _got_sigterm);
    apr_signal(SIGINT, _got_sigterm);
    apr_signal(SIGQUIT, _got_sigterm);
    apr_signal(SIGHUP, _got_sighup);
    apr_signal(SIGUSR1, _got_sigusr1);
    apr_signal(SIGUSR2, _got_sigusr2);

    // ignore sigpipe
    apr_signal(SIGPIPE, SIG_IGN);
}



static void nxlog_exit(int exitval)
{
    static boolean exited = FALSE;
    nxlog_t *nxlog;

    nxlog = nxlog_get();

    if ( exited == TRUE )
    {
	return;
    }
    exited = TRUE;

    if ( _nxlog_initialized == FALSE )
    {
	return;
    }

    if ( nxlog->pid != (int) getpid() )
    {
	return;
    }

    nxlog_shutdown(nxlog);
    nxlog_remove_pidfile(nxlog);

    if ( (nxlog->ctx != NULL) && (nxlog->ctx->nofreeonexit == FALSE) )
    {
	nx_ctx_free(nxlog->ctx);
	apr_pool_destroy(nxlog->pool);
	apr_terminate();
    }

    exit(exitval);
}



/**
 * The whole thing starts here
 */

int main(int argc, const char * const *argv, const char * const *env)
{
    nxlog_t nxlog;
    nx_exception_t e;

    nx_init(&argc, &argv, &env);

    nxlog_init(&nxlog);
    _nxlog_initialized = TRUE;

    //atexit(nxlog_exit_function);

    try
    {
	// read cmd line
	parse_cmd_line(&nxlog, argc, argv);

	// load and parse config
	nx_ctx_parse_cfg(nxlog.ctx, nxlog.cfgfile);

	if ( nxlog.ctx->rootdir != NULL )
	{ 
	    if ( chroot(nxlog.ctx->rootdir) != 0 )
	    {
		throw_errno("Couldn't change to RootDir '%s'", nxlog.ctx->rootdir);
	    }
	}

	if ( nxlog.ctx->spooldir != NULL )
	{ 
	    CHECKERR_MSG(apr_filepath_set(nxlog.ctx->spooldir, nxlog.pool),
			 "Couldn't change to SpoolDir '%s'", nxlog.ctx->spooldir);
	}

	// reload switch?
	if ( nxlog.do_restart == TRUE )
	{
	    nxlog_send_sighup(&nxlog);
	    nxlog_exit(0);
	}

	// stop switch?
	if ( nxlog.do_stop == TRUE )
	{
	    nxlog_stop(&nxlog);
	    nxlog_exit(0);
	}

	nx_ctx_init_logging(nxlog.ctx);

	// check pid
	nxlog_check_pidfile(&nxlog);

	// read config cache
	nx_config_cache_read();

	// load DSO and read and verify module config
	nx_ctx_config_modules(nxlog.ctx);

	if ( nxlog.verify_conf == TRUE )
	{
	    nx_ctx_init_routes(nxlog.ctx);
	    log_info("configuration OK");
	    nxlog_exit(0);
	}

	// daemonize unless running in foreground
	nxlog_daemonize(&nxlog);
	if ( nxlog.foreground == FALSE )
	{
	    // reopen log file because all descriptors were closed
	    nx_ctx_init_logging(nxlog.ctx);
	}

	if ( nxlog.ctx->spooldir != NULL )
	{ // need to change to spooldir again, because apr_proc_detach calls chdir("/")
	    CHECKERR_MSG(apr_filepath_set(nxlog.ctx->spooldir, nxlog.pool),
			 "Couldn't change to SpoolDir '%s'", nxlog.ctx->spooldir);
	}

	// setup signals
	nxlog_setup_signals();
	
	// initialize modules
	nx_ctx_init_modules(nxlog.ctx);

	nxlog_drop_privs(&nxlog);

	nxlog_create_pidfile(&nxlog);

	// initialize log routes
	nx_ctx_init_routes(nxlog.ctx);

	nx_ctx_init_jobs(nxlog.ctx);

	nx_ctx_restore_queues(nxlog.ctx);

	// setup threadpool
	nxlog_create_threads(&nxlog);

	nx_ctx_start_modules(nxlog.ctx);
    }
    catch(e)
    {
	log_exception(e);
	nxlog_exit(1);
    }

    log_info(PACKAGE"-"VERSION_STRING" started");

    // mainloop
    nxlog_mainloop(&nxlog, FALSE);

    nxlog_exit(0);
    return ( 0 );
}


#endif /* !WIN32 */
