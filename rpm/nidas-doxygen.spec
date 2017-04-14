%define nidas_prefix /opt/nidas

Summary: Doxygen documentation for NIDAS
Name: nidas-doxygen
Version: %{gitversion}
Release: %{releasenum}
License: GPL
Group: Applications/Engineering
BuildArch: noarch
Url: https://github.com/ncareol/nidas
Vendor: UCAR
# Source: %{name}-%{version}.tar.gz
Source: https://github.com/ncareol/%{name}/archive/master.tar.gz#/%{name}-%{version}.tar.gz
BuildRequires: doxygen graphviz
BuildRoot: %{_topdir}/%{name}-%{version}-root
# Allow this package to be relocatable to other places than /opt/nidas
# rpm --relocate /opt/nidas=/usr
Prefix: %{nidas_prefix}
%description
NIDAS source documentation generated by doxygen

%prep
%setup -q -c

%build

%install
rm -rf $RPM_BUILD_ROOT

cd src
scons -j 4 BUILDS=host PREFIX=${RPM_BUILD_ROOT}%{nidas_prefix} REPO_TAG=v%{version}-%{release} doxinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%{nidas_prefix}/doxygen

%changelog