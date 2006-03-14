
# Release date (YYMMDD)
%define RELEASE_DATE		051202
%define RELEASE_NO		1

# Version for the Qtel application
%define QTEL			0.9.2
%define QTEL_RPM_RELEASE	1

# Version for the EchoLib library
%define ECHOLIB			0.11.0
%define ECHOLIB_RPM_RELEASE	1

# Version for the Async library
%define LIBASYNC		0.13.0
%define LIBASYNC_RPM_RELEASE	1

# SvxLink versions
%define SVXLINK			0.7.0
%define SVXLINK_RPM_RELEASE	1


Summary: The SvxLink project files
Name: svxlink
Version: %{RELEASE_DATE}
Release: %{RELEASE_NO}
License: GPL
Group: Ham
Packager: Tobias Blomberg (SM0SVX)
BuildRoot: %{_tmppath}/%{name}-root-%(id -u -n)
Source0: %{name}-%{version}.tar.gz
Source1: sounds-%{version}.tar.gz

%description
The SvxLink project is a multi purpose voice services system for
ham radio use. For example, EchoLink connections are supported.
Also, the SvxLink server can act as a repeater controller.

This is the source package which can be used to build binary RPMS:

	rpmbuild --rebuild svxlink-%{RELEASE_DATE}-%{RELEASE_NO}.src.rpm


%prep
rm -rf ${RPM_BUILD_ROOT}
%setup -n svxlink
%setup -n svxlink -a1 -D


%build
make release
doxygen doxygen.async
doxygen doxygen.echolib


%install
INSTALL_ROOT=${RPM_BUILD_ROOT} NO_CHOWN=1 make install
find ${RPM_BUILD_ROOT}/usr/lib -type l -exec rm {} \;
mkdir -p ${RPM_BUILD_ROOT}/usr/share/svxlink
cp -a sounds ${RPM_BUILD_ROOT}/usr/share/svxlink/


%clean
rm -rf ${RPM_BUILD_ROOT}



%package -n svxlink-server
Summary: SvxLink - A general purpose voice services system
Version: %{SVXLINK}
Release: %{SVXLINK_RPM_RELEASE}
Group: Ham
Requires: libasync echolib

%description -n svxlink-server
The SvxLink server is a general purpose voice services system for ham radio use.
Each voice service is implemented as a plugin called a module. Some examples of
voice services are: Help system, Simplex repeater, EchoLink connection.

The core of the system handle the radio interface and is quite flexible as well.
It can act both as a simplex node and as a repeater controller.

%files -n svxlink-server
%defattr(644,root,root)
%config(noreplace) /etc/svxlink.conf
%config(noreplace) /etc/TclVoiceMail.conf
%attr(-,root,root) /usr/share/svxlink/sounds
%doc svxlink/ChangeLog
%defattr(755,root,root)
%dir /var/spool/svxlink/voice_mail
/usr/share/svxlink/event_test.tcl
/usr/bin/svxlink
/usr/bin/dtmf_plot
/usr/lib/svxlink/ModuleEchoLink.so
/usr/lib/svxlink/ModuleHelp.so
/usr/lib/svxlink/ModuleParrot.so
/usr/lib/svxlink/ModuleTcl.so
/usr/lib/svxlink/logtime.pl
%exclude /usr/include/svxlink/AudioPacer.h
%exclude /usr/include/svxlink/Module.h
%exclude /usr/include/svxlink/MsgHandler.h
%exclude /usr/include/svxlink/EventHandler.h



%package -n qtel
Summary: The QT EchoLink Client
Version: %{QTEL}
Release: %{QTEL_RPM_RELEASE}
Group: Ham
Requires: libasync echolib

%description -n qtel
This package contains Qtel, the Qt EchoLink client. It is an implementation of
the EchoLink software in Qt. This is only an EchoLink client, that is it can not
be connected to a transciever to create a link. If it is a pure link node you
want, install the svxlink-server package.

%files -n qtel
%defattr(644,root,root)
%doc qtel/ChangeLog
/usr/share/qtel/sounds/connect.raw
/usr/share/qtel/translations/qtel_sv.qm
%attr(755,root,root) /usr/bin/qtel


%package -n echolib
Summary: EchoLink communications library
Version: %{ECHOLIB}
Release: %{ECHOLIB_RPM_RELEASE}
Group: Ham
Requires: libasync

%description -n echolib

%files -n echolib
%defattr(644,root,root)
%doc echolib/ChangeLog
%defattr(755,root,root)
/usr/lib/libecholib-%{version}.so

%post -n echolib
/sbin/ldconfig



%package -n echolib-devel
Summary: Development files for the EchoLink communications library
Version: %{ECHOLIB}
Release: %{ECHOLIB_RPM_RELEASE}
Group: Ham

%description -n echolib-devel

%files -n echolib-devel
%defattr(644,root,root)
%doc doc/echolib/html
/usr/include/svxlink/EchoLinkDirectory.h
/usr/include/svxlink/EchoLinkDispatcher.h
/usr/include/svxlink/EchoLinkQso.h
/usr/include/svxlink/EchoLinkStationData.h
/usr/lib/libecholib.a


%package -n libasync
Summary: SvxLink Async libs
Version: %{LIBASYNC}
Release: %{LIBASYNC_RPM_RELEASE}
Group: Ham

%description -n libasync
The Async library files.

%files -n libasync
%defattr(644,root,root)
%doc async/ChangeLog
%defattr(755,root,root)
/usr/lib/libasynccore-%{version}.so
/usr/lib/libasynccpp-%{version}.so
/usr/lib/libasyncqt-%{version}.so

%post -n libasync
/sbin/ldconfig



%package -n libasync-devel
Summary: SvxLink Async development files
Version: %{LIBASYNC}
Release: %{LIBASYNC_RPM_RELEASE}
Group: Ham

%description -n libasync-devel
The Async library development files

%files -n libasync-devel
%defattr(644,root,root)
%doc doc/async/html
/usr/include/svxlink/AsyncApplication.h
/usr/include/svxlink/AsyncAudioIO.h
/usr/include/svxlink/AsyncConfig.h
/usr/include/svxlink/AsyncCppApplication.h
/usr/include/svxlink/AsyncDnsLookup.h
/usr/include/svxlink/AsyncFdWatch.h
/usr/include/svxlink/AsyncIpAddress.h
/usr/include/svxlink/AsyncQtApplication.h
/usr/include/svxlink/AsyncSampleFifo.h
/usr/include/svxlink/AsyncTcpClient.h
/usr/include/svxlink/AsyncTcpConnection.h
/usr/include/svxlink/AsyncTcpServer.h
/usr/include/svxlink/AsyncTimer.h
/usr/include/svxlink/AsyncUdpSocket.h
/usr/include/svxlink/AsyncSerial.h
/usr/lib/libasynccore.a
/usr/lib/libasynccpp.a
/usr/lib/libasyncqt.a
