## Makefile.am -- an automake template for a Makefile.in file
## EPSON ESC/P-R Printer Driver for Linux
## Copyright (C) 2000-2006 AVASYS CORPORATION.
## Copyright (C) Seiko Epson Corporation 2000-2006.
##  This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA.

lib_LTLIBRARIES = libescpr.la

## Make sure these will be cleaned even when they're not built by
## default.
CLEANFILES = libescpr.la
libescpr_la_SOURCES = \
	escpr_api.c escpr_api.h \
	escpr_cmd.c escpr_cmd.h \
	escpr_def.h \
	escpr_osdep.c escpr_osdep.h \
	escpr_rle.c escpr_rle.h \
	escpr_sp.c escpr_sp.h
libescpr_la_LDFLAGS = -O2 -version-info 1:0:0

