Name:			ebiso
Version:		0.1.3
Release:		0
Summary:		UEFI bootable ISO image creator
License:		GPL
Group:			Productivity/Multimedia/CD/Record
Source: 		ebiso-0.1.3.tgz
URL:			https://github.com/gozora/ebiso
Distribution:		SuSE Linux Enterprise Server 11
Vendor:			Vladimir (sodoma) Gozora
Packager:		Vladimir (sodoma) Gozora
BuildArch:		x86_64

%description
UEFI bootable ISO image creator


%prep
%setup

%build
make

%install
make install

%files
/usr/local/bin/ebiso

%post
%postun
