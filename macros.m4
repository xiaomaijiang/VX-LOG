
dnl AC_C_ADD_FLAG
AC_DEFUN([AC_C_ADD_FLAG],
    [AC_CACHE_CHECK([whether ${CC-cc} accepts $2], [ac_cv_c_flag_$1],
	    [saved_CFLAGS="$CFLAGS" ac_cv_c_flag_$1=no
		for i in $2; do
		    CFLAGS="$saved_CFLAGS $i"
		    # gcc-3.0 -ggdb crashes from this:
		    AC_TRY_COMPILE([#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define max(a, b)							\
	({								\
		typeof(a)	_a = (a);				\
		typeof(b)	_b = (b);				\
		MAX(_a, _b);						\
	})],
			[max(42, 63)],
			[ac_cv_c_flag_$1="$i"
			    break])
		done
		CFLAGS="$saved_CFLAGS"])
	if test "x$ac_cv_c_flag_$1" != "xno"; then
	    CFLAGS="$CFLAGS $ac_cv_c_flag_$1"
	fi])


dnl AC_ENABLE_FEATURE
AC_DEFUN([AC_ENABLE_FEATURE],
    [AC_MSG_CHECKING([whether $1 is set/enabled])
    AC_CACHE_VAL(ac_cv_enable_[]translit($1, [ ], [_]), [
	withval=
	enableval=
	AC_ARG_$2(translit($1, [ ], [-]),
	    AC_HELP_STRING(--[]translit($2, [A-Z], [a-z])[]-[]translit($1, [ ], [-]),
		$4 (default: ifelse($3, , yes, $3))),
	    if test "x$enableval" != "x"; then
		withval=$enableval
	    fi
	    ac_cv_enable_[]translit($1, [ ], [_])=$withval
	    if test "x$withval" = "xyes"; then
		case "$3" in
		    ''|yes|no|auto[)] ;;
		    *[)] ac_cv_enable_[]translit($1, [ ], [_])=$3;;
		esac
	    fi,
	    if test "x$enableval" != "x"; then
		withval=$enableval
	    fi
	    ac_cv_enable_[]translit($1, [ ], [_])=$withval
	    if test -z "$withval"; then
		case "$3" in
		    ''[)] ac_cv_enable_[]translit($1, [ ], [_])=yes;;
		    *[)] ac_cv_enable_[]translit($1, [ ], [_])=$3;;
		esac
	    fi)])
    ac_err=
    withval=$ac_cv_enable_[]translit($1, [ ], [_])
    case $withval in
    $5
	*) ac_err="invalid argument for $1: $withval";;
    esac
    if test -z "$ac_err"; then
	AC_MSG_RESULT($withval)
    else
	AC_MSG_RESULT(failed)
	AC_MSG_ERROR($ac_err)
    fi])
