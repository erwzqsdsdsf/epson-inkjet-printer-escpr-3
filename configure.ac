dnl configure.in -- Process this file with autoconf to produce a configure script.
dnl Epson Inkjet Printer Driver (ESC/P-R) for Linux
dnl Copyright (C) 2006-2009 AVASYS CORPORATION.
dnl Copyright (C) Seiko Epson Corporation 2006-2011.
dnl  This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

AC_PREREQ(2.53)
AC_INIT([Epson Inkjet Printer Driver (ESC/P-R) for Linux],
	[1.0.3], [Seiko Epson Corporation <linux-epson-inkjet@avasys.jp>], [epson-inkjet-printer-escpr])
AC_SUBST(PACKAGE_RELEASE, 1)            dnl  keep ALL versioning info in
                                        dnl  one location
AM_INIT_AUTOMAKE([gnu 1.7])

AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(src/filter.c)
AC_CONFIG_MACRO_DIR([m4])

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL


AC_CHECK_PROG(have_cups_config, cups-config, yes)
if test $have_cups_config = yes; then
   if test "x${prefix}" != "xNONE"; then
      CUPS_SERVER_DIR=`cups-config --serverbin | sed -e "s,^${ac_default_prefix},${prefix},"`
      CUPS_PPD_DIR=`cups-config --datadir | sed -e "s,^${ac_default_prefix},${prefix},"`
   else
      CUPS_SERVER_DIR=`cups-config --serverbin`
      CUPS_PPD_DIR=`cups-config --datadir`/model
   fi
else
   if test "x${prefix}" != "xNONE"; then
      CUPS_SERVER_DIR=${prefix}/lib/cups
      CUPS_PPD_DIR=${prefix}/share/cups/model
   else
      CUPS_SERVER_DIR=/usr/lib/cups
      CUPS_PPD_DIR=/usr/share/cups/model
   fi
   dnl AC_MSG_ERROR([*** 'cups-config' missing, please install CUPS or fix your \$PATH ***])
fi

BUILD_MACHINE_ARCH=`uname -m`
if test "x$BUILD_MACHINE_ARCH" = "xx86_64"; then
	LSB_ARCH=64
fi

dnl CUPS_LIBS='-lcups -lm'
dnl CUPS_IMAGE_LIBS='-lcupsimage -lcups -ljpeg -lm'
CUPS_LIBS="/opt/lsb/lib"$LSB_ARCH"/libcups.so /opt/lsb/lib"$LSB_ARCH"/libm.so"
CUPS_IMAGE_LIBS="/opt/lsb/lib"$LSB_ARCH"/libcupsimage.so /opt/lsb/lib"$LSB_ARCH"/libcups.so /opt/lsb/lib"$LSB_ARCH"/libjpeg.so"

AC_SUBST(CUPS_LIBS)
AC_SUBST(CUPS_IMAGE_LIBS)
AC_SUBST(CUPS_SERVER_DIR)
AC_SUBST(CUPS_PPD_DIR)


dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h locale.h malloc.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/types.h sys/time.h sys/timeb.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STAT
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([ftime memmove memset strchr strrchr select socket strcspn strerror strstr])


AC_CONFIG_FILES( \
Makefile \
lib/Makefile \
src/Makefile \
layout_script/Makefile \
ppd/Makefile \
$PACKAGE_TARNAME.spec
)
AC_OUTPUT
