/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../common/types.h"
#include "../common/error_debug.h"
#include "../common/value.h"
#include "../core/nxlog.h"
#include "../common/exception.h"
#include "../common/alloc.h"
#include "../core/core.h"

#include <apr_lib.h>
#include <apr_pools.h>
#include <apr_getopt.h>
#include <apr_file_io.h>
#include <apr_poll.h>

#define CONNECTION_CNT 1
#define MAX_THREADS 20

#define NX_LOGMODULE NX_LOGMODULE_CORE

#include "testinput.h"

nxlog_t nxlog;

typedef struct conn_thd
{
    const char *host;
    apr_port_t port;
    apr_thread_t *thd_id;
    apr_socket_t *sockets[CONNECTION_CNT];
    int outpos;
    apr_size_t offs;
    int64_t evcnt;
    int64_t conncnt;
} conn_thd;

int testinputsize = 0;
testinput_t *testinput;


static void write_cb(conn_thd *thd_data, apr_socket_t *sock)
{
    apr_size_t nbytes;
    apr_status_t rv;

    for ( ; ; )
    {
	ASSERT(testinput[thd_data->outpos].len > thd_data->offs);
	nbytes = testinput[thd_data->outpos].len - thd_data->offs;
	//printf("%d %d\n", strlen(testinput[thd_data->outpos].str), testinput[thd_data->outpos].len);
	//ASSERT(strlen(testinput[thd_data->outpos].str) == testinput[thd_data->outpos].len);
	rv = apr_socket_send(sock, testinput[thd_data->outpos].str, &nbytes);
	if ( APR_STATUS_IS_EAGAIN(rv) )
	{
	    if ( nbytes == 0 )
	    {
		log_error("socket buffer full?");
		apr_sleep(100);
		//log_info("wrote %ld bytes %s", nbytes, rv == APR_SUCCESS ? "SUCCESS" : "EAGAIN");
	    }
	}
	else if ( rv == APR_SUCCESS )
	{
	    thd_data->offs += nbytes;
	    if ( thd_data->offs >= testinput[thd_data->outpos].len )
	    {
		(thd_data->evcnt)++;
		thd_data->offs = 0;
		(thd_data->outpos)++;
		if ( thd_data->outpos >= testinputsize )
		{
		    thd_data->outpos = 0;
		};
	    }
	    break;
	}
	else
	{
	    throw(rv, "socket send failed");
	}
    }
}



static void* APR_THREAD_FUNC conn_thread(apr_thread_t *thd UNUSED, void *data)
{
    apr_pollset_t *pollset;
    nx_exception_t e;
    apr_pool_t *pool;
    conn_thd *thd_data = (conn_thd *) data;
    apr_sockaddr_t *sa;
    int i;
    apr_status_t rv;
    apr_int32_t num;
    const apr_pollfd_t *ret_pfd;
    boolean done = FALSE;
    apr_pollfd_t pfd;

    ASSERT(thd != NULL);
    try
    {
	pool = nx_pool_create_core(NULL);
	CHECKERR(apr_pollset_create(&pollset, 100, pool, 0));
	CHECKERR(apr_sockaddr_info_get(&sa, thd_data->host, APR_INET, thd_data->port, 0, pool));
	for ( i = 0; i < CONNECTION_CNT; i++ )
	{
	    CHECKERR(apr_socket_create(&(thd_data->sockets[i]), sa->family,
				       SOCK_STREAM, APR_PROTO_TCP, pool));
	    CHECKERR(apr_socket_opt_set(thd_data->sockets[i], APR_SO_NONBLOCK, 1));
	    CHECKERR(apr_socket_timeout_set(thd_data->sockets[i], APR_USEC_PER_SEC * 30));
	    log_info("connecting to %s:%d", thd_data->host, thd_data->port);
	    rv = apr_socket_connect(thd_data->sockets[i], sa);
	    if ( rv == APR_SUCCESS )
	    {
		(thd_data->conncnt)++;
	    }
	    else
	    {
		throw(rv, "connection failed");
	    }
	    CHECKERR(apr_socket_opt_set(thd_data->sockets[i], APR_SO_NONBLOCK, 0));
	    CHECKERR(apr_socket_timeout_set(thd_data->sockets[i], APR_USEC_PER_SEC * 60));
	    //CHECKERR(apr_socket_timeout_set(thd_data->sockets[i], 0));
	    log_info("connected to %s:%d", thd_data->host, thd_data->port);
	    
	    pfd.desc.s = thd_data->sockets[i];
	    pfd.desc_type = APR_POLL_SOCKET;
	    pfd.reqevents = APR_POLLHUP | APR_POLLOUT | APR_POLLIN;
	    pfd.p = pool;
	    CHECKERR(apr_pollset_add(pollset, &pfd));
	}

	while ( done != TRUE )
	{
	    rv = apr_pollset_poll(pollset, APR_USEC_PER_SEC * 1, &num, &ret_pfd);
	    for ( i = 0; i < num; i++ )
	    {
		if ( rv == APR_SUCCESS )
		{
		    if ( ((ret_pfd[i].reqevents & ret_pfd[i].rtnevents) & (APR_POLLIN | APR_POLLPRI)) != 0 )
		    { // can read
			log_info("got read event, thread exiting");
			done = TRUE;
			break;
		    }
		    else if ( ((ret_pfd[i].reqevents & ret_pfd[i].rtnevents) & APR_POLLHUP) != 0 )
		    { // disconnect
			log_info("got disconnect, thread exiting");
			done = TRUE;
			break;
		    }
		    else if ( ((ret_pfd[i].reqevents & ret_pfd[i].rtnevents) & APR_POLLOUT) != 0 )
		    { // can write
			write_cb(thd_data, thd_data->sockets[i]);
		    }
		    else
		    {
			log_error("other event");
			done = TRUE;
			break;
		    }
		}
		else if ( APR_STATUS_IS_TIMEUP(rv) )
		{
		    log_info("no poll events, pollset_poll timed out");
		}
		else if ( APR_STATUS_IS_EINTR(rv) )
		{
		    log_info("apr_pollset_poll was interrupted");
		}
		else
		{
		    throw(rv, "poll failed");
		}
	    }
	}
    }
    catch(e)
    {
	//log_exception(NX_LOGMODULE, &e, NULL);
	apr_thread_exit(thd, e.code);
    }
    apr_thread_exit(thd, APR_SUCCESS);

    return ( NULL );
}



