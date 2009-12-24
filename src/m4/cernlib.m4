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
