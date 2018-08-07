/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_portable.h>

#include "../core/nxlog.h"
#include "error_debug.h"
#include "ssl.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static int nx_ssl_ssl_ctx_idx = -1;
static int nx_ssl_verify_result_idx = -1;

static void nx_ssl_locking_callback(int mode, int id, char *file UNUSED, int line UNUSED)
{
    nxlog_t *nxlog;
    
    //log_debug("ssl_locking_callback()");

    nxlog = nxlog_get();

    if ( nxlog->openssl_locks != NULL )
    {
	if ( mode & CRYPTO_LOCK )
	{
	    CHECKERR(apr_thread_mutex_lock(nxlog->openssl_locks[id]));
	}
	else
	{
	    CHECKERR(apr_thread_mutex_unlock(nxlog->openssl_locks[id]));
	}
    }
}


    
static unsigned long nx_ssl_thread_id()
{
    unsigned long ret;
    
    ret = (unsigned long) apr_os_thread_current();

    return ( ret );
}



static void nx_ssl_init_locking(nxlog_t *nxlog)
{
    int i;

    if ( nxlog->openssl_locks == NULL )
    {
	nxlog->openssl_locks = apr_pcalloc(nxlog->pool, ((unsigned int) CRYPTO_num_locks()) * sizeof(apr_thread_mutex_t *));

	for ( i = 0; i < CRYPTO_num_locks(); i++ )
	{
	    CHECKERR(apr_thread_mutex_create(&(nxlog->openssl_locks[i]),
					     APR_THREAD_MUTEX_UNNESTED, nxlog->pool));
	}
    }
    
    CRYPTO_set_id_callback((unsigned long (*)())nx_ssl_thread_id);
    CRYPTO_set_locking_callback((void (*)())nx_ssl_locking_callback);
}



void nx_ssl_error(boolean printerror,
		  const char *fmt,
		  ...)

{
    const char *str;
    unsigned long errcode;
    const char *libstr;
    const char *funcstr;
    nx_loglevel_t loglevel = NX_LOGLEVEL_ERROR;
    nx_ctx_t *ctx;
    char errmsg[512];

    errmsg[0] = '\0';
    if ( fmt != NULL )
    {
	va_list ap;

	va_start(ap, fmt);
	apr_vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
	va_end(ap);
    }

    ctx = nx_ctx_get();
    if ( ctx != NULL )
    {
	loglevel = ctx->loglevel;
    }

    if ( errno == 0 )
    {
	printerror = 0;
    }
    if ( printerror > 0 )
    {
	throw_errno("SSL error, %s", errmsg);
    }

    while ( ((unsigned long) (errcode = ERR_get_error())) > 0 )
    {
	str = ERR_reason_error_string(errcode);
	funcstr = ERR_func_error_string(errcode);
	libstr = ERR_lib_error_string(errcode);
	if ( libstr == NULL )
	{
	    libstr = "unknown";
	}
	if ( funcstr == NULL )
	{
	    funcstr = "unknown";
	}

	if ( str == NULL )
	{
	    if ( errcode == 1 )
	    {
		//log_error("ssl lib usage error");
	    }
	    else
	    {
		throw_msg("unknown SSL error, code: %ld, lib: %s, func: %s",
			  (long int) errcode, libstr, funcstr);
	    }
	}
	else
	{
	    if ( loglevel == NX_LOGLEVEL_DEBUG )
	    {
		throw_msg("SSL error, %s, %s [lib:%s func:%s]", errmsg, str,
			  libstr, funcstr);
	    }
	    else
	    {
		throw_msg("SSL error, %s, %s,", errmsg, str);
	    }
	}
    }
    throw_msg("SSL error: %s", errmsg);
}



int nx_ssl_check_io_error(SSL *ssl, int retval)
{
    int errcode;
    void *verify_result;

    errcode = SSL_get_error(ssl, retval);
    verify_result = SSL_get_ex_data(ssl, nx_ssl_verify_result_idx);

    if ( verify_result != NULL )
    { // cert verification failed;
	throw_msg("SSL certificate verification failed: %s (err: %d)",
		  X509_verify_cert_error_string((int) verify_result),
		  (int) verify_result);
    }

    switch ( errcode  )
    {
	case SSL_ERROR_ZERO_RETURN:
	    //log_info("SSL connection closed");
	    break;
	case SSL_ERROR_WANT_READ:
	    //log_debug("SSL want read");
	    break;
	case SSL_ERROR_WANT_WRITE:
	    //log_debug("SSL want write");
	    break;
	case SSL_ERROR_WANT_CONNECT:
	    //log_debug("SSL want connect");
	    break;
	case SSL_ERROR_WANT_ACCEPT:
	    //log_debug("SSL want accept");
	    break;
	case SSL_ERROR_WANT_X509_LOOKUP:
	    //log_debug("SSL want x509 lookup");
	    break;
	case SSL_ERROR_SYSCALL:
	    if ( retval == 0 )
	    {
		//log_debug("EOF during SSL read");
		return ( SSL_ERROR_ZERO_RETURN );
	    }
	    else
	    {   // openssl does something bad because we get EBADF when the connection
		// is reset during handshake, so we treat this as connection EOF
		if ( (errno == 0) || (errno == EBADF) || (errno == EOF) || (errno == EPIPE) )
		{
		    throw(APR_EOF, "remote ssl socket was reset? (SSL_ERROR_SYSCALL with errno=%d)", errno);
		}
		nx_ssl_error(retval == -1, "SSL_ERROR_SYSCALL: retval %d, errno: %d",
			     retval, errno);
	    }
	    break;
	case SSL_ERROR_SSL:
	    // openssl does something bad because we get EBADF when the connection
	    // is reset during read/write, so we treat this as connection EOF
	    if ( (errno == EBADF) || (errno == EOF) || (errno == EPIPE) )
	    {
		throw(APR_EOF, "remote ssl socket was reset? (SSL_ERROR_SSL with errno=%d)", errno);
	    }
	    nx_ssl_error(retval == -1, "SSL_ERROR_SSL: retval %d", retval);
	    break;
	default:
	    nx_ssl_error(FALSE, "unknown SSL error (errorcode: %d)", retval);
	    break;
    }

    return ( errcode );
}



