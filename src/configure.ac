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
#             http://root.cern.ch (from build/misc/root.m4 of source distrib.)
#
# Modified somewhat 7/23/2008 D.L.
# ===========================================================================
dnl -*- mode: autoconf -*- 
dnl
dnl $Id: root.m4,v 1.3 2005/03/21 21:42:21 rdm Exp $
dnl $Author: rdm $
dnl $Date: 2005/03/21 21:42:21 $
dnl
dnl Autoconf macro to check for existence or ROOT on the system
dnl Synopsis:
dnl
dnl  ROOT_PATH([MINIMUM-VERSION, [ACTION-IF-FOUND, [ACTION-IF-NOT-FOUND]]])
dnl
dnl Some examples: 
dnl 
dnl    ROOT_PATH(3.03/05, , AC_MSG_ERROR(Your ROOT version is too old))
dnl    ROOT_PATH(, AC_DEFINE([HAVE_ROOT]))
dnl 
dnl The macro defines the following substitution variables
dnl
dnl    ROOTCONF           full path to root-config
dnl    ROOTEXEC           full path to root
dnl    ROOTCINT           full path to rootcint
dnl    ROOTLIBDIR         Where the ROOT libraries are 
dnl    ROOTINCDIR         Where the ROOT headers are 
dnl    ROOTCFLAGS         Extra compiler flags
dnl    ROOTLIBS           ROOT basic libraries 
dnl    ROOTGLIBS          ROOT basic + GUI libraries
dnl    ROOTAUXLIBS        Auxilary libraries and linker flags for ROOT
dnl    ROOTAUXCFLAGS      Auxilary compiler flags 
dnl    ROOTRPATH          Same as ROOTLIBDIR
dnl
dnl The macro will fail if root-config and rootcint isn't found.
dnl
dnl Christian Holm Christensen <cholm@nbi.dk>
dnl
AC_DEFUN([ROOT_PATH],
[
  AC_ARG_WITH(rootsys,
  [  --with-rootsys          top of the ROOT installation directory],
    user_rootsys=$withval,
    user_rootsys="none")
  if test ! x"$user_rootsys" = xnone; then
    rootbin="$user_rootsys/bin"
  elif test ! x"$ROOTSYS" = x ; then 
    rootbin="$ROOTSYS/bin"
  else 
   rootbin=$PATH
  fi
  AC_PATH_PROG(ROOTCONF, root-config , no, $rootbin)
  AC_PATH_PROG(ROOTEXEC, root , no, $rootbin)
  AC_PATH_PROG(ROOTCINT, rootcint , no, $rootbin)
	
  if test ! x"$ROOTCONF" = "xno" && \
     test ! x"$ROOTCINT" = "xno" ; then 

    # define some variables 
    ROOTLIBDIR=`$ROOTCONF --libdir`
    ROOTINCDIR=`$ROOTCONF --incdir`
    ROOTCFLAGS=`$ROOTCONF --noauxcflags --cflags` 
    ROOTLIBS=`$ROOTCONF --noauxlibs --noldflags --libs`
    ROOTGLIBS=`$ROOTCONF --noauxlibs --noldflags --glibs`
    ROOTAUXCFLAGS=`$ROOTCONF --auxcflags`
    ROOTAUXLIBS=`$ROOTCONF --auxlibs`
    ROOTRPATH=$ROOTLIBDIR
    ROOTVERSION=`$ROOTCONF --version`
    ROOTSOVERSION=`dirname $ROOTVERSION`
	
    if test $1 ; then 
      AC_MSG_CHECKING(whether ROOT version >= [$1])
      vers=`$ROOTCONF --version | tr './' ' ' | awk 'BEGIN { FS = " "; } { printf "%d", ($''1 * 1000 + $''2) * 1000 + $''3;}'`
      requ=`echo $1 | tr './' ' ' | awk 'BEGIN { FS = " "; } { printf "%d", ($''1 * 1000 + $''2) * 1000 + $''3;}'`
		if test $vers -lt $requ ; then 
        AC_MSG_RESULT(no)
	no_root="yes"
      else 
        AC_MSG_RESULT(yes)
		  AC_DEFINE([HAVE_ROOT],[1],[ROOT analysis framework is defined])
		  HAVE_ROOT=yes
		  AC_MSG_NOTICE([defining HAVE_ROOT=$HAVE_ROOT])
      fi
    fi
  else
    # otherwise, we say no_root
    no_root="yes"
  fi

  AC_SUBST(ROOTLIBDIR)
  AC_SUBST(ROOTINCDIR)
  AC_SUBST(ROOTCFLAGS)
  AC_SUBST(ROOTLIBS)
  AC_SUBST(ROOTGLIBS) 
  AC_SUBST(ROOTAUXLIBS)
  AC_SUBST(ROOTAUXCFLAGS)
  AC_SUBST(ROOTRPATH)
  AC_SUBST(ROOTVERSION)
  AC_SUBST(ROOTSOVERSION)

  if test "x$no_root" = "x" ; then 
    ifelse([$2], , :, [$2])     
  else 
    ifelse([$3], , :, [$3])     
  fi
])

