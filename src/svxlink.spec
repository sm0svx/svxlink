
# Release date (YYMMDD)
%define RELEASE_DATE		070415
%define RELEASE_NO		1

# Version for the Qtel application
%define QTEL			0.10.0
%define QTEL_RPM_RELEASE	1

# Version for the EchoLib library
%define ECHOLIB			0.12.0
%define ECHOLIB_RPM_RELEASE	1

# Version for the Async library
%define LIBASYNC		0.14.0
%define LIBASYNC_RPM_RELEASE	1

# SvxLink versions
%define SVXLINK			0.8.0
%define SVXLINK_RPM_RELEASE	1
%define REMOTERX      	      	0.1.0
%define REMOTERX_RPM_RELEASE  	1


Summary: The SvxLink project files
Name: svxlink
Version: %{RELEASE_DATE}
Release: %{RELEASE_NO}.%{dist}
License: GPL
Group: Applications/Ham Radio
Packager: Tobias Blomberg (SM0SVX) <sm0svx@users.sourceforge.net>
Vendor: Tobias Blomberg (SM0SVX)
URL: http://svxlink.sourceforge.net/
BuildRoot: %{_tmppath}/%{name}-root-%(id -u -n)
Source0: %{name}-%{version}.tar.gz
Source1: sounds-%{version}.tar.gz

%description
The SvxLink project is a multi purpose voice services system for
ham radio use. For example, EchoLink connections are supported.
Also, the SvxLink server can act as a repeater controller.

This is the source package which can be used to build binary RPMS:

	rpmbuild --rebuild svxlink-%{RELEASE_DATE}-%{RELEASE_NO}.src.rpm

You also need to setup the %%dist variable to something identifying the
distribution you're compiling SvxLink for, like "fc6" if you're compiling
SvxLink for Fedora Core 6.


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
mkdir -p ${RPM_BUILD_ROOT}/var/log
touch ${RPM_BUILD_ROOT}/var/log/svxlink
touch ${RPM_BUILD_ROOT}/var/log/svxlink.{1,2,3,4}


%clean
rm -rf ${RPM_BUILD_ROOT}



%package -n svxlink-server
Summary: SvxLink - A general purpose voice services system
Version: %{SVXLINK}
Release: %{SVXLINK_RPM_RELEASE}.%{dist}
Group: Applications/Ham Radio
Requires: libasync echolib

%description -n svxlink-server
The SvxLink server is a general purpose voice services system for ham radio use.
Each voice service is implemented as a plugin called a module. Some examples of
voice services are: Help system, Simplex repeater, EchoLink connection.

The core of the system handle the radio interface and is quite flexible as well.
It can act both as a simplex node and as a repeater controller.

%pre -n svxlink-server
if ! egrep -q "^svxlink" /etc/passwd; then
  /usr/sbin/useradd -r -n -g daemon -s /sbin/nologin -d / -c "SvxLink Daemon" svxlink
fi

%post -n svxlink-server
/sbin/chkconfig --add svxlink

%preun -n svxlink-server
if [ $1 = 0 ]; then
  /sbin/service svxlink stop >/dev/null 2>&1 || :
  /sbin/chkconfig --del svxlink
fi

%postun -n svxlink-server
if [ $1 = 0 ]; then
  /usr/sbin/userdel svxlink
fi

