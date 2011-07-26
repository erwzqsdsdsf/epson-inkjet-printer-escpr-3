# epson-escpr.spec.in -- an rpm spec file templete
# Epson Inkjet Printer Driver (ESC/P-R) for Linux
# Copyright (C) 2000-2006 AVASYS CORPORATION.
# Copyright (C) Seiko Epson Corporation 2006-2011.
#  This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

%define drivername epson-inkjet-printer-escpr
%define driverstr epson-inkjet-printer-escpr
%define distribution LSB
%define manufacturer EPSON
%define supplier %{drivername}
%define lsbver 3.2

%define extraversion -1lsb%{lsbver}
%define supplierstr Seiko Epson Corporation

Name: epson-inkjet-printer-escpr
Version: 1.0.4
Release: 1lsb%{lsbver}
Source0: %{name}-%{version}-%{release}.tar.gz
License: GPL
Vendor: Seiko Epson Corporation
URL: http://avasys.jp/english/linux_e/
Packager: Seiko Epson Corporation <linux-epson-inkjet@avasys.jp>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}
Group: Applications/System
Requires: lsb >= %{lsbver}
BuildRequires: lsb-build-cc, lsb-build-c++, lsb-appchk
BuildRequires: perl, gzip
Summary: Epson Inkjet Printer Driver (ESC/P-R) for Linux

%description
This software is a filter program used with Common UNIX Printing
System (CUPS) from the Linux. This can supply the high quality print
with Seiko Epson Color Ink Jet Printers.

This product supports only EPSON ESC/P-R printers. This package can be
used for all EPSON ESC/P-R printers.

For detail list of supported printer, please refer to below site:
http://avasys.jp/english/linux_e/


# Packaging settings
%install_into_opt

%prep
rm -rf $RPM_BUILD_DIR/%{name}-%{version}-%{release}
%setup -q

%build
%configure
make CUPS_SERVER_DIR=%{_cupsserverbin} CUPS_PPD_DIR=%{_cupsppd} libdir=%{_libdir} pkgdatadir=%{_datadir}

%install
rm -rf %{buildroot}

# Make directories
# install -d %{buildroot}%{_cupsppd}/%{manufacturer}
install -d %{buildroot}%{_cupsserverbin}/filter
install -d %{buildroot}%{_libdir}
install -d %{buildroot}%{_datadir}
install -d %{buildroot}%{_docdir}

make install CUPS_SERVER_DIR=%{buildroot}%{_cupsserverbin} CUPS_PPD_DIR=%{buildroot}%{_cupsppd} libdir=%{buildroot}%{_libdir} pkgdatadir=%{buildroot}%{_datadir}

%adjust_ppds
# # replace adjust_ppds because of difference of PPD localization
# ( cd %{buildroot}%{_cupsppd}
#     # Uncompress the PPDs, as we will do some modifications on them
#     for f in `find . -name '*.ppd.gz' -print`; do
#         gunzip $f;
#     done
#     # Make CUPS filter paths in the PPDs absolute
#     for f in `find . -name '*.ppd' -print`; do
#         perl -p -i -e 's:(\*cupsFilter\:\s*\"\S+\s+\d+\s+)(\S+\"):$1%{_cupsserverbin}/filter/$2:' $f
#         mv $f %{buildroot}%{_cupsppd}/%{manufacturer}/
#         gzip -9 %{buildroot}%{_cupsppd}/%{manufacturer}/$f
#     done
# )


install -m 644 README README.ja COPYING AUTHORS NEWS %{buildroot}%{_docdir}

# replace remove_devel_files. Remove only *.a and *.la files.
rm -f %{buildroot}%{_libdir}/*.a
rm -f %{buildroot}%{_libdir}/*.la
# remove debug symbol for lsb-appchk
strip -d %{buildroot}%{_libdir}/*.so

%pre
%create_opt_dirs

%post
/sbin/ldconfig
%set_ppd_links
%update_ppds_fast
%restart_cups

%postun
/sbin/ldconfig
%not_on_rpm_update
%remove_ppd_links
%restart_cups
%end_not_on_rpm_update

%clean
make clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%if %{optinstall}
%{_prefix}
%else
%{_cupsserverbin}/filter/*
%{_cupsppd}
%{_libdir}
%{_datadir}/%{name}
%endif
%docdir %{_docdir}
