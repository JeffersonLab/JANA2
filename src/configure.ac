#
# autoconf configuration file for JANA
#
# JANA uses its own make system (BMS) and so does not make use
# of the automake tool. It does however use the autoconf tool
# to generate the file jana_config.h. This is included by other
# files in JANA to conditionally compile sections of code that
# use optional packages such as xerces and mysql.
#
# Most of the macros used here were downloaded and are marked
# with the URL from which they were retrieved.
# 

AC_INIT(janaconfig, version-0.1)

echo "                             Configuring JANA ..."

# ===========================================================================
#              http://autoconf-archive.cryp.to/ax_lib_mysql.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_LIB_MYSQL([MINIMUM-VERSION])
#
# DESCRIPTION
#
#   This macro provides tests of availability of MySQL client library of
#   particular version or newer.
#
#   AX_LIB_MYSQL macro takes only one argument which is optional. If there
#   is no required version passed, then macro does not run version test.
#
#   The --with-mysql option takes one of three possible values:
#
#   no - do not check for MySQL client library
#
#   yes - do check for MySQL library in standard locations (mysql_config
#   should be in the PATH)
#
#   path - complete path to mysql_config utility, use this option if
#   mysql_config can't be found in the PATH
#
#   This macro calls:
#
#     AC_SUBST(MYSQL_CFLAGS)
#     AC_SUBST(MYSQL_LDFLAGS)
#     AC_SUBST(MYSQL_VERSION)
#
#   And sets:
#
#     HAVE_MYSQL
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Mateusz Loskot <mateusz@loskot.net>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([AX_LIB_MYSQL],
[
    AC_ARG_WITH([mysql],
        AC_HELP_STRING([--with-mysql=@<:@ARG@:>@],
            [use MySQL client library @<:@default=yes@:>@, optionally specify path to mysql_config]
        ),
        [
        if test "$withval" = "no"; then
            want_mysql="no"
        elif test "$withval" = "yes"; then
            want_mysql="yes"
        else
            want_mysql="yes"
            MYSQL_CONFIG="$withval"
        fi
        ],
        [want_mysql="yes"]
    )

    MYSQL_CFLAGS=""
    MYSQL_LDFLAGS=""
    MYSQL_VERSION=""

    dnl
    dnl Check MySQL libraries (libpq)
    dnl

    if test "$want_mysql" = "yes"; then

        if test -z "$MYSQL_CONFIG" -o test; then
            AC_PATH_PROG([MYSQL_CONFIG], [mysql_config], [no])
        fi

        if test "$MYSQL_CONFIG" != "no"; then
            AC_MSG_CHECKING([for MySQL libraries])

            MYSQL_CFLAGS="`$MYSQL_CONFIG --cflags`"
            MYSQL_LDFLAGS="`$MYSQL_CONFIG --libs`"

            MYSQL_VERSION=`$MYSQL_CONFIG --version`

            AC_DEFINE([HAVE_MYSQL], [1],
                [Define to 1 if MySQL libraries are available])
				HAVE_MYSQL="yes"

            found_mysql="yes"
            AC_MSG_RESULT([yes])
        else
            found_mysql="no"
            AC_MSG_RESULT([no])
        fi
    fi

    dnl
    dnl Check if required version of MySQL is available
    dnl


    mysql_version_req=ifelse([$1], [], [], [$1])

    if test "$found_mysql" = "yes" -a -n "$mysql_version_req"; then

        AC_MSG_CHECKING([if MySQL version is >= $mysql_version_req])

        dnl Decompose required version string of MySQL
        dnl and calculate its number representation
        mysql_version_req_major=`expr $mysql_version_req : '\([[0-9]]*\)'`
        mysql_version_req_minor=`expr $mysql_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        mysql_version_req_micro=`expr $mysql_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$mysql_version_req_micro" = "x"; then
            mysql_version_req_micro="0"
        fi

        mysql_version_req_number=`expr $mysql_version_req_major \* 1000000 \
                                   \+ $mysql_version_req_minor \* 1000 \
                                   \+ $mysql_version_req_micro`

        dnl Decompose version string of installed MySQL
        dnl and calculate its number representation
        mysql_version_major=`expr $MYSQL_VERSION : '\([[0-9]]*\)'`
        mysql_version_minor=`expr $MYSQL_VERSION : '[[0-9]]*\.\([[0-9]]*\)'`
        mysql_version_micro=`expr $MYSQL_VERSION : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$mysql_version_micro" = "x"; then
            mysql_version_micro="0"
        fi

        mysql_version_number=`expr $mysql_version_major \* 1000000 \
                                   \+ $mysql_version_minor \* 1000 \
                                   \+ $mysql_version_micro`

        mysql_version_check=`expr $mysql_version_number \>\= $mysql_version_req_number`
        if test "$mysql_version_check" = "1"; then
            AC_MSG_RESULT([yes])
        else
            AC_MSG_RESULT([no])
        fi
    fi

    AC_SUBST([MYSQL_VERSION])
    AC_SUBST([MYSQL_CFLAGS])
    AC_SUBST([MYSQL_LDFLAGS])
])


# ===========================================================================
#              http://autoconf-archive.cryp.to/acx_pthread.html
# ===========================================================================
#
# SYNOPSIS
#
#   ACX_PTHREAD([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   This macro figures out how to build C programs using POSIX threads. It
#   sets the PTHREAD_LIBS output variable to the threads library and linker
#   flags, and the PTHREAD_CFLAGS output variable to any special C compiler
#   flags that are needed. (The user can also force certain compiler
#   flags/libs to be tested by setting these environment variables.)
#
#   Also sets PTHREAD_CC to any special C compiler that is needed for
#   multi-threaded programs (defaults to the value of CC otherwise). (This
#   is necessary on AIX to use the special cc_r compiler alias.)
#
#   NOTE: You are assumed to not only compile your program with these flags,
#   but also link it with them as well. e.g. you should link with
#   $PTHREAD_CC $CFLAGS $PTHREAD_CFLAGS $LDFLAGS ... $PTHREAD_LIBS $LIBS
#
#   If you are only building threads programs, you may wish to use these
#   variables in your default LIBS, CFLAGS, and CC:
#
#          LIBS="$PTHREAD_LIBS $LIBS"
#          CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
#          CC="$PTHREAD_CC"
#
#   In addition, if the PTHREAD_CREATE_JOINABLE thread-attribute constant
#   has a nonstandard name, defines PTHREAD_CREATE_JOINABLE to that name
#   (e.g. PTHREAD_CREATE_UNDETACHED on AIX).
#
#   ACTION-IF-FOUND is a list of shell commands to run if a threads library
#   is found, and ACTION-IF-NOT-FOUND is a list of commands to run it if it
#   is not found. If ACTION-IF-FOUND is not specified, the default action
#   will define HAVE_PTHREAD.
#
#   Please let the authors know if this macro fails on any platform, or if
#   you have any other suggestions or comments. This macro was based on work
#   by SGJ on autoconf scripts for FFTW (http://www.fftw.org/) (with help
#   from M. Frigo), as well as ac_pthread and hb_pthread macros posted by
#   Alejandro Forero Cuervo to the autoconf macro repository. We are also
#   grateful for the helpful feedback of numerous users.
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Steven G. Johnson <stevenj@alum.mit.edu>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Macro Archive. When you make and
#   distribute a modified version of the Autoconf Macro, you may extend this
#   special exception to the GPL to apply to your modified version as well.

AC_DEFUN([ACX_PTHREAD], [
#AC_REQUIRE([AC_CANONICAL_HOST])    # This caused errors on my system 5/9/2008 DL
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no


# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
#      ... -mt is also the pthreads flag for HP/aCC
# pthread: Linux, etcetera
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthreads/-mt/
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthreads pthread -mt -pthread $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

		pthread-config)
		AC_CHECK_PROG(acx_pthread_config, pthread-config, yes, no)
		if test x"$acx_pthread_config" = xno; then continue; fi
		PTHREAD_CFLAGS="`pthread-config --cflags`"
		PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
		;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: JOINABLE attribute is called UNDETACHED.
	AC_MSG_CHECKING([for joinable pthread attribute])
	attr_name=unknown
	for attr in PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED; do
	    AC_TRY_LINK([#include <pthread.h>], [int attr=$attr; return attr;],
                        [attr_name=$attr; break])
	done
        AC_MSG_RESULT($attr_name)
        if test "$attr_name" != PTHREAD_CREATE_JOINABLE; then
            AC_DEFINE_UNQUOTED(PTHREAD_CREATE_JOINABLE, $attr_name,
                               [Define to necessary symbol if this constant
                                uses a non-standard name on your system.])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
            *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
            *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
            PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with xlc_r or cc_r
	if test x"$GCC" != xyes; then
          AC_CHECK_PROGS(PTHREAD_CC, xlc_r cc_r, ${CC})
        else
          PTHREAD_CC=$CC
	fi
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD


# ===========================================================================
#             http://autoconf-archive.cryp.to/ax_lib_xerces.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_LIB_XERCES([MINIMUM-VERSION])
#
# DESCRIPTION
#
#   This macro provides tests of availability of Apache Xerces C++ Parser of
#   particular version or newer. This macros checks for Apache Xerces C++
#   Parser headers and libraries and defines compilation flags
#
#   Macro supports following options and their values:
#
#   1) Single-option usage:
#
#     --with-xerces - yes, no or path to Xerces installation prefix
#
#   2) Three-options usage (all options are required):
#
#     --with-xerces=yes
#     --with-xerces-inc - path to base directory with Xerces headers
#     --with-xerces-lib - linker flags for Xerces
#
#   This macro calls:
#
#     AC_SUBST(XERCES_CPPFLAGS)
#     AC_SUBST(XERCES_LDFLAGS)
#     AC_SUBST(XERCES_VERSION) - only if version requirement is used
#
#   And sets:
#
#     HAVE_XERCES
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Mateusz Loskot <mateusz@loskot.net>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([AX_LIB_XERCES],
[
    AC_REQUIRE([ACX_PTHREAD])

    # Default to environment  6/4/2008 DL
    if test x"$XERCESCROOT" != "x"; then
      xerces_prefix="$XERCESCROOT"
    fi
	 
    AC_ARG_WITH([xerces],
        AC_HELP_STRING([--with-xerces=@<:@ARG@:>@],
            [use Xerces C++ Parser from given prefix (ARG=path); check standard prefixes (ARG=yes); disable (ARG=no)]
        ),
        [
        if test "$withval" = "yes"; then
            if test -d /usr/local/xerces/PRO/include/xercesc ; then
                xerces_prefix=/usr/local/xerces/PRO
            elif test -d /usr/local/include/xercesc ; then
                xerces_prefix=/usr/local
            elif test -d /usr/include/xercesc ; then
                xerces_prefix=/usr
            else
                xerces_prefix=""
            fi
            xerces_requested="yes"
        elif test -d "$withval"; then
            xerces_prefix="$withval"
            xerces_requested="yes"
        else
            xerces_prefix=""
            xerces_requested="no"
        fi
        ],
        [
        dnl Default behavior is implicit yes
			if test -d /usr/local/xerces/PRO/include/xercesc ; then
				xerces_prefix=/usr/local/xerces/PRO
			elif test -d /usr/local/include/xercesc ; then
            xerces_prefix=/usr/local
        elif test -d /usr/include/xercesc ; then
            xerces_prefix=/usr
        else
            xerces_prefix=""
        fi
        ]
    )

    AC_ARG_WITH([xerces-inc],
        AC_HELP_STRING([--with-xerces-inc=@<:@DIR@:>@],
            [path to Xerces C++ Parser headers]
        ),
        [xerces_include_dir="$withval"],
        [xerces_include_dir=""]
    )
    AC_ARG_WITH([xerces-lib],
        AC_HELP_STRING([--with-xerces-lib=@<:@ARG@:>@],
            [link options for Xerces C++ Parser libraries]
        ),
        [xerces_ldflags="$withval"],
        [xerces_ldflags=""]
    )

    XERCES_CPPFLAGS=""
    XERCES_LDFLAGS=""
    XERCES_VERSION=""

    dnl
    dnl Collect include/lib paths and flags
    dnl
    run_xerces_test="no"

AC_MSG_NOTICE([xerces_prefix = $xerces_prefix])
    if test -n "$xerces_prefix"; then
        xerces_include_dir="$xerces_prefix/include"
        xerces_include_dir2="$xerces_prefix/include/xercesc"
        xerces_ldflags="-L$xerces_prefix/lib"
        run_xerces_test="yes"
    elif test "$xerces_requested" = "yes"; then
        if test -n "$xerces_include_dir" -a -n "$xerces_lib_flags"; then
            xerces_include_dir2="$xerces_include_dir/xercesc"
            run_xerces_test="yes"
        fi
    else
        run_xerces_test="no"
    fi

    xerces_libs="-lxerces-c"

    dnl
    dnl Check Xerces C++ Parser files
    dnl
    if test "$run_xerces_test" = "yes"; then

        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS -I$xerces_include_dir -I$xerces_include_dir2"

        saved_LDFLAGS="$LDFLAGS"
        LDFLAGS="$LDFLAGS $xerces_ldflags $PTHREAD_LDFLAGS"

        saved_LIBS="$LIBS"
        LIBS="$xerces_libs $PTHREAD_LIBS $LIBS"

        dnl
        dnl Check Xerces headers
        dnl
        AC_MSG_CHECKING([for Xerces C++ Parser headers in $xerces_include_dir and $xerces_include_dir2])

        AC_LANG_PUSH([C++])
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM(
                [[
@%:@include <xercesc/util/XercesDefs.hpp>
@%:@include <xercesc/util/PlatformUtils.hpp>
                ]],
                [[]]
            )],
            [
            XERCES_CPPFLAGS="-I$xerces_include_dir -I$xerces_include_dir2"
            xerces_header_found="yes"
            AC_MSG_RESULT([found])
            ],
            [
            xerces_header_found="no"
            AC_MSG_RESULT([not found])
            ]
        )
        AC_LANG_POP([C++])

        dnl
        dnl Check Xerces libraries
        dnl
        if test "$xerces_header_found" = "yes"; then

            AC_MSG_CHECKING([for Xerces C++ Parser libraries])

            AC_LANG_PUSH([C++])
            AC_LINK_IFELSE([
                AC_LANG_PROGRAM(
                    [[
@%:@include <xercesc/util/XercesDefs.hpp>
@%:@include <xercesc/util/PlatformUtils.hpp>
#if (_XERCES_VERSION >= 20200)
XERCES_CPP_NAMESPACE_USE
#endif
                    ]],
                    [[
XMLPlatformUtils::Initialize();
                    ]]
                )],
                [
                XERCES_LDFLAGS="$xerces_ldflags $PTHREAD_LDFLAGS"
                XERCES_LIBS="$xerces_libs $PTHREAD_LIBS"
                xerces_lib_found="yes"
                AC_MSG_RESULT([found])
                ],
                [
                xerces_lib_found="no"
                AC_MSG_RESULT([not found])
                ]
            )
            AC_LANG_POP([C++])
        fi

        CPPFLAGS="$saved_CPPFLAGS"
        LDFLAGS="$saved_LDFLAGS"
        LIBS="$saved_LIBS"
    fi

    AC_MSG_CHECKING([for Xerces C++ Parser])

    if test "$run_xerces_test" = "yes"; then
        if test "$xerces_header_found" = "yes" -a "$xerces_lib_found" = "yes"; then

            AC_SUBST([XERCES_CPPFLAGS])
            AC_SUBST([XERCES_LDFLAGS])
            AC_SUBST([XERCES_LIBS])

            HAVE_XERCES="yes"
        else
            HAVE_XERCES="no"
        fi

        AC_MSG_RESULT([$HAVE_XERCES])

        dnl
        dnl Check Xerces version
        dnl
        if test "$HAVE_XERCES" = "yes"; then
		  
				AC_DEFINE([HAVE_XERCES],[1],[Xerces C++ XML parser is defined])

            xerces_version_req=ifelse([$1], [], [], [$1])

            if test  -n "$xerces_version_req"; then

                AC_MSG_CHECKING([if Xerces C++ Parser version is >= $xerces_version_req])

                if test -f "$xerces_include_dir2/util/XercesVersion.hpp"; then

                    xerces_major=`cat $xerces_include_dir2/util/XercesVersion.hpp | \
                                    grep '^#define.*XERCES_VERSION_MAJOR.*[0-9]$' | \
                                    sed -e 's/#define XERCES_VERSION_MAJOR.//'`

                    xerces_minor=`cat $xerces_include_dir2/util/XercesVersion.hpp | \
                                    grep '^#define.*XERCES_VERSION_MINOR.*[0-9]$' | \
                                    sed -e 's/#define XERCES_VERSION_MINOR.//'`

                    xerces_revision=`cat $xerces_include_dir2/util/XercesVersion.hpp | \
                                    grep '^#define.*XERCES_VERSION_REVISION.*[0-9]$' | \
                                    sed -e 's/#define XERCES_VERSION_REVISION.//'`

                    XERCES_VERSION="$xerces_major.$xerces_minor.$xerces_revision"
                    AC_SUBST([XERCES_VERSION])

                    dnl Decompose required version string and calculate numerical representation
                    xerces_version_req_major=`expr $xerces_version_req : '\([[0-9]]*\)'`
                    xerces_version_req_minor=`expr $xerces_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
                    xerces_version_req_revision=`expr $xerces_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
                    if test "x$xerces_version_req_revision" = "x"; then
                        xerces_version_req_revision="0"
                    fi

                    xerces_version_req_number=`expr $xerces_version_req_major \* 10000 \
                                               \+ $xerces_version_req_minor \* 100 \
                                               \+ $xerces_version_req_revision`

                    dnl Calculate numerical representation of detected version
                    xerces_version_number=`expr $xerces_major \* 10000 \
                                          \+ $xerces_minor \* 100 \
                                           \+ $xerces_revision`

                    xerces_version_check=`expr $xerces_version_number \>\= $xerces_version_req_number`
                    if test "$xerces_version_check" = "1"; then
                        AC_MSG_RESULT([yes])
                    else
                        AC_MSG_RESULT([no])
                        AC_MSG_WARN([Found Xerces C++ Parser $XERCES_VERSION, which is older than required. Possible compilation failure.])
                    fi
                else
                    AC_MSG_RESULT([no])
                    AC_MSG_WARN([Missing header XercesVersion.hpp. Unable to determine Xerces version.])
                fi
            fi
        fi

    else
        HAVE_XERCES="no"
        AC_MSG_RESULT([$HAVE_XERCES])

        if test "$xerces_requested" = "yes"; then
            AC_MSG_WARN([Xerces C++ Parser support requested but headers or library not found. Specify valid prefix of Xerces C++ using --with-xerces=@<:@DIR@:>@ or provide include directory and linker flags using --with-xerces-inc and --with-xerces-lib])
        fi
    fi
])


# ===========================================================================
#             http://www.jlab.org/JANA
# ===========================================================================
#
# If either the JANA_MY or JANA_HOME environment variable is set, then use
# it as our default installation prefix (with JANA_MY being preferred over
# JANA_HOME). Otherwise, default to /usr/local.
#
if test x"$JANA_HOME" != "x" ; then
    ac_default_prefix="$JANA_HOME"
fi
if test x"$JANA_MY" != "x" ; then
    ac_default_prefix="$JANA_MY"
fi
#
# For solaris machines we want to default to the solaris compiler(CC) as
# opposed to the gnu compiler(g++). For some reason, this is not the default
# with the AC_PROG_CXX test below which finds a gnu compiler, but then
# procedes to complain that we are not using the native compiler and
# requires us to re-run it with a special command line switch. Note that
# we also only set the C++ compiler (variable CCC) if the variable is
# not already set to allow the user to override this default behavior
# in an expected way.
my_os=`uname`
if test "$my_os" = "SunOS"; then
    if test x"$CCC"="x"; then
        CCC="CC"
    fi
fi
AC_MSG_NOTICE([uname/CCC: $my_os/$CCC])


# ===========================================================================
# Perform tests for optional packages used by JANA
AC_PROG_CXX
AC_LANG(C++)
AX_LIB_MYSQL([3.23.00])
ACX_PTHREAD
AX_LIB_XERCES

AC_MSG_NOTICE([creating JANA/jana_config.h])
cp confdefs.h JANA/jana_config.h

AC_SUBST([HAVE_MYSQL])
AC_SUBST([HAVE_XERCES])
AC_SUBST([prefix])
AC_SUBST([exec_prefix])
AC_SUBST([testme])
AC_OUTPUT(BMS/jana_conf.mk)