/**
 * initialize ssl context
 */

void nx_ssl_ctx_init(nx_ssl_ctx_t *ctx, apr_pool_t *pool)
{
    BIO *cert_bio = NULL;
    BIO *key_bio = NULL;
    nxlog_t *nxlog;
    
    //log_debug("SSL init");

    nxlog = nxlog_get();

    nx_lock();
     nx_ssl_init_locking(nxlog);
     SSL_library_init();
     SSL_load_error_strings();
     ERR_load_crypto_strings();
     OpenSSL_add_all_algorithms();

     if ( nx_ssl_ssl_ctx_idx == -1 )
     {
	 nx_ssl_ssl_ctx_idx = SSL_get_ex_new_index(0, (void *) "ssl_ctx_idx", 0, 0, 0);
     }
     if ( nx_ssl_verify_result_idx == -1 )
     {
	 nx_ssl_verify_result_idx = SSL_get_ex_new_index(0, (void *) "verify_result_idx", 0, 0, 0);
     }

    nx_unlock();

    ctx->pool = pool;
    // FIXME: use pool for allocations

    if ( (ctx->certfile != NULL) && (ctx->certkeyfile != NULL) )
    {
	cert_bio = BIO_new_file(ctx->certfile, "r");
	if ( cert_bio == NULL )
	{
	    nx_ssl_error(TRUE, "Failed to open certfile: %s", ctx->certfile);
	}

	key_bio = BIO_new_file(ctx->certkeyfile, "r");
	if ( key_bio == NULL )
	{
	    nx_ssl_error(TRUE, "Failed to open certkey: %s", ctx->certkeyfile);
	}

	ctx->cert = PEM_read_bio_X509(cert_bio, NULL, 0, (void *) ctx->keypass);
	if ( ctx->cert == NULL )
	{
	    BIO_free(cert_bio);
	    nx_ssl_error(FALSE, "couldn't read cert");
	}

	ctx->key = PEM_read_bio_PrivateKey(key_bio, NULL, 0, (void *) ctx->keypass);
	if ( ctx->key == NULL )
	{
	    BIO_free(cert_bio);
	    BIO_free(key_bio);
	    nx_ssl_error(FALSE, "invalid certificate key passphrase [%s], couldn't decrypt key",
			 ctx->keypass);
	}
	BIO_free(key_bio);
	BIO_free(cert_bio);
    }
}



static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    int err;
    int	retval = preverify_ok;
    SSL *ssl;
    int verify_result;
    nx_ssl_ctx_t *ssl_ctx;

    ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    ASSERT(ssl != NULL);

    ssl_ctx = SSL_get_ex_data(ssl, nx_ssl_ssl_ctx_idx);
    ASSERT(ssl_ctx != NULL);

    verify_result = 0;
    log_debug("verify callback (ok: %d)", preverify_ok);
    if ( !preverify_ok )
    {
	err = X509_STORE_CTX_get_error(ctx);
	log_debug("preverification returned non-OK: %s", X509_verify_cert_error_string(err));

	switch ( err )
	{
	    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
	    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
		if ( ssl_ctx->allow_untrusted == TRUE )
		{
		    retval = 1;
		}
		else
		{
		    retval = 0;
		    verify_result = err;
		}
		break;
	    case X509_V_ERR_UNABLE_TO_GET_CRL:
		log_warn("CRL verification requested but no CRLs found");
		retval = 1;
		break;
	    default:
		retval = 0;
		verify_result = err;
	}
    }

    SSL_set_ex_data(ssl, nx_ssl_verify_result_idx, (void *) verify_result);

    return ( retval );
}



/**
 * return NULL on error, an SSL structure on success
 */
 
