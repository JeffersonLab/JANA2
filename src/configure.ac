#
# autoconf configuration file for JANA
#
# To re-generate the configue script, run the "reconf" script in this
# directory (note, you probably DON'T need to regenerate configure!)
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

AC_INIT(janaconfig, version-0.2)

echo "                             Configuring JANA ..."


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
#AC_LIB_CERNLIB(graflib, CLTOU, [AC_LIB_CERNLIB(mathlib,GAUSS,[LIBS="$CERNLIB_LIBS $LIBS"])], AC_MSG_RESULT(CERNLIB support will not be included))
CMSG

AC_MSG_NOTICE([creating JANA/jana_config.h])
cp confdefs.h JANA/jana_config.h

AC_SUBST([HAVE_MYSQL])
AC_SUBST([HAVE_XERCES])
AC_SUBST([prefix])
AC_SUBST([exec_prefix])
AC_SUBST([JANA_SRC_DIR])
AC_SUBST([HAVE_ROOT])
#AC_SUBST([HAVE_CERNLIB])
AC_SUBST([HAVE_CMSG])
AC_OUTPUT(BMS/Makefile.config)

