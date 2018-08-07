/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "charconv.h"
#include "../../../common/error_debug.h"
#include "../../../common/exception.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

/* utf-8 replacement character */
#define NX_UNICODE_INVALID_UTF8 "\xEF\xBF\xBD"
#define NX_UNICODE_INVALID_UTF8_LENGTH   3


/**
 * Return the locale charset name.
 * This function returns a string with the name of the character encoding
 * used in the selected locale.
 * \return name of the character encoding used in the selected locale
 */

const char  *nx_get_locale_charset(void)
{
    const char *charset = NULL;

#ifdef HAVE_NL_LANGINFO
    charset = nl_langinfo(CODESET);
#else
    extern const char *locale_charset(void);
    charset = locale_charset();
#endif	

    if ( strcasecmp(charset, "646") == 0 )
    { // solaris+libiconv with an unset locale returns "646" and iconv_open fails
	charset = "ANSI_X3.4-1968";
    }

    return ( charset );
}



iconv_t	nx_iconv_open(const char *to_encoding,	 ///< convert to this encoding, NULL to use locale charset
		      const char *from_encoding) ///< convert from this encoding, NULL to use locale charset
{
    iconv_t cd;

    cd = iconv_open(to_encoding == NULL ? nx_get_locale_charset() : to_encoding,
		    from_encoding == NULL ? nx_get_locale_charset() : from_encoding);
    
    if ( cd == (iconv_t) -1 )
    {
	switch ( errno )
	{
	    case EINVAL:
		throw_errno("iconv_open(): conversion from '%s' to '%s' not available",
			    from_encoding == NULL ? nx_get_locale_charset() : from_encoding,
			    to_encoding == NULL ? nx_get_locale_charset() : to_encoding);
	    case EMFILE:
		throw_errno("iconv_open(): the process already has OPEN_MAX file descriptors open");
	    case ENFILE:
		throw_errno("iconv_open(): the system limit of open files is reached");
	    case ENOMEM:
		throw_errno("iconv_open(): not enough memory");
	    case 0:
		    nx_panic("iconv_open() returned -1 and errno is 0");
	    default:
		throw_errno("iconv() failed: unknown error (errno: %d)");
	}
    }

    return ( cd );
}



/**
 * Convert two strings using iconv()
 * \return the number of bytes written to \p outptr
 */