SSL *nx_ssl_from_socket(nx_ssl_ctx_t *ctx, apr_socket_t *sock)
{
    SSL_CTX *ssl_ctx;
    const SSL_METHOD *meth;
    SSL *ssl;
    BIO *bio;
    apr_os_sock_t fd;
    int verify_mode = SSL_VERIFY_NONE;
    unsigned long verify_flags = X509_V_FLAG_POLICY_CHECK;

    CHECKERR_MSG(apr_os_sock_get(&fd, sock), "couldn't get fd of accepted socket");

    bio = BIO_new_socket(fd, BIO_CLOSE);
    if ( bio == NULL )
    {
	throw_msg("error allocating BIO from socket");
    }

    meth = SSLv23_method();
    if ( meth == NULL )
    {
	nx_ssl_error(FALSE, "failed to init SSLv23");
    }

    ssl_ctx = SSL_CTX_new(meth);
    if ( ssl_ctx == NULL )
    {
	nx_ssl_error(FALSE, "failed to create ssl_ctx");
    }

    if ( (ctx->cafile != NULL) || (ctx->cadir != NULL) )
    {
	if ( SSL_CTX_load_verify_locations(ssl_ctx, ctx->cafile, ctx->cadir) != 1 )
	{
	    nx_ssl_error(FALSE, "failed to load ca cert from '%s'",
			 ctx->cafile == NULL ? ctx->cadir : ctx->cafile);
	}
    }

    if ( (ctx->crlfile != NULL) || (ctx->crldir != NULL) )
    {
	verify_flags |= X509_V_FLAG_CRL_CHECK_ALL | X509_V_FLAG_CRL_CHECK;
	if ( SSL_CTX_load_verify_locations(ssl_ctx, ctx->crlfile, ctx->crldir) != 1 )
	{
	    nx_ssl_error(FALSE, "failed to load crl from '%s'",
			 ctx->crlfile == NULL ? ctx->crldir : ctx->crlfile);
	}
    }
    X509_VERIFY_PARAM_set_flags(ssl_ctx->param, verify_flags);

    if ( ctx->allow_untrusted != TRUE )
    {
	verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
    }
    if ( ctx->require_cert == TRUE )
    {
	verify_mode |= SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }

    SSL_CTX_set_verify(ssl_ctx, verify_mode, verify_callback);

    if ( ctx->cert != NULL )
    {
	if ( SSL_CTX_use_certificate(ssl_ctx, ctx->cert) != 1 )
	{
	    nx_ssl_error(FALSE, "use_certificate() failed");
	}
    }

    if ( ctx->key != NULL )
    {
	if ( SSL_CTX_use_PrivateKey(ssl_ctx, ctx->key) != 1 )
	{
	    nx_ssl_error(FALSE, "use_PrivateKey() failed");
	}
	if ( SSL_CTX_check_private_key(ssl_ctx) != 1 )
	{                       
	    throw_msg("Private key %s does not match certificate %s",
		      ctx->key, ctx->cert);
	}
    }

    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2); // TODO: forbid SSLv3. For compatibility with older versions we still need that.

    ssl = SSL_new(ssl_ctx);
    if ( ssl == NULL )
    {
	nx_ssl_error(FALSE, "failed to initialize ssl context");
    }
    SSL_set_bio(ssl, bio, bio);
    SSL_set_ex_data(ssl, nx_ssl_ssl_ctx_idx, ctx);
    SSL_set_ex_data(ssl, nx_ssl_verify_result_idx, NULL); // clear verify_result

    return ( ssl );
}



int nx_ssl_read(SSL *ssl, char *buf, int *size)
{
    int retval;

    ASSERT(ssl != NULL);
    ASSERT(buf != NULL);
    ASSERT(size != NULL);
    ASSERT(*size > 0);

    retval = SSL_read(ssl, buf, *size);
    if ( retval > 0 )
    {
	*size = retval;
	return ( SSL_ERROR_NONE );
    }
    else
    {
	*size = 0;
    }
    return ( nx_ssl_check_io_error(ssl, retval) );
}



int nx_ssl_write(SSL *ssl, const char *buf, int *size)
{
    int retval;

    ASSERT(ssl != NULL);
    ASSERT(buf != NULL);
    ASSERT(size != NULL);
    ASSERT(*size > 0);

    retval = SSL_write(ssl, buf, *size);

    if ( retval > 0 )
    {
	*size = retval;
	return ( SSL_ERROR_NONE );
    }
    else
    {
	*size = 0;
    }
    return ( nx_ssl_check_io_error(ssl, retval) );
}



void nx_ssl_destroy(SSL **ssl)
{
    SSL_CTX *ssl_ctx;

    if ( *ssl == NULL )
    {
	return;
    }

    ssl_ctx = SSL_get_SSL_CTX(*ssl);
    SSL_shutdown(*ssl);
    if ( ssl_ctx != NULL )
    {
	SSL_CTX_free(ssl_ctx);
    }
    SSL_free(*ssl);
    *ssl = NULL;
}