#
# Macro to check if ROOT has a specific feature:
#
#   ROOT_FEATURE(FEATURE,[ACTION_IF_HAVE,[ACTION_IF_NOT]])
#
# For example 
#
#   ROOT_FEATURE([ldap],[AC_DEFINE([HAVE_ROOT_LDAP])])
# 
AC_DEFUN([ROOT_FEATURE],
[
  AC_REQUIRE([ROOT_PATH])
  feat=$1
  res=`$ROOTCONF --has-$feat` 
  if test "x$res" = "xyes" ; then 
    ifelse([$2], , :, [$2])     
  else 
    ifelse([$3], , :, [$3])     
  fi
])

#
# EOF
#

# ===========================================================================
#    http://web.lmd.jussieu.fr/cgi-bin/viewvc.cgi/mini_ker/config/cernlib.m4
#
# Modified somewhat 8/11/2008 D.L.
# ===========================================================================
dnl                                                      -*- Autoconf -*- 
dnl  autoconf macros for the cernlib libraries
dnl  Copyright (C) 2004, 2005 Patrice Dumas
dnl
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 2 of the License, or
dnl  (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

dnl A basic example of the macro usage:
dnl 
dnl AC_LIB_CERNLIB(kernlib,CLTOU,
dnl [
dnl    AC_LIB_CERNLIB(mathlib,GAUSS,[LIBS="$CERNLIB_LIBS $LIBS"])
dnl ])

dnl
dnl --with-static-cernlib forces the static or dynamic linking.
dnl --with-cernlib gives the location of cernlib.
dnl
dnl If the type of linking isn't specified it is assumed to be static.
dnl
dnl For static linking:
dnl - if the binary program 'cernlib-static' or 'cernlib' is in the path it is 
dnl   assumed that a static linking using the information reported by that 
dnl   script is wanted.
dnl - otherwise if the environment variable CERNLIB exists it is assumed to 
dnl   be the location of the static library.
dnl - otherwise if the environment variable CERN_ROOT exists it is assumed 
dnl   that CERN_ROOT/lib is the location of the static library.
dnl - otherwise a simple linking is performed.
dnl 
dnl If a dynamic linking is selected:
dnl - if the binary program 'cernlib' is in the path it is assumed that it 
dnl   is the debian script and it is called with -dy.
dnl - otherwise a simple linking is performed.
dnl
dnl AC_LIB_CERNLIB ([LIBRARY = kernlib], [FUNCTION = CLTOU], [ACTION-IF-FOUND],
dnl          [ACTION-IF-NOT-FOUND]) 
dnl should be called afterwards for each of the cernlib library needed. 
dnl Based on the information collected by AC_CERNLIB the needed flags are 
dnl added to the linking flags or the files and flags are added to the 
dnl $CERNLIB_LIBS shell variable and a test of linking is performed.
dnl
dnl The static library linking flags and files are in $CERNLIB_LIBS.


# AC_CERNLIB check for cernlib location and whether it should be 
# statically linked or not.
AC_DEFUN([AC_CERNLIB], [
CERNLIB_PATH=
CERNLIB_LIB_PATH=
CERNLIB_BIN_PATH=
CERNLIB_STATIC=yes
AC_ARG_WITH(static_cernlib,
[  --with-static-cernlib             link statically with the cernlib],
[  CERNLIB_STATIC=$withval      ])

AC_ARG_WITH(cernlib,
[  --with-cernlib           cernlib location],
[  CERNLIB_PATH=$withval    
   CERNLIB_LIB_PATH=$CERNLIB_PATH/lib
   CERNLIB_BIN_PATH=$CERNLIB_PATH/bin
])

AC_ARG_VAR([CERNLIB_SCRIPT], [cernlib link script])

if test z"$CERNLIB_SCRIPT" != 'z'; then
    :
elif test "z$CERNLIB_STATIC" != "zno"; then    
    CERNLIB_STATIC=yes
    if test "z$CERNLIB_PATH" = z; then
        AC_PATH_PROG(CERNLIB_SCRIPT, cernlib-static, no)
        if test $CERNLIB_SCRIPT = no; then
            AC_PATH_PROG(CERNLIB_SCRIPT_NOSTATIC, cernlib, no)
            if test "z$CERNLIB_SCRIPT_NOSTATIC" != 'zno'; then
                CERNLIB_SCRIPT=$CERNLIB_SCRIPT_NOSTATIC
            fi
        fi
    else
        AC_PATH_PROG(CERNLIB_SCRIPT, cernlib-static, no, [$CERNLIB_BIN_PATH:$PATH])
        if test $CERNLIB_SCRIPT = no; then
            AC_PATH_PROG(CERNLIB_SCRIPT_NOSTATIC, cernlib, no, [$CERNLIB_BIN_PATH:$PATH])
            if test "z$CERNLIB_SCRIPT_NOSTATIC" != 'zno'; then
                CERNLIB_SCRIPT=$CERNLIB_SCRIPT_NOSTATIC
            fi
        fi
        if test $CERNLIB_SCRIPT = no; then
            if test "z$CERNLIB" != z -a -d "$CERNLIB"; then
               CERNLIB_LIB_PATH=$CERNLIB
               AC_MSG_NOTICE([using the CERNLIB environment variable for cernlib location])
            elif test "z$CERN_ROOT" != z -a -d "$CERN_ROOT/lib"; then
                CERNLIB_LIB_PATH=$CERN_ROOT/lib
                AC_MSG_NOTICE([using the CERN_ROOT environment variable for cernlib location])
            fi
        fi
    fi
else
    if test "z$CERNLIB_PATH" = z; then
         AC_PATH_PROG(CERNLIB_SCRIPT, cernlib, no)
    else
         AC_PATH_PROG(CERNLIB_SCRIPT, cernlib, no, [$CERNLIB_BIN_PATH:$PATH])
         LDFLAGS="$LDFLAGS -L$CERNLIB_LIB_PATH"
    fi
fi
])

# AC_LIB_CERNLIB ([LIBRARY = kernlib], [FUNCTION = CLTOU], [ACTION-IF-FOUND],
#          [ACTION-IF-NOT-FOUND]) 
# check for a function in a library of the cernlib
AC_DEFUN([AC_LIB_CERNLIB], [
AC_REQUIRE([AC_CERNLIB])

cernlib_lib_ok="no"

if test "z$1" = z; then
    cern_library=kernlib
else
    cern_library=$1
fi

if test "z$2" = z; then
    cern_func=CLTOU
else
    cern_func=$2
fi

save_CERNLIB_LIBS=$CERNLIB_LIBS
save_LIBS=$LIBS

if test "z$CERNLIB_STATIC" = "zyes"; then
    cernlib_lib_static_found=no
    AC_MSG_NOTICE([cernlib: linking with a static $cern_library])
    if test "z$CERNLIB_SCRIPT" != "zno"; then
        cernlib_bin_out=`$CERNLIB_SCRIPT $cern_library`
        CERNLIB_LIBS="$cernlib_bin_out $CERNLIB_LIBS"
        cernlib_lib_static_found=yes
    elif test "z$CERNLIB_LIB_PATH" != z; then
dnl .a for UNIX libraries only ?
        if test -f "$CERNLIB_LIB_PATH/lib${cern_library}.a"; then 
            CERNLIB_LIBS="$CERNLIB_LIB_PATH/lib${cern_library}.a $CERNLIB_LIBS"
            cernlib_lib_static_found=yes
        fi
    fi
    if test "z$cernlib_lib_static_found" = zno; then
        AC_MSG_WARN([cannot determine the cernlib location for static linking])
    fi
else
    AC_MSG_NOTICE([trying a dynamical link with $cern_library])
    if test "z$CERNLIB_SCRIPT" != "zno"; then
        # try link with debian cernlib script with -dy
        cernlib_bin_out=`$CERNLIB_SCRIPT -dy $cern_library`
        CERNLIB_LIBS="$cernlib_bin_out $CERNLIB_LIBS"
    else
        CERNLIB_LIBS="-l$cern_library $CERNLIB_LIBS"
    fi
fi

dnl now try the link
LIBS="$CERNLIB_LIBS $LIBS"
AC_LANG_PUSH(Fortran)
AC_LINK_IFELSE([      program main
      call $cern_func
      end
], 
[ 
    cernlib_lib_ok=yes
],
[
    CERNLIB_LIBS=$save_CERNLIB_LIBS
])
AC_LANG_POP(Fortran)
LIBS=$save_LIBS
AC_SUBST([CERNLIB_LIBS])

if test x"$cernlib_lib_ok" = xyes ; then
	AC_DEFINE([HAVE_CERNLIB],[1],[CERNLIB is defined])
	HAVE_CERNLIB=yes
fi

AS_IF([test x"$cernlib_lib_ok" = xyes],
      [m4_default([$3], [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_CERNLIB_${cern_library}_${cern_func}))
      ])],
      [
       AC_MSG_WARN([cannot link $cern_func in lib$cern_library ])
       $4
      ])

])