static int32_t _nx_convert(char		**outptr,		///< destination string
			  int32_t	*outbytesleft,		///< number of bytes in \p dst
			  const char	**inptr,		///< string to convert from
			  int32_t	*inbytesleft,		///< number of bytes to convert from \p inptr
			  const char	*to_encoding,		///< convert to this encoding, NULL to use locale charset
			  const char	*from_encoding,		///< convert from this encoding, NULL to use locale charset
			  iconv_t	*iconv_ctx,		///< iconv handle, can be NULL to allocate one iternally
			  const char	**ok_encoding,		///< store the encoding which was used, NULL if not needed
			  boolean	convert_invalid)	///< if false, throw an exception if a conversion error was detected; if TRUE, invalid characters are converted to a questionmark '?'
{
    iconv_t cd;
    size_t nconv;
    boolean done = FALSE;
    int32_t outbytes;
    const char *start;
    size_t _inbytesleft, _outbytesleft;
    
    ASSERT(outptr != NULL);
    ASSERT(*outptr != NULL);
    ASSERT(inptr != NULL);
    ASSERT(*inptr != NULL);
    ASSERT(outbytesleft != NULL);
    ASSERT(inbytesleft != NULL);
    ASSERT(*outbytesleft > 0);
    ASSERT(*inbytesleft >= 0);

    start = *inptr;

    if ( *inbytesleft == 0 )
    {
	return ( 0 );
    }
    _inbytesleft = (size_t) *inbytesleft;
    _outbytesleft = (size_t) *outbytesleft;
    outbytes = *outbytesleft;

    errno = 0;
    if ( iconv_ctx == NULL )
    {
	cd = nx_iconv_open(to_encoding, from_encoding);
    }
    else
    {
	cd = *iconv_ctx;
    }

    do
    {
	errno = 0;
	nconv = iconv(cd, (char **) inptr, &_inbytesleft, outptr, &_outbytesleft);
	*inbytesleft = (int32_t) _inbytesleft;
	*outbytesleft = (int32_t) _outbytesleft;
	if ( nconv == (size_t) -1 ) // conversion failed
	{
	    if ( convert_invalid == FALSE )
	    {
		switch ( errno )
		{
		    case EILSEQ:
			if ( iconv_ctx == NULL )
			{ //
			    iconv_close(cd);
			}
			throw_msg("iconv() failed: invalid %s byte sequence in input at pos %lld",
				  from_encoding == NULL ? "" : from_encoding,
				  (long long int) (*inptr - start));
		    case E2BIG:
			//throw_errno("iconv() failed: output buffer is too small (%d bytes)", outbytes);
			done = TRUE;
			errno = 0;
			break;
		    case EINVAL:
			if ( iconv_ctx == NULL )
			{ //
			    iconv_close(cd);
			}
			throw_msg("iconv() failed: incomplete byte sequence at end of input");
		    case EBADF:
			throw_errno("iconv() failed: invalid iconv descriptor: %p", cd);
		    case 0:
			nx_panic("iconv() returned -1 and errno is 0");
		    default:
			if ( iconv_ctx == NULL )
			{ //
			    iconv_close(cd);
			}
			throw_errno("iconv() failed: unknown error (errno: %d)");
		}
	    }
	    else
	    {
		switch ( errno )
		{
		    case EINVAL: /* incomplete multibyte sequence */
		    case EILSEQ: /* invalid multibyte sequence */
			if ( ((to_encoding != NULL) && (strcasecmp(to_encoding, "UTF-8") == 0)) ||
			     ((to_encoding == NULL) && (strcasecmp(nx_get_locale_charset(), "UTF-8") == 0)) )
			{ // use a UTF-8 replacement character
			    if ( *outbytesleft < (int32_t) sizeof(NX_UNICODE_INVALID_UTF8) - 1 )
			    {
				done = TRUE;
				break;
			    }
			    (*inptr)++;
			    apr_cpystrn(*outptr, NX_UNICODE_INVALID_UTF8, sizeof(NX_UNICODE_INVALID_UTF8) - 1);
			    (*outptr) += sizeof(NX_UNICODE_INVALID_UTF8) - 1;
			    (*outbytesleft) -= ((int32_t) sizeof(NX_UNICODE_INVALID_UTF8)) - 1;
			    (_outbytesleft) -= sizeof(NX_UNICODE_INVALID_UTF8) - 1;
			}
			else
			{ //FIXME: this will only work with ascii compatible character sets
			    (*inptr)++;
			    **outptr = '?';
			    (*outptr)++;
			    (*outbytesleft)--;
			    _outbytesleft--;
			}
			break;
		    case E2BIG:
			// end is truncated, do nothing
			done = TRUE;
			break;
		    case EBADF:
			throw_errno("iconv() failed: invalid iconv descriptor: %p", cd);
		    case 0:
			nx_panic("iconv() returned -1 and errno is 0");
		    default:
			if ( iconv_ctx == NULL )
			{ //
			    iconv_close(cd);
			}
			throw_errno("iconv() failed: unknown error (errno: %d)");
		}
		errno = 0;
	    }
	}
    } while ( (done == FALSE) && (*inbytesleft > 0) && (*outbytesleft > 0) );


    if ( iconv_ctx == NULL )
    {
	if ( iconv_close(cd) != 0 )
	{
	    switch ( errno )
	    {
		case EBADF:
		    throw_errno("iconv_close() failed: invalid iconv descriptor: %p", cd);
		default:
		    throw_msg("iconv()_close failed: unknown error (errno: %d)", errno);
	    }
	}
    }

    if ( (ok_encoding != NULL) && (to_encoding != NULL) )
    {
	*ok_encoding = from_encoding;
    }

    return ( outbytes - *outbytesleft );
}



