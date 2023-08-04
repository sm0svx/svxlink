
# Release date (YYMMDD)
%define RELEASE_DATE              080730
%define RELEASE_NO                1

# Version for the Qtel application
%define QTEL                      0.11.1
%define QTEL_RPM_RELEASE          1

# Version for the EchoLib library
%define ECHOLIB                   0.13.0
%define ECHOLIB_RPM_RELEASE       2

# Version for the Async library
%define LIBASYNC                  0.16.1
%define LIBASYNC_RPM_RELEASE      1

# SvxLink versions
%define SVXLINK                   0.10.1
%define SVXLINK_RPM_RELEASE       1
%define REMOTETRX                 0.1.0
%define REMOTETRX_RPM_RELEASE     2
%define SIGLEVDETCAL              0.1.0
%define SIGLEVDETCAL_RPM_RELEASE  2


Summary: The SvxLink project files
Name: svxlink
Version: %{RELEASE_DATE}
Release: %{RELEASE_NO}.%{dist}
License: GPL
Group: Applications/Ham Radio
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
distribution you're compiling SvxLink for, like "fc8" if you're compiling
SvxLink for Fedora 8.


%prep
%setup -q
%setup -q -a1 -D


%build
make release
doxygen doxygen.async
doxygen doxygen.echolib


%install
make INSTALL_ROOT=%{buildroot} NO_CHOWN=1 LIB_INSTALL_DIR=%{_libdir} \
        INC_INSTALL_DIR=%{_includedir}/svxlink BIN_INSTALL_DIR=%{_bindir} \
        SBIN_INSTALL_DIR=%{_sbindir} install
find %{buildroot}%{_libdir} -type l -exec rm {} \;
mkdir -p %{buildroot}/usr/share/svxlink
cp -a sounds %{buildroot}/usr/share/svxlink/
mkdir -p %{buildroot}/var/log
touch %{buildroot}/var/log/svxlink
touch %{buildroot}/var/log/svxlink.{1,2,3,4}


