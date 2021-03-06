Summary: Metapackage for installing Arcom Embedded Linux (AEL) for NIDAS development.
Name: nidas-ael
Version: 1.0
Release: 9
License: GPL
Group: Development
BuildArch: noarch

Requires: ael-base

Requires: xmlrpc++-cross-armbe-linux

Requires: xerces-c-cross-armbe-linux
Requires: xerces-c-cross-devel-armbe-linux

Requires: bzip2-cross-armbe-linux

%description
Package with dependencies needed for NIDAS cross development for
Arcom Embedded Linux (AEL) targets (Vulcan).

%files

%changelog
* Mon Jan 16 2017 Gordon Maclean <maclean@ucar.edu> 1.0-9
- Removed requires for Viper (arm)
* Wed Aug 14 2013 Gordon Maclean <maclean@ucar.edu> 1.0-8
- Removed requires for viper,vulcan,titan kernel source, which
- are already required in ael-base.
* Tue Feb  5 2013 Gordon Maclean <maclean@ucar.edu> 1.0-7
- Updated to 2.6.35 kernels for viper, titan.
* Wed Mar 21 2012 Gordon Maclean <maclean@ucar.edu> 1.0-6
- Added lsb-arcom-titan-linux-source-2.6.20.18-ael4
* Sat Nov 28 2009 Gordon Maclean <maclean@ucar.edu> 1.0-5
- Added bzip2-cross-{arm,armbe}
* Tue May 12 2009 Gordon Maclean <maclean@ucar.edu> 1.0-4
- Removed xmlrpc++ and xerces-c-devel dependencies and udev rules.
* Thu Feb 21 2008 Gordon Maclean <maclean@ucar.edu> 1.0-3
- Added etc/udev/rules.d/99-nidas.rules
- This eventually should go in a separate nidas-base package
* Thu Jan 31 2008 Gordon Maclean <maclean@ucar.edu> 1.0-2
- Split base AEL stuff off to ael-base
- Added kernel source trees
* Tue Jan 15 2008 Gordon Maclean <maclean@ucar.edu> 1.0-1
- Initial hack

