%define bindir /usr/bin
%define rpmrelease  %nil


### Work-around the fact that OpenSUSE/SLES _always_ defined both :-/
%if 0%{?sles_version} == 0
%undefine sles_version
%endif

Name:			ebiso
Version:		0.1.4
Release:		1%{?rpmrelease}%{?dist}
Summary:		UEFI bootable ISO image creator
License:		GPLv3

Group:			Applications/System
URL:			https://github.com/gozora/ebiso
Source:			%{name}-%{version}.tar.gz
Vendor:			Vladimir (sodoma) Gozora
Packager:		Vladimir (sodoma) Gozora
BuildRoot:		%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
ExclusiveArch:		x86_64

%description
UEFI bootable ISO image creator


%prep
%setup -q

%build
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%{__make} install DESTDIR="%{buildroot}"

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root,0755)
%{bindir}/ebiso


%changelog
* Wed Oct 21 2015 Gratien D'haese <gratien.dhaese@gmail.com>
  Rewrote original spec file from Vladimir