%clean
rm -rf %{buildroot}



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
%dir /etc/svxlink.d
/usr/share/svxlink/sounds/Core
/usr/share/svxlink/sounds/Default
/usr/share/svxlink/sounds/DtmfRepeater
/usr/share/svxlink/sounds/EchoLink
/usr/share/svxlink/sounds/Help
/usr/share/svxlink/sounds/Parrot
/usr/share/svxlink/sounds/TclVoiceMail
%config /usr/share/svxlink/sounds/events.tcl
%config /usr/share/svxlink/sounds/events.d/*
%defattr(644,root,root)
%config(noreplace) /etc/svxlink.conf
%config(noreplace) /etc/svxlink.d/*
%config(noreplace) /etc/TclVoiceMail.conf
%config(noreplace) /etc/logrotate.d/svxlink
%config(noreplace) /etc/remotetrx.conf
%doc svxlink/ChangeLog
/etc/sysconfig/svxlink
/etc/sysconfig/remotetrx
%config(noreplace) /etc/security/console.perms.d/90-svxlink.perms
%config(noreplace) /etc/udev/rules.d/10-svxlink.rules
/usr/share/man/man1/svxlink.1.gz
/usr/share/man/man1/remotetrx.1.gz
/usr/share/man/man1/siglevdetcal.1.gz
/usr/share/man/man5/svxlink.conf.5.gz
/usr/share/man/man5/remotetrx.conf.5.gz
/usr/share/man/man5/Module*.5.gz
%dir %attr(755,svxlink,daemon) /var/spool/svxlink
%dir %attr(755,svxlink,daemon) /var/spool/svxlink/voice_mail
%defattr(755,root,root)
/usr/share/svxlink/event_test.tcl
%{_bindir}/svxlink
%{_bindir}/dtmf_plot
%{_bindir}/remotetrx
%{_bindir}/siglevdetcal
%{_libdir}/svxlink/Module*.so
/etc/init.d/svxlink
/etc/init.d/remotetrx
%ghost /var/log/*
%exclude %{_includedir}/svxlink/Module.h
%exclude %{_includedir}/svxlink/MsgHandler.h
%exclude %{_includedir}/svxlink/EventHandler.h
%exclude %{_includedir}/svxlink/NetTrxMsg.h
%exclude %{_includedir}/svxlink/Rx.h
%exclude %{_includedir}/svxlink/Tx.h
%exclude %{_libdir}/libtrx.a



%package -n qtel
Summary: The QT EchoLink Client
Version: %{QTEL}
Release: %{QTEL_RPM_RELEASE}.%{dist}
Group: Applications/Ham Radio
Requires: libasync echolib hicolor-icon-theme

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
%attr(755,root,root) %{_bindir}/qtel
/usr/share/icons/hicolor/128x128/apps/qtel.png
/usr/share/applications/qtel.desktop
/usr/share/metainfo/org.svxlink.Qtel.metainfo.xml


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
%{_libdir}/libecholib-%{version}.so



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
%{_includedir}/svxlink/EchoLinkDirectory.h
%{_includedir}/svxlink/EchoLinkDispatcher.h
%{_includedir}/svxlink/EchoLinkQso.h
%{_includedir}/svxlink/EchoLinkStationData.h
%{_libdir}/libecholib.a


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
%{_libdir}/libasynccore-%{version}.so
%{_libdir}/libasynccpp-%{version}.so
%{_libdir}/libasyncqt-%{version}.so
%{_libdir}/libasyncaudio-%{version}.so



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
%{_includedir}/svxlink/AsyncApplication.h
%{_includedir}/svxlink/AsyncAudioIO.h
%{_includedir}/svxlink/AsyncConfig.h
%{_includedir}/svxlink/AsyncCppApplication.h
%{_includedir}/svxlink/AsyncDnsLookup.h
%{_includedir}/svxlink/AsyncFdWatch.h
%{_includedir}/svxlink/AsyncIpAddress.h
%{_includedir}/svxlink/AsyncQtApplication.h
%{_includedir}/svxlink/AsyncTcpClient.h
%{_includedir}/svxlink/AsyncTcpConnection.h
%{_includedir}/svxlink/AsyncTcpServer.h
%{_includedir}/svxlink/AsyncTimer.h
%{_includedir}/svxlink/AsyncUdpSocket.h
%{_includedir}/svxlink/AsyncSerial.h
%{_includedir}/svxlink/AsyncAudioAmp.h
%{_includedir}/svxlink/AsyncAudioDelayLine.h
%{_includedir}/svxlink/AsyncAudioPassthrough.h
%{_includedir}/svxlink/AsyncAudioSelector.h
%{_includedir}/svxlink/AsyncAudioValve.h
%{_includedir}/svxlink/AsyncAudioClipper.h
%{_includedir}/svxlink/AsyncAudioCompressor.h
%{_includedir}/svxlink/AsyncAudioFilter.h
%{_includedir}/svxlink/AsyncAudioProcessor.h
%{_includedir}/svxlink/AsyncAudioSink.h
%{_includedir}/svxlink/AsyncAudioSource.h
%{_includedir}/svxlink/AsyncAudioSplitter.h
%{_includedir}/svxlink/AsyncAudioDebugger.h
%{_includedir}/svxlink/AsyncAudioDecimator.h
%{_includedir}/svxlink/AsyncAudioFifo.h
%{_includedir}/svxlink/AsyncAudioInterpolator.h
%{_includedir}/svxlink/AsyncAudioPacer.h
%{_includedir}/svxlink/AsyncAudioReader.h
%{_includedir}/svxlink/AsyncAudioMixer.h
%{_includedir}/svxlink/SigCAudioSink.h
%{_includedir}/svxlink/SigCAudioSource.h
%{_libdir}/libasynccore.a
%{_libdir}/libasynccpp.a
%{_libdir}/libasyncqt.a
%{_libdir}/libasyncaudio.a

%changelog
* Fri Aug 04 2023 Daniel Rusek <mail@asciiwolf.com>
- Added AppStream metainfo into files.
- Added png icon into files.
- Removed xpm icon from files.
* Wed Jul 30 2008 Tobias Blomberg (SM0SVX) <sm0svx@users.sourceforge.net>
- Fixed a couple of things that rpmlint complained about.
- Making use of some directory macros (_libdir, _includedir, _bindir, _sbindir).
- Updated versions for release 080730.
* Wed May 18 2008 Tobias Blomberg (SM0SVX) <sm0svx@users.sourceforge.net>
- Updated versions.
- Added the remotetrx application and removed the remoterx application.
- The /etc/svxlink.d directory is now created with the correct permissions.
* Wed Jan 02 2008 Tobias Blomberg (SM0SVX) <sm0svx@users.sourceforge.net>
- Updated versions
- The root directory of the source archive now includes the version.
* Sat Apr 14 2007 Tobias Blomberg (SM0SVX) <sm0svx@users.sourceforge.net>
- Added this ChangeLog :-)
- Improved pre/post scriptlets
- Added a dist variable that should be set to a short form of the distribution
  name (e.g. fc6 for Fedora Core 6).
- Added new project files to the file lists.
- Added new tags: vendor, url.
- Changed the group tags to something more "standard" but it seems like
  there is no standard for ham radio.