int getrand()
{
    int retval = -1;

    retval = rand();
    retval %= 60;

    return ( retval );
}


int main(int argc, const char * const *argv, const char * const *env)
{
    apr_status_t rv;
    apr_pool_t *pool;
    int thd_cnt = 0;
    int i, j;
    conn_thd *threads[MAX_THREADS];
    conn_thd *thd_data;
    apr_time_t started;
    int64_t evcnt = 0;
    int64_t conncnt = 0;
    int64_t sec;
    int hostnum;
    char *numstart;

    nx_init(&argc, &argv, &env);

    pool = nx_pool_create_child(NULL);

    if ( argc <= 1 )
    {
	log_error("argument required");
	exit(-1);
    }


    testinputsize = _testinputsize * 30;
    testinput = apr_pcalloc(pool, sizeof(testinput_t) * (apr_size_t) testinputsize);

    for ( i = 0; i < 30; i++ )
    {
	for ( j = 0; j < _testinputsize; j++ )
	{
	    testinput[i * _testinputsize + j].str = apr_pstrdup(pool, _testinput[j].str);
	    testinput[i * _testinputsize + j].len = _testinput[j].len;
	    hostnum = getrand();
	    numstart = strstr(testinput[i * _testinputsize + j].str, "  .localdomain");
	    ASSERT(numstart != NULL);
	    numstart[0] = (char) ((hostnum / 10) + '0');
	    numstart[1] = (char) ((hostnum % 10) + '0');
	}
    }

    started = apr_time_now();
    for ( i = 1; i < argc; i++ )
    {
	thd_data = apr_pcalloc(pool, sizeof(conn_thd));
	threads[thd_cnt] = thd_data;
	for ( j = 0; argv[i][j] != '\0'; j++ )
	{
	    ASSERT(j < MAX_THREADS);
	    if ( argv[i][j] == ':' )
	    {
		thd_data->host = apr_pstrndup(pool, argv[i], (apr_size_t) j);
		j++;
		if ( argv[i][j] != '\0' )
		{
		    thd_data->port = (apr_port_t) atoi(argv[i] + j);
		}
	    }
	}
	if ( thd_data->port == 0 )
	{
	    log_error("invalid port");
	    break;
	}
	thd_cnt++;
	nx_thread_create(&(thd_data->thd_id), NULL, conn_thread, thd_data, pool);
    }

    for ( i = 0; i < thd_cnt; i++ )
    {
	CHECKERR(apr_thread_join(&rv, threads[i]->thd_id));
	if ( rv != APR_SUCCESS )
	{
	    log_aprerror(rv, "Test failed.");
	}
	evcnt += threads[i]->evcnt;
	conncnt += threads[i]->conncnt;
    }
    
    sec = (apr_time_now() - started) / APR_USEC_PER_SEC;
    log_info("Connections: %ld", conncnt);
    log_info("Syslog lines transferred: %ld", evcnt);
    log_info("Time elapsed: %lds", sec);
    if ( sec > 0 )
    {
	log_info("EPS: %ld", evcnt / sec);
    }

    return ( 0 );
}
