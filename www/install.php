<?php
  include("header.inc");
?>

<h2>Installation from prebuilt binaries</h2>
SvxLink has been developed under RedHat Linux 9 and briefly tested under Fedora
Core 1. SvxLink should be easy to install under these Linux versions since there
are binary packages available.
<P>
SvxLink have a few dependencies. Most of them should be installed by default on
a normal Linux workstation. The Qtel application requires X/Qt but the svxlink
server is a pure console application.
The only external dependency is the <em>libsigc++</em> library. Only version
1.0 will work. The one I use is version 1.0.4. RPMS for RH9 and FC1 can be
downloaded from the
<A href="http://atrpms.physik.fu-berlin.de/dist/rh9/libsigc++/">ATrpms</A>
site. Make sure to pick the 1.0.4 version.
<P>
Download the SvxLink RPMS using the
<A href="https://sourceforge.net/project/showfiles.php?group_id=84813">
SourceForge download system
</A>. Make sure you pick the RPMS that are built for your
system (fc1=Fedora Core 1, rh9=Red Hat 9). You will at least need the
<em>libasync</em> and the <em>echolib</em> RPMS. To run the EchoLink client GUI,
download the <em>qtel</em> package. If it is the svxlink server you are
interrested in, download the <em>svxlink-server</em> package.
<P>
Install all of the above mentioned packages (libsigc++, libasync, echolib,
qtel and/or svxlink-server) on your system using the "rpm -Uvh" command.
<P>
If you are going to run the svxlink server, check out the configuration
description below before starting it.
<P>
After the configuration has been done, start the server by typing
<em>svxlink</em> at the command prompt. It is possible to simulate DTMF input by
pressing the 1-9, A-D, *, # keys. Have a look at the
<A href="svxlink_usage.php">user documentation</A> to begin testing the server.


<h2>Installation from source</h2>
[To be written]


<h2>Hardware</h2>
[To be written]


<h2>SvxLink server configuration</h2>
During the svxlink-server package installation the <em>/etc/svxlink.conf</em>
default configuration file is installed. Optionally, copy this file to the
home directory of the user you are going to run the svxlink server as and
rename it to ".svxlinkrc" or just use the /etc/svxlink.conf file.
Edit the configuration file to enter your personal data.
<P>
There is a voice greeting message sent to connecting echolink stations. This
message should be replaced with a personal one. I have used the <em>rec</em>
application that is included in the <em>sox</em> package to record the sounds.
Any tool capable of recording raw sound files using 8kHz sampling rate and 16
bit signed samples is ok to use. Another alternative is to record the sound
files using your favorite tool and then convert them to the correct format using
the sox application. The rec application should be used as shown below.
<PRE>
  rec -r8000 -sw /usr/share/svxlink/sounds/EchoLink/greeting.raw
  play -r8000 -sw /usr/share/svxlink/sounds/EchoLink/greeting.raw
</PRE>

The configuration file is in the famous INI-file format. It looks like the
following:
<PRE>
  [SECTION1]
  VALUE1=1
  VALUE2="TWO"
  VAULE3="Multi "
         "line"
  
  [SECTION2]
  VALUE1=2
</PRE>

The first section is the <b>GLOBAL</b> section. This section contains some
application global configuration data.

<DL>
  <DT>MODULE_PATH</DT>
  <DD>
    Specify where the SvxLink modules can be found. The default is
    /usr/lib/svxlink
  </DD>
</DL>

The next section is the <b>SimlexLogic</b> section. This section contains
configuration data for one logic controller. Right now, there can be only one
logic controller but in the future there may be more.

<DL>
  <DT>RX</DT>
  <DD>
    Specify the section name of the receiver to use.
  </DD>
  
  <DT>TX</DT>
  <DD>
    Specify the section name of the transmitter to use.
  </DD>
  
  <DT>MODULES</DT>
  <DD>
    Specify a comma separated list of sections for the modules to load.
  </DD>
  
  <DT>CALLSIGN</DT>
  <DD>
    Specify the callsign that should be announced on the radio interface.
  </DD>
  
  <DT>SOUNDS</DT>
  <DD>
    Specify the path to the directory where the sound samples are stored. The
    default is /usr/share/svxlink/sounds.
  </DD>
</DL>

A receiver section (called <b>Rx1</b> in the default configuration file) is
used to specify the configuration for a receiver.

<DL>
  <DT>AUDIO_DEV</DT>
  <DD>
    Specify the audio device to use. Normally <em>/dev/dsp</em>.
  </DD>

  <DT>VOX_FILTER_DEPTH</DT>
  <DD>
    The number of samples to use for calculating the mean value.
  </DD>

  <DT>VOX_LIMIT</DT>
  <DD>
    The threshold that the mean value of the samples must exceed for the
    squlech to be considered open.
  </DD>

  <DT>VOX_HANGTIME</DT>
  <DD>
    How long, in milliseconds, the squelch will stay open efter the sample
    mean value have fallen below the threshold (VOX_LIMIT).
  </DD>
</DL>

A transmitter section (called <b>Tx1</b> in the default configuration file) is
used to specify the configuration for a transmitter.

<DL>
  <DT>AUDIO_DEV</DT>
  <DD>
    Specify the audio device to use. Normally <em>/dev/dsp</em>.
  </DD>

  <DT>PTT_PORT</DT>
  <DD>
    Specify the serial port that the PTT is connected to.
  </DD>

  <DT>PTT_PIN</DT>
  <DD>
    Specify the pin in the serial port that the PTT is connected to. The
    possible values are RTS or DTR.
  </DD>

  <DT>TIMEOUT</DT>
  <DD>
    This is a feature that will prevent the transmitter from getting stuck
    transmitting. Specify the number of seconds before the transmitter is turned
    off.
  </DD>
</DL>


A module section have some general configuration variables and then there
are some module specific configuration variables. The general configuration
variables are listed below.

<DL>
  <DT>NAME</DT>
  <DD>
    The base name of the module. For example if this configuration variable is
    set to Foo, the core will look for a plugin called ModuleFoo.so.
  </DD>

  <DT>ID</DT>
  <DD>
    Specify the module identification. This is the number used to access the
    module from the radio interface.
  </DD>

  <DT>TIMEOUT</DT>
  <DD>
    Specify the timeout time, in seconds, after which a module will be
    automatically deactivated if there has been no activity.
  </DD>
</DL>


Specific configuration variables for the <b>EchoLink</b> module.

<DL>
  <DT>ALLOW_IP</DT>
  <DD>
    Use this variable very carefully. Connections originating from the given
    subnet will not be checked against the EchoLink directory server. A typical
    use of this is if you want to connect to your own svxlink server and both
    the server and you are behind a IP masquerading firewall.
  </DD>

  <DT>SERVER</DT>
  <DD>
    The EchoLink directory server to use.
  </DD>

  <DT>CALLSIGN</DT>
  <DD>
    The callsign to use to login to the EchoLink directory server.
  </DD>

  <DT>SYSOPNAME</DT>
  <DD>
    The name of the person that is responsible for this system.
  </DD>

  <DT>PASSWORD</DT>
  <DD>
    The EchoLink directory server password to use.
  </DD>

  <DT>LOCATION</DT>
  <DD>
    The location of the station.
  </DD>

  <DT>DESCRIPTION</DT>
  <DD>
    A longer description that is sent to remote stations upon connection.
  </DD>
</DL>


<?php include("footer.inc"); ?>

