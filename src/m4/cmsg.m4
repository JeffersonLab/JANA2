#
# Autoconf macro for cMsg3.0
# September 12, 2009  David Lawrence <davidl@jlab.org>
#
# Usage:
#
# Just include a single line with "CMSG" in your configure.ac file. This macro
# doesn't take any arguments. It will add the necessary parameters to CPPFLAGS,
# LIBS, and LD_FLAGS based on where cMsg is installed.
#
# Inclusion of the CMSG macro implies that cmsg is enabled
#
AC_DEFUN([CMSG],
[
	AC_ARG_WITH([cmsg],
		[AC_HELP_STRING([--with-cmsg],
		[top of the cMsg installation directory])],
		[user_cmsg=$withval],
		[user_cmsg="yes"])
	
	HAVE_CMSG="no"

	if test ! "$user_cmsg" = "no"; then
		HAVE_CMSG="yes"

		if test ! x"$user_cmsg" = xyes; then
			cmsgdir="$user_cmsg"
		elif test ! x"$CMSGROOT" = x ; then 
			cmsgdir="$CMSGROOT"
		else 
			cmsgdir="/usr/local"
		fi
		
		AC_MSG_CHECKING([Looking for cMsg in $cmsgdir])
		if test ! -d $cmsgdir; then
			AC_MSG_RESULT([no])
			HAVE_CMSG="no"
		else
			AC_MSG_RESULT([yes])
		
			CMSG_CPPFLAGS=-I$cmsgdir/include
			CMSG_LDFLAGS=-L$cmsgdir/lib
			CMSG_LIBS='-lcmsg -lcmsgxx -lcmsgRegex'
			
			save_CPPFLAGS=$CPPFLAGS
			save_LDFLAGS=$LDFLAGS
			save_LIBS=$LIBS
			
			CPPFLAGS=$CMSG_CPPFLAGS
			LDFLAGS=$CMSG_LDFLAGS
			LIBS=$CMSG_LIBS
			
			AC_LANG_PUSH(C++)
			AC_CHECK_HEADER(cMsg.hxx, [h=1], [h=0])
			
			AC_MSG_CHECKING([if cmsg needs librt to link])
			cmsg_link_ok=no
			AC_TRY_LINK([#include <cMsg.hxx>],
				[cmsg::cMsg cMsgSys("","","");],
				[cmsg_link_ok=yes])
			
			if test "$cmsg_link_ok" = "no"; then
				cmsg_link_ok=failed
				CMSG_LIBS+=' -ldl -lpthread -lrt'
				LIBS=$CMSG_LIBS
				AC_TRY_LINK([#include <cMsg.hxx>],
					[cmsg::cMsg cMsgSys("","","");],
					[cmsg_link_ok=yes])
			else
				# we get here if the first link attempt succeeded and cmsg_link_ok=yes
				# We set it to "no" to indicate -lrt was not needed (confusing isn't it?)
				cmsg_link_ok=no
			fi
			
			AC_MSG_RESULT($cmsg_link_ok);
			
			if test "$cmsg_link_ok" = "failed"; then
				AC_MSG_ERROR("Can't find cMsg.hxx (using path=$cmsgdir). Set your CMSGROOT environment variable or use the --with-cmsg=PATH_TO_CMSG argument when running configure")
			fi

			AC_LANG_POP
			
			CPPFLAGS="$save_CPPFLAGS $CMSG_CPPFLAGS"
			LDFLAGS="$save_LDFLAGS $CMSG_LDFLAGS"
			LIBS="$save_LIBS $CMSG_LIBS"
			CMSG_DIR=$cmsgdir
		fi
		
		if test "$HAVE_CMSG" = "no"; then
			AC_MSG_NOTICE("Can't find cMsg (using path=$cmsgdir). ")
			AC_MSG_NOTICE("Set your CMSGROOT environment variable or use the --with-cmsg=PATH_TO_CMSG")
			AC_MSG_NOTICE("argument when running configure. Otherwise disable cMsg support by")
			AC_MSG_NOTICE("re-running configure with --without-cmsg.")
		else
			AC_MSG_NOTICE([Configuring cMsg])
			AC_SUBST(CMSG_CPPFLAGS)
			AC_SUBST(CMSG_LDFLAGS)
			AC_SUBST(CMSG_LIBS)
		fi

	else
		AC_MSG_CHECKING([for cMsg])
		AC_MSG_RESULT([no])
	fi # test ! "$user_cmsg" = "no"

])

