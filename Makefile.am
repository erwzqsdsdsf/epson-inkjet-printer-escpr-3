## Makefile.am -- an automake template for a Makefile.in file
## EPSON ESC/P-R Printer Driver for Linux
## Copyright (C) 2000-2008 AVASYS CORPORATION.
## Copyright (C) Seiko Epson Corporation 2000-2010.
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
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

SUBDIRS = lib src \
          layout_script ppd

ACLOCAL_AMFLAGS = -I m4


#  Automate maintenance of the list of shipped M4 macros.
#  Do not change the list unnecessarily.  As a side effect, this also
#  works around issues with a read-only $(srcdir) during a 'distcheck'.

$(top_builddir)/m4-macros: $(top_srcdir)/m4
	echo M4_MACROS = `ls $^ | sed 's,^,m4/,'` > m4-macros.tmp
	cmp -s m4-macros.tmp $@ || cp m4-macros.tmp $@
	rm m4-macros.tmp

include m4-macros


EXTRA_DIST = \
	README.ja \
	m4 \
	$(M4_MACROS) \
	$(PACKAGE_TARNAME).spec.in \
	$(PACKAGE_TARNAME).spec
