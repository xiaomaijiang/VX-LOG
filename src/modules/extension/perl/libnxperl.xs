#include "../common/logdata.h"

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

MODULE = Log::Nxlog		PACKAGE = Log::Nxlog


void set_field_integer(event, key, value)
    nx_logdata_t *event;
    char         *key;
    SV           *value;

    CODE:

    if ( SvOK(value) )
    {
	if ( SvIOK(value) )
	{
	    nx_logdata_set_integer(event, key, SvIV(value));
	}
	else
	{
	    Perl_croak(aTHX_ "Non-integer argument passed to nxlog::set_field_integer()");
	}
    }
    else
    { // undef
	nx_logdata_delete_field(event, key);
    }



void set_field_string(event, key, value)
    nx_logdata_t *event;
    char         *key;
    SV           *value;

    CODE:

    if ( SvOK(value) )
    {
	if ( SvPOK(value) )
	{
	    nx_logdata_set_string(event, key, SvPV_nolen(value));
	}
	else
	{
	    Perl_croak(aTHX_ "Non-string argument passed to nxlog::set_field_string()");
	}
    }
    else
    { // undef
	nx_logdata_delete_field(event, key);
    }



void set_field_boolean(event, key, value)
    nx_logdata_t *event;
    char         *key;
    SV           *value;

    CODE:

    if ( SvOK(value) )
    {
	if ( SvIOK(value) )
	{
	    if ( SvIV(value) )
	    {
		nx_logdata_set_boolean(event, key, TRUE);
	    }
	    else
	    {
		nx_logdata_set_boolean(event, key, FALSE);
	    }
	}
	else
	{
	    Perl_croak(aTHX_ "Non-integer argument passed to nxlog::set_field_boolean()");
	}
    }
    else
    { // undef
	nx_logdata_delete_field(event, key);
    }



SV *get_field(event, key)
    nx_logdata_t  *event;
    char          *key;

    CODE:

    boolean       rc;
    nx_value_t    nx_value;

    rc = nx_logdata_get_field_value(event, key, &nx_value);

    if ( rc )
    {
	if ( nx_value.defined == FALSE )
	{
	    XSRETURN_UNDEF;
	}

	if ( nx_value.type == NX_VALUE_TYPE_STRING )
	{
	    XSRETURN_PV(nx_value.string->buf);
	}
	else if ( nx_value.type == NX_VALUE_TYPE_INTEGER )
	{
	    XSRETURN_IV(nx_value.integer);
	}
	else if ( nx_value.type == NX_VALUE_TYPE_DATETIME )
	{
	    XSRETURN_IV(nx_value.datetime);
	}
	else if ( nx_value.type == NX_VALUE_TYPE_BOOLEAN )
	{
	    if ( nx_value.boolean == TRUE )
	    {
		XSRETURN_YES;
	    }
	    else
	    {
		XSRETURN_NO;
	    }
	}
	else if ( (nx_value.type == NX_VALUE_TYPE_IP4ADDR) ||
		  (nx_value.type == NX_VALUE_TYPE_IP6ADDR) )
	{
	    char *addr;

	    addr = nx_value_to_string(&nx_value);
	    RETVAL = sv_2mortal(newSVpv(addr, 0));
	    free(addr);
	}
	else if ( nx_value.type == NX_VALUE_TYPE_BINARY )
	{
	    RETVAL = sv_2mortal(newSVpv(nx_value.binary.value, nx_value.binary.len));
	}
	else
	{
	    XSRETURN_UNDEF;
	}
    }
    else
    {
	XSRETURN_UNDEF;
    }

    OUTPUT:
      RETVAL



void delete_field(event, key)
    nx_logdata_t *event;
    char         *key;

    CODE:

    nx_logdata_delete_field(event, key);



SV *field_type(event, key)
    nx_logdata_t  *event;
    char          *key;

    CODE:

    boolean       rc;
    nx_value_t    nx_value;

    rc = nx_logdata_get_field_value(event, key, &nx_value);

    if ( rc )
    {
	if ( nx_value.defined == FALSE )
	{
	    XSRETURN_UNDEF;
	}

	XSRETURN_PV(nx_value_type_to_string(nx_value.type));
    }
    else
    {
	XSRETURN_UNDEF;
    }

    OUTPUT:
      RETVAL



AV *field_names(event)
    nx_logdata_t  *event;

    CODE:
    nx_logdata_field_t *field;
    SV *sv;

    RETVAL = newAV();
    sv_2mortal((SV*) RETVAL);
    for ( field = NX_DLIST_FIRST(&(event->fields));
	  field != NULL;
	  field = NX_DLIST_NEXT(field, link) )
    {
	sv = newSVpv(field->key, 0);
	av_push(RETVAL, sv);
    }

    OUTPUT:
      RETVAL



void log_debug(msg)
    char         *msg;

    CODE:

    log_debug("%s", msg);



void log_info(msg)
    char         *msg;

    CODE:

    log_info("%s", msg);



void log_warning(msg)
    char         *msg;

    CODE:

    log_warn("%s", msg);



void log_error(msg)
    char         *msg;

    CODE:

    log_error("%s", msg);