/**
 * Convert two strings with an already initialized iconv handle
 * \return the number of bytes written to \p outptr
 */


int32_t nx_convert_ctx(char		**outptr,		///< destination string
		      int32_t		*outbytesleft,		///< number of bytes in \p outptr
		      const char	**inptr,		///< string to convert from
		      int32_t		*inbytesleft,		///< number of bytes to convert from \p inptr
		      iconv_t		*iconv_ctx,		///< iconv handle
		      boolean		convert_invalid)	///< if false, throw an exception if a conversion error was detected; if TRUE, invalid characters are converted to a questionmark '?'
{
    ASSERT(iconv_ctx != NULL);

    return ( _nx_convert(outptr, outbytesleft, inptr, inbytesleft, NULL, NULL,
			iconv_ctx, NULL, convert_invalid) );
}



/**
 * Convert two strings using the specified encodings.
 * This function will try the conversion with all the specified encondings and stop
 * at the first that succeeds.
 * \return the number of bytes written to \p outptr
 */

int32_t nx_convert_auto(char		**outptr,		///< destination string
		       int32_t		*outbytesleft,		///< number of bytes in \p outptr
		       const char	**inptr,		///< string to convert from
		       int32_t		*inbytesleft,		///< number of bytes to convert from \p inptr
		       const char	* volatile to_encoding,	///< convert to this encoding, NULL to use locale charset
		       int32_t		num_encoding,		///< size of the encodings array
		       const char	*from_encodings[],	///< encodings to try
		       const char	**ok_encoding)		///< store the encoding which was used, NULL if not needed
{
    volatile int32_t i = 0;
    nx_exception_t e;
    volatile int32_t retval = 0;
    volatile boolean done = FALSE;

    ASSERT( num_encoding >= 0 );

    for ( i = 0; (i < num_encoding) && (done == FALSE); i++ )
    {
	try
	{
	    int32_t _outbytesleft, _inbytesleft;
	    const char *_inptr = *inptr;
	    char *_outptr = *outptr;

	    ASSERT(from_encodings[i] != NULL);
	    _outbytesleft = *outbytesleft;
	    _inbytesleft = *inbytesleft;

	    retval = _nx_convert(&_outptr, &_outbytesleft, &_inptr, &_inbytesleft, to_encoding,
				from_encodings[i], NULL, ok_encoding, FALSE);
	    *outbytesleft = _outbytesleft;
	    *inbytesleft = _inbytesleft;
	    *outptr = _outptr;
	    *inptr = _inptr;
	    done = TRUE;
	}
	catch(e)
	{
	    if ( e.code != APR_SUCCESS )
	    {
		rethrow(e);
	    }
	    if ( i >= num_encoding - 1)
	    {
		rethrow_msg(e, "automatic conversion to %s failed",
			    to_encoding == NULL ? "locale charset" : to_encoding);
	    }
	}
    }

    return ( retval );
}



/**
 * Convert two strings using the specified encodings
 * \return the number of bytes written to \p outptr
 */

int32_t nx_convert(char		**outptr,		///< destination string
		  int32_t	*outbytesleft,		///< number of bytes in \p outpr
		  const char	**inptr,		///< string to convert from
		  int32_t	*inbytesleft,		///< number of bytes to convert from \p inptr
		  const char	*to_encoding,		///< convert to this encoding, NULL to use locale charset
		  const char	*from_encoding,		///< convert from this encoding, NULL to use locale charset
		  boolean	convert_invalid)	///< if false, throw an exception if a conversion error was detected; if TRUE, invalid characters are converted to a questionmark '?'
{
    return ( _nx_convert(outptr, outbytesleft, inptr, inbytesleft, to_encoding,
			from_encoding, NULL, NULL, convert_invalid) );
}

