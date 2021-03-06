# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.25
# 

Name:       sensorfw-qt5

# >> macros
# << macros

Summary:    Sensor Framework Qt5
Version:    0.7.2.4
Release:    0
Group:      System/Sensor Framework
License:    LGPLv2+
URL:        http://gitorious.org/sensorfw
Source0:    %{name}-%{version}.tar.bz2
Source1:    sensorfw-rpmlintrc
Source2:    sensord.service
Source3:    sensord-daemon-conf-setup
Source100:  sensorfw-qt5.yaml
Requires:   qt5-qtcore
Requires:   GConf-dbus
Requires:   %{name}-configs
Requires:   systemd
Requires(preun): systemd
Requires(post): /sbin/ldconfig
Requires(post): systemd
Requires(postun): /sbin/ldconfig
Requires(postun): systemd
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  pkgconfig(gconf-2.0)
#BuildRequires:  pkgconfig(mce)
Provides: sensord-qt5
Obsoletes:   sensorframework

%description
Sensor Framework provides an interface to hardware sensor drivers through logical sensors. This package contains sensor framework daemon and required libraries.


%package devel
Summary:    Sensor framework daemon libraries development headers
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   qt-devel

%description devel
Development headers for sensor framework daemon and libraries.


%package tests
Summary:    Unit test cases for sensord
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   testrunner-lite
Requires:   python

%description tests
Contains unit test cases for CI environment.


%package configs
Summary:    Sensorfw configuration files
Group:      System/Libraries
BuildArch:    noarch
Requires:   %{name} = %{version}
Provides:   sensord-config
Provides:   config-n900
Provides:   config-aava
Provides:   config-icdk
Provides:   config-ncdk
Provides:   config-oemtablet
Provides:   config-oaktraili
Provides:   config-u8500

%description configs
Sensorfw configuration files.



%prep
%setup -q -n %{name}-%{version}

# >> setup
# << setup

%build
unset LD_AS_NEEDED
# >> build pre
export LD_RUN_PATH=/usr/lib/sensord/
# << build pre

export QT_SELECT=5
%qmake5 
#\
#    CONFIG+=mce

make %{?jobs:-j%jobs}

# >> build post
# << build post

%install
rm -rf %{buildroot}
# >> install pre
# << install pre
export QT_SELECT=5
%qmake_install

# >> install post
install -D -m644 %{SOURCE2} $RPM_BUILD_ROOT/%{_lib}/systemd/system/sensord.service
install -D -m750 %{SOURCE3} $RPM_BUILD_ROOT/%{_bindir}/sensord-daemon-conf-setup

mkdir -p %{buildroot}/%{_lib}/systemd/system/basic.target.wants
ln -s ../sensord.service %{buildroot}/%{_lib}/systemd/system/basic.target.wants/sensord.service
# << install post


%preun
if [ "$1" -eq 0 ]; then
systemctl stop sensord.service
fi

%post
/sbin/ldconfig
systemctl daemon-reload
systemctl reload-or-try-restart sensord.service

%postun
/sbin/ldconfig
systemctl daemon-reload

%post tests
/sbin/ldconfig

%postun tests
/sbin/ldconfig

%files
%defattr(-,root,root,-)
# >> files
%attr(755,root,root)%{_sbindir}/sensord
%{_libdir}/sensord-qt5/*.so
%{_libdir}/libsensorfw*.so.*
%{_libdir}/libsensordatatypes*.so.*
%{_libdir}/libsensorclient*.so.*
%config %{_sysconfdir}/dbus-1/system.d/sensorfw.conf
%config %{_sysconfdir}/sensorfw/sensord.conf
%dir %{_sysconfdir}/sensorfw/sensord.conf.d/
%doc debian/copyright debian/README COPYING
/%{_lib}/systemd/system/sensord.service
/%{_lib}/systemd/system/basic.target.wants/sensord.service
%{_bindir}/sensord-daemon-conf-setup
# << files

%files devel
%defattr(-,root,root,-)
# >> files devel
%{_libdir}/libsensorfw*.so
%{_libdir}/libsensordatatypes*.so
%{_libdir}/libsensorclient*.so
%{_libdir}/pkgconfig/*
%{_includedir}/sensord-qt5/*
%{_datadir}/qt5/mkspecs/features/sensord.prf
# << files devel

%files tests
%defattr(-,root,root,-)
# >> files tests
%{_libdir}/libsensorfakeopen*.so
%{_libdir}/libsensorfakeopen*.so.*
%{_libdir}/sensord-qt5/testing/*
%attr(755,root,root)%{_datadir}/sensorfw-tests/*.p*
%attr(644,root,root)%{_datadir}/sensorfw-tests/*.xml
%attr(644,root,root)%{_datadir}/sensorfw-tests/*.conf
%attr(755,root,root)%{_bindir}/*
# << files tests

%files configs
%defattr(-,root,root,-)
# >> files configs
%config %{_sysconfdir}/sensorfw/sensord.conf.d/*conf
%config %{_sysconfdir}/sensorfw/*conf
%exclude %{_sysconfdir}/sensorfw/sensord.conf
# << files configs