# ===========================================================================
#             http://www.jlab.org/JANA
# ===========================================================================
#
# By default, install in the directory parallel to the directory configure
# is being run from (presumably src).
#
ac_default_prefix="$PWD/.."
#
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
#
#
# Set the current location as the source directory so the makefiles
# Makefile.lib and Makefile.bin, ... can make a symlink to it if they
# are installing the binaries to a non-default location.
JANA_SRC_DIR="$PWD"

# ===========================================================================
# Perform tests for optional packages used by JANA
AC_PROG_CXX
AC_LANG(C++)
AX_LIB_MYSQL([3.23.00])
ACX_PTHREAD
AX_LIB_XERCES
ROOT_PATH(5.08/00, ,AC_MSG_RESULT(ROOT support will not be included))
AC_LIB_CERNLIB(graflib, CLTOU, [AC_LIB_CERNLIB(mathlib,GAUSS,[LIBS="$CERNLIB_LIBS $LIBS"])], AC_MSG_RESULT(CERNLIB support will not be included))

AC_MSG_NOTICE([creating JANA/jana_config.h])
cp confdefs.h JANA/jana_config.h

AC_SUBST([HAVE_MYSQL])
AC_SUBST([HAVE_XERCES])
AC_SUBST([prefix])
AC_SUBST([exec_prefix])
AC_SUBST([JANA_SRC_DIR])
AC_SUBST([HAVE_ROOT])
AC_SUBST([HAVE_CERNLIB])
AC_OUTPUT(BMS/Makefile.config)