%files -n svxlink-server
%defattr(-,root,root)
%dir /usr/share/svxlink
/usr/share/svxlink/sounds
%config /usr/share/svxlink/sounds/events.tcl
%config /usr/share/svxlink/sounds/events.d/*
%defattr(644,root,root)
%config(noreplace) /etc/svxlink.conf
%config(noreplace) /etc/svxlink.d
%config(noreplace) /etc/TclVoiceMail.conf
%config(noreplace) /etc/logrotate.d/svxlink
%doc svxlink/ChangeLog
/etc/sysconfig/svxlink
%config(noreplace) /etc/security/console.perms.d/90-svxlink.perms
%config(noreplace) /etc/udev/rules.d/10-svxlink.rules
/usr/share/man/man1/svxlink.1.gz
/usr/share/man/man1/remoterx.1.gz
/usr/share/man/man5/svxlink.conf.5.gz
/usr/share/man/man5/remoterx.conf.5.gz
/usr/share/man/man5/Module*.5.gz
%dir %attr(755,svxlink,daemon) /var/spool/svxlink
%dir %attr(755,svxlink,daemon) /var/spool/svxlink/voice_mail
%defattr(755,root,root)
/usr/share/svxlink/event_test.tcl
/usr/bin/svxlink
/usr/bin/dtmf_plot
/usr/bin/remoterx
/usr/lib/svxlink/Module*.so
/etc/init.d/svxlink
%ghost /var/log/*
%exclude /usr/include/svxlink/AudioPacer.h
%exclude /usr/include/svxlink/Module.h
%exclude /usr/include/svxlink/MsgHandler.h
%exclude /usr/include/svxlink/EventHandler.h
%exclude /usr/include/svxlink/NetRxMsg.h
%exclude /usr/include/svxlink/Rx.h
%exclude /usr/lib/librx.a



%package -n qtel
Summary: The QT EchoLink Client
Version: %{QTEL}
Release: %{QTEL_RPM_RELEASE}.%{dist}
Group: Applications/Ham Radio
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
/usr/share/icons/link.xpm
/usr/share/applications/qtel.desktop


%package -n echolib
Summary: EchoLink communications library
Version: %{ECHOLIB}
Release: %{ECHOLIB_RPM_RELEASE}.%{dist}
Group: Libraries/Ham Radio
Requires: libasync

%description -n echolib
%{summary}

%post -n echolib -p /sbin/ldconfig
%postun -n echolib -p /sbin/ldconfig

%files -n echolib
%defattr(644,root,root)
%doc echolib/ChangeLog
%defattr(755,root,root)
/usr/lib/libecholib-%{version}.so



%package -n echolib-devel
Summary: Development files for the EchoLink communications library
Version: %{ECHOLIB}
Release: %{ECHOLIB_RPM_RELEASE}.%{dist}
Group: Development/Libraries/Ham Radio

%description -n echolib-devel
%{summary}

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
Release: %{LIBASYNC_RPM_RELEASE}.%{dist}
Group: Libraries/Ham Radio

%description -n libasync
The Async library files.

%post -n libasync -p /sbin/ldconfig
%postun -n libasync -p /sbin/ldconfig

%files -n libasync
%defattr(644,root,root)
%doc async/ChangeLog
%defattr(755,root,root)
/usr/lib/libasynccore-%{version}.so
/usr/lib/libasynccpp-%{version}.so
/usr/lib/libasyncqt-%{version}.so
/usr/lib/libasyncaudio-%{version}.so



%package -n libasync-devel
Summary: SvxLink Async development files
Version: %{LIBASYNC}
Release: %{LIBASYNC_RPM_RELEASE}.%{dist}
Group: Development/Libraries/Ham Radio

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
/usr/include/svxlink/AsyncAudioAmp.h
/usr/include/svxlink/AsyncAudioDelayLine.h
/usr/include/svxlink/AsyncAudioPassthrough.h
/usr/include/svxlink/AsyncAudioSelector.h
/usr/include/svxlink/AsyncAudioValve.h
/usr/include/svxlink/AudioClipper.h
/usr/include/svxlink/AudioCompressor.h
/usr/include/svxlink/AudioFilter.h
/usr/include/svxlink/AudioProcessor.h
/usr/include/svxlink/AudioSink.h
/usr/include/svxlink/AudioSource.h
/usr/include/svxlink/AudioSplitter.h
/usr/include/svxlink/SigCAudioSink.h
/usr/include/svxlink/SigCAudioSource.h
/usr/lib/libasynccore.a
/usr/lib/libasynccpp.a
/usr/lib/libasyncqt.a
/usr/lib/libasyncaudio.a

%changelog
* Sat Apr 14 2007 Tobias Blomberg (SM0SVX) <sm0svx@users.sourceforge.net>
- Added this ChangeLog :-)
- Improved pre/post scriptlets
- Added a dist variable that should be set to a short form of the distribution
  name (e.g. fc6 for Fedora Core 6).
- Added new project files to the file lists.
- Added new tags: vendor, url.
- Changed the group tags to something more "standard" but it seems like
  there is no standard for ham radio.

