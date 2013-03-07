#
# Autoconf macro for ccdb0.0.6
# March 2, 2013  David Lawrence <davidl@jlab.org>
#
# Usage:
#
# Just include a single line with "CCDB" in your configure.ac file. This macro
# doesn't take any arguments. It will define HAVE_CCDB to be either "0" or
# "1" depending on whether CCDB is available (and requested). It will also
# define CCDB_CPPFLAGS, CCDB_LDFLAGS, and CCDB_LIBS.
#
#
AC_DEFUN([CCDB],
[
	HAVE_CCDB="0"

	# The behavior we want here is that if the user doesn't specify whether to
	# include CCDB or not, then we look to see if the CCDB_HOME environment
	# variable is set. If it is, then we will automatically try to include it.
	# If it is not set, and the --with[out]-ccdb command line switch is not
	# specified, then we disable CCDB support.
	AC_ARG_WITH([ccdb],
		[AC_HELP_STRING([--with-ccdb@<:@=DIR@:>@],
		[include CCDB support (with CCDB install dir)])],
		[user_ccdb=$withval],
		[user_ccdb="notspecified"])

	if test "$user_ccdb" = "notspecified"; then
		if test ! x"$CCDB_HOME" = x ; then
			user_ccdb="yes"
		else
			user_ccdb="no"
		fi
	fi
	
	if test ! "$user_ccdb" = "no"; then
		HAVE_CCDB="1"

		if test ! x"$user_ccdb" = xyes; then
			ccdbdir="$user_ccdb"
		elif test ! x"$CCDB_HOME" = x ; then 
			ccdbdir="$CCDB_HOME"
		else 
			ccdbdir="/usr/local"
		fi
		
		AC_MSG_CHECKING([Looking for CCDB in $ccdbdir])
		if test ! -d $ccdbdir; then
			AC_MSG_RESULT([no])
			HAVE_CCDB="0"
		else
			AC_MSG_RESULT([yes])
		
			CCDB_CPPFLAGS=-I$ccdbdir/include
			CCDB_LDFLAGS=-L$ccdbdir/lib
			CCDB_LIBS='-lccdb'
			
			save_CPPFLAGS=$CPPFLAGS
			save_LDFLAGS=$LDFLAGS
			save_LIBS=$LIBS
			
			CPPFLAGS=$CCDB_CPPFLAGS
			LDFLAGS=$CCDB_LDFLAGS
			LIBS=$CCDB_LIBS
			
			AC_LANG_PUSH(C++)
			AC_CHECK_HEADER(cMsg.hxx, [h=1], [h=0])
			
			AC_MSG_CHECKING([if ccdb needs libmysql to link])
			ccdb_link_ok=no
			AC_TRY_LINK([#include <CCDB/CalibrationGenerator.h>],
				[new ccdb::CalibrationGenerator();],
				[ccdb_link_ok=yes])
			
			if test "$ccdb_link_ok" = "no"; then
				ccdb_link_ok=failed
				CCDB_LIBS+=' -lmysql'
				LIBS=$CCDB_LIBS
				AC_TRY_LINK([[#include <CCDB/CalibrationGenerator.h>],
					[new ccdb::CalibrationGenerator();],
					[ccdb_link_ok=yes])
			else
				# we get here if the first link attempt succeeded and ccdb_link_ok=yes
				# We set it to "no" to indicate -lmysql was not needed (confusing isn't it?)
				ccdb_link_ok=no
			fi
			
			AC_MSG_RESULT($ccdb_link_ok);
			
			if test "$ccdb_link_ok" = "failed"; then
				AC_MSG_ERROR("Test link of CCDB failed (using path=$ccdbdir). Set your CCDB_HOME environment variable or use the --with-ccdb=PATH_TO_CCDB argument when running configure")
			fi

			AC_LANG_POP
			
			CPPFLAGS="$save_CPPFLAGS $CCDB_CPPFLAGS"
			LDFLAGS="$save_LDFLAGS $CCDB_LDFLAGS"
			LIBS="$save_LIBS $CCDB_LIBS"
			CCDB_DIR=$ccdbdir
		fi
		
		if test "$HAVE_CCDB" = "0"; then
			AC_MSG_NOTICE("Can't find cMsg (using path=$ccdbdir). ")
			AC_MSG_NOTICE("Set your CCDB_HOME environment variable or use the --with-ccdb=PATH_TO_CCDB")
			AC_MSG_NOTICE("argument when running configure. Otherwise disable cMsg support by")
			AC_MSG_NOTICE("re-running configure with --without-ccdb.")
		else
			CCDB_HOME="$ccdbdir"
			AC_MSG_NOTICE([Configuring cMsg])
			AC_SUBST(CCDB_HOME)
			AC_SUBST(CCDB_CPPFLAGS)
			AC_SUBST(CCDB_LDFLAGS)
			AC_SUBST(CCDB_LIBS)
		fi

	else
		AC_MSG_CHECKING([for cMsg])
		AC_MSG_RESULT([no])
	fi # test ! "$user_ccdb" = "no"

])

