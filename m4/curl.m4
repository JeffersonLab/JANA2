#
#  Downloaded from the following URL on Oct. 24, 2012
#
# http://code.crt.realtors.org/projects/librets/browser/librets/trunk/project/build/ac-macros/curl.m4?rev=1489
#
# This has been modified to:
#
# 1.) set the variable HAVE_CURL to be either 0 or 1.
#
# 2.) Print warning messages instead of errors where appropriate
#     which stopped configure
#
# 3.) Lowered version number down to 7.0.0 (centOS5.3 seems to have
#     7.15.5 and the original version of this macro had minimum of
#     7.18.2) n.b. version number 7.17.00 is also buried in this macro
#     and seems to be used to test whether to run curl-config with
#     --static-libs or not
#
# 4.) Supressed warning about needing libcurl.a for librets (librets
#     is the package I stole this macro from)
#
# 5.) Added --with[out]-curl command line option
#
# 6.) Disabled shared/static library checks which seems to be librets
#     specific since it needs the static library
#
# ---------------------------------------------------------------------
dnl
dnl Test for libcurl
dnl
AC_DEFUN([MY_TEST_CURL], [

  HAVE_CURL="0"

  # This is here just to check for --without-curl so we can 
  # disable it explictly below. If the user passes us
  # --without-curl then the variable with_curl is automatically
  # set to "no"
  AC_ARG_WITH([curl],
		[AC_HELP_STRING([--with-curl],
		[include cURL library support])],
		[],
		[with_curl="notspecified"])

  if test "x$with_curl" = xno; then
 
    HAVE_CURL="0"
    AC_MSG_NOTICE([cURL library support disabled. Building to use external curl binary])

  else
 
	  AC_CACHE_VAL(my_cv_curl_vers, [
   	 my_cv_curl_vers=NONE
   	 dnl check is the plain-text version of the required version
   	 check="7.0.0"
   	 check_hex="070000"
	#    check="7.18.2"
	#    check_hex="071202"

   	 AC_MSG_CHECKING([for curl >= $check])

   	 if eval curl-config --version 2>/dev/null >/dev/null; then
		  HAVE_CURL="1"

      	ver=`curl-config --version | perl -pe "s/libcurl //g"`
      	hex_ver=`curl-config --vernum`
      	ok=`perl -e "print (hex('$hex_ver')>=hex('$check_hex') ? '1' : '0')"`

      	if test x$ok != x0; then
      	  my_cv_curl_vers="$ver"
      	  AC_MSG_RESULT([$my_cv_curl_vers])

      	  CURL_PREFIX=`curl-config --prefix`
      	  CURL_CFLAGS=`curl-config --cflags`
		  CURL_LDFLAGS="-L$CURL_PREFIX/lib"
		case $host_os in
	   	 *mingw* | *cygwin*) CURL_CFLAGS="$CURL_CFLAGS -DCURL_STATICLIB" ;;
		esac

      	  CURL_LIBS=`curl-config --libs`

      	else
      	  AC_MSG_RESULT(FAILED)
      	  AC_MSG_ERROR([$ver is too old. Need version $check or higher.])
      	fi
   	 else
      	AC_MSG_RESULT(FAILED)
			if test "$with_curl" = notspecified; then
      	  AC_MSG_WARN([curl-config was not found. Building with external cURL support...])
			else
			  AC_MSG_ERROR([curl-config was not found! (use --without-curl to build without curl library support)])
         fi
   	 fi
	  ])


	  AC_SUBST(CURL_PREFIX)
	  AC_SUBST(CURL_CFLAGS)
	  AC_SUBST(CURL_LDFLAGS)
	  AC_SUBST(CURL_LIBS)

  fi # Test if with_curl==no
])
