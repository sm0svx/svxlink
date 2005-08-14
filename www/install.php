<?php
  $selected="install";
  include("header.inc");
?>

<A name="prebuilt"><h2>Installation from prebuilt binaries</h2></A>
SvxLink has been developed under RedHat Linux 9 and briefly tested under Fedora
Core 1. My own node is running under Fedora Core 2. SvxLink should be easy to
install under these Linux versions since there are binary packages available.
<P>
SvxLink have a few dependencies. Most of them should be installed by default on
a normal Linux workstation. The Qtel application requires X/Qt but the svxlink
server is a pure console application.
There are two external dependencies. The <em>libsigc++</em> library and the
<em>gsm</em> library. Only version 1.0 of the libsigc++ library will work.
The one I use is version 1.0.4. RPMS for RH9, FC1 and FC2 can be
downloaded from the
<A href="http://atrpms.net/">ATrpms</A> site. Make sure to pick the 1.0.4
version. RPMs for the gsm library can be found for FC1 and FC2 at the
<A href="http://freshrpms.net">FreshRPMS</A> site. An RPM for RH9 can be built
by downloading the src-rpm for the FC1 version. Build and install it by
executing the folowing commands:
<PRE>
rpmbuild --rebuild gsm-1.0.10-4.1.fc1.fr.src.rpm
rpm -Uvh /usr/src/redhat/RPMS/gsm-1.0.10-4.1.fc1.fr.i386.rpm
</PRE>
Download the SvxLink RPMS using the
<A href="https://sourceforge.net/project/showfiles.php?group_id=84813">
SourceForge download system
</A>. Make sure you pick the RPMS that are built for your
system (fc1=Fedora Core 1, fc2=Fedora Core 2, rh9=Red Hat 9). You will at least
need the <em>libasync</em> and the <em>echolib</em> RPMS. To run the EchoLink
client GUI, download the <em>qtel</em> package. If it is the svxlink server you
are interrested in, download the <em>svxlink-server</em> package.
<P>
Install all of the above mentioned packages (libsigc++, gsm, libasync, echolib,
qtel and/or svxlink-server) on your system using the "rpm -Uvh" command.
<P>
Now continue below reading the
<A href="#post-install">post install stuff</A> chapter.
<P>


<A name="source"><h2>Installation from source</h2></A>
If you are not running one of the distributions that there are prebuilt
binaries for, you will have to build the whole thing from source. You will still
need to satisfy the dependencies specified above. That is
<A href="http://libsigc.sourceforge.net/">libsigc++</A> (make sure you pick the
1.0.4 version!) and
<A href="http://kbs.cs.tu-berlin.de/~jutta/toast.html">gsm</A>. Maybe you can
find prebuilt binaries for these two libs for your distribution. Otherwise you
will just have to compile them as well.
<P>
To compile Qtel, the <A href="http://www.trolltech.com/">Qt widget toolkit</A>
and the X window system are needed. There is a good chance that these will
already be installed on your system. If Qt is not installed, find a prebuilt
package or compile it from source. If the X window system is not installed,
you're on your own...
<P>
Now download the sources for SvxLink from the
<A href="https://sourceforge.net/project/showfiles.php?group_id=84813">
sourceforge download area</A>. Find the <em>svxlink-YYMMDD.tar.gz</em> with the
latest date. If you are going to run the svxlink-server, you will also need the
<em>sounds-YYMMDD.tar.gz</em> with the matching date. Find a good spot to unpack
and compile the source and cd into that directory. Then do the following
(install must be done as user root):
<PRE>
tar xvzf svxlink-YYMMDD.tar.gz
cd svxlink
make
make install
</PRE>
If you are going to run the SvxLink server, unpack the sound files in a good
location. A good location could for example be /usr/share/svxlink/. As user
root, do the following:
<PRE>
cd /usr/share
mkdir svxlink
cd svxlink
tar xvzf /path-to-wherever-you-put-the-tar-file/sounds-YYMMDD.tar.gz
</PRE>
<P>
Now continue below reading the
<A href="#post-install">post install stuff</A> chapter.
<P>


<A name="post-install"><h2>Post install stuff</h2></A>
<strong>Note1:</strong> For Alsa based systems (like Fedora Core 2), the Alsa
OSS emulation is used for sound I/O. There is a bug in the emulation layer
which will make SvxLink/Qtel fail. To work around this bug, set the
environment variable ASYNC_AUDIO_NOTRIGGER to 1 before starting SvxLink/Qtel.
An example of how this is done is shown below (assuming you are running the bash shell = usually the default).
<pre>
export ASYNC_AUDIO_NOTRIGGER=1
qtel &
</pre>
Or on one line:
<pre>
ASYNC_AUDIO_NOTRIGGER=1 qtel &
</pre>
The environment variable setting will be lost on logout so the <em>export</em>
line is best put into the file ".bash_profile", which can be found in your home
directory.
<P>
Note that setting this environment variable when it is not needed can make
SvxLink/Qtel to stop working. Only set it if you are getting the error message
below.
<PRE>
SNDCTL_DSP_SETTRIGGER ioctl failed: Broken pipe
</PRE>
<P>
<strong>Note2:</strong> Make sure that no other audio applications are running
at the same time as SvxLink/Qtel. If another application has opened the sound
device, SvxLink/Qtel will hang until the device is closed by the other
application. Especially, if you are having problems with SvxLink/Qtel hanging,
check for sound servers like <em>artsd</em> and the like.
<P>
If you only are going to run Qtel, go directly to the
<A href="qtel_usage.php">Qtel User Docs</A>.
<P>
If you are going to run the svxlink server, first check out the
<A href="#server-config">configuration description</A> below before starting it.
After the configuration has been done, start the server by typing
<em>svxlink</em> at the command prompt. It is possible to simulate DTMF input by
pressing the 1-9, A-D, *, # keys. Have a look at the
<A href="svxlink_usage.php">user documentation</A> to begin testing the server.
To get help about command line options, start the svxlink server with the
<em>--help</em> switch.
<P>
<strong>Note3:</strong> To start the svxlink server in the background, use the
<em>--daemon</em> switch. Do not use "&amp;". This will make the server hang
when trying to read from standard input.
<P>


<A name="hardware"><h2>Hardware</h2></A>
To run the SvxLink server, some kind of hardware is needed to connect the
computer to the transciever. I have done very few experiments on this. My
hardware consists of just a cable. No active components needed. I have connected
the microphone input and the line out output on the computer to the transcievers
packet radio connector. This works quite well. However, there is a slight hum on
the signal. This can be fixed with an isolation transformer but I havn't gotten
around to buy one yet.
<P>
Typical EchoLink hardware should work with SvxLink as well. Have a look at the
<A href="http://www.echolink.org/interfaces.htm">EchoLink interfaces</A> page.
However, I have not tried any of these so there are no guarantees. SvxLink
cannot make use of an external DTMF detector.
<P>


<A name="audio-level-adj"><h2>Audio level adjustment</h2></A>
There are no audio level controls in SvxLink server nor Qtel. The levels must be
adjusted with an external tool like aumix, kmix, alsamixer or whatever your
favourite mixer is. Start one of the mixers and locate the controls to use for
adjusting the levels. The output level is adjusted using the two sliders Pcm
and Vol. The input level is adjusted using the IGain slider, not the
Mic or Input (line-in) slider. The latter two are used to adjust the monitoring
level of the two inputs. Set these two to zero. Select to use either the
microphone or the line-in input. Set the Pcm, Vol and Mic/Input sliders half way
up. Adjust the levels according to the instructions below.
<P>
To adjust the levels in Qtel, start by connecting to the *ECHOTEST* server. This
EchoLink server will echo back everything you send to it. Right after the
connection has been established, a greeting message will be played back. Adjust
the speaker level to a comfortable level using the Pcm and Vol sliders. Press
the PTT and say something and listen how it comes back. Adjust the Mic slider
until you are satisfied.
<P>
To adjust the levels for the SvxLink server, start it up and press the * key on
the keyboard. This will make the svxlink server identify itself. Do this a
couple of times and adjust the Pcm and Vol sliders to the highest volume
possible without distorsion. Now, activate the parrot module by pressing 1# on
the keyboard. Use another transmitter to make a short transmission. Listen to
the recorded audio and adjust the Mic slider (if using the microphone input) or
Input slider (if using the line-in input) to the highest level possible without
distorsion. Repeat util happy. Now try to press some DTMF digits and see if the
digits are detected. If not, try to adjust the input level up or down and try
again.


<A name="event-subsystem"><h2>The Event Handling Subsystem</h2></A>
There are a lot of sounds that should be played as a response to an event in
the SvxLink system. To make these sounds as configurable as possible there is
a programmable event handling subsystem. The programming language chosen for
this is TCL (Tool Command Language). For each event in SvxLink there is
a corresponding TCL function that is called when the event occurs. In this
function the normal action is to play a sound or a couple of sound clips. It is
of course also possible to use the full power of TCL to make all sorts of things
happen. For example execution of an external application, reading files with
information (e.g. DX, weather data etc), time based events (e.g. only do this
when the time is...).

The TCL event scripts are located under /usr/share/svxlink/sounds. The main
script is called events.tcl. When this script is loaded by the SvxLink server at
startup, it looks in a subdirectory called events.d. Any file that ends in
".tcl" in this directory will be read and should contain a TCL script. If you
have a look in this directory you will find files like: Logic.tcl (common
events for logic cores), RepeaterLogic.tcl (repeater logic events),
SimplexLogic.tcl (simplex logic events), Module.tcl (common module events),
Help.tcl (help module events), Parrot.tcl (parrot module events), EchoLink.tcl
(echolink module events). There is a comment above each function that says what
it does so have a look in these files and let your imagination flow.


<A name="server-config"><h2>SvxLink server configuration</h2></A>
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

  <DT>LOGICS</DT>
  <DD>
    Specify a comma separated list of logic cores that should be created.
    The logic core is the thing that ties the transciever and the voice
    services (modules) together. It contains the rules for how the radio
    interface should be handled. The specified name of a logic core must
    have a corresponding section specified in the config file. This is
    where the behaviour of the logic core is specified.
  </DD>
</DL>

The next section is the <b>SimlexLogic</b> section. This section contains
configuration data for a simplex logic core. The SvxLink server can handle
more than one logic core. The name of the section, which in this case is
SimplexLogic, must have a corresponding list item in the GLOBAL/LOGICS config
variable for this logic core to be activated. The name "SimplexLogic" is not
magic. It could be called what ever you want.

<DL>
  <DT>TYPE</DT>
  <DD>
    The type of logic core this is. In this case we are setting up a simplex
    logic core so it should be set to "Simplex".
  </DD>
  
  <DT>RX</DT>
  <DD>
    Specify the configuration section name of the receiver to use. All
    configuration for the receiver is done in the specified configuration
    section.
  </DD>
  
  <DT>TX</DT>
  <DD>
    Specify the configuration section name of the transmitter to use. All
    configuration for the trasceiver is done in the specified configuration
    section.
  </DD>
  
  <DT>MODULES</DT>
  <DD>
    Specify a comma separated list of configuration sections for the modules
    to load.
  </DD>
  
  <DT>CALLSIGN</DT>
  <DD>
    Specify the callsign that should be announced on the radio interface.
  </DD>
  
  <DT>IDENT_INTERVAL</DT>
  <DD>
    The number of seconds between identification.
  </DD>
  
  <DT>IDENT_ONLY_AFTER_TX</DT>
  <DD>
    This feature controls when identification is done.  By default,
    identification is done every time the IDENT_INTERVAL expires.  If you
    enable this feature, identification will be done only if there has been
    a recent transmission.  This feature is good for nodes using an RF link
    to provide echolink to a repeater.  Often, in this situation, it is not
    desirable for the link to identify unless legally necessary.  The number
    provided should be greater than 0 and represents the number of seconds
    after an identification has been sent before a transmission is considered
    "new" and requires a new identification to be sent.  If the value is too
    low, the identification itself could trigger the need to identify.  If
    it's too long, a transmission could be made that won't be identified
    because it happened within this window.  For most configurations, 4
    seconds is a good number.
    Note that IDENT_INTERVAL still have to be set for this feature to work.
    That config variable will then be interpreted as the minimum number of
    seconds between identifications.
  </DD>
    
  <DT>EXEC_CMD_ON_SQL_CLOSE</DT>
  <DD>
    Specify a time, in milliseconds, after squelch close after which entered
    DTMF digits will be executed as a command without the need to send the
    # character. This really only is of use when using a radio that it is
    difficult to send DTMF digits from, like the Yaesu VX-2 handheld.
    The down side of enabling this option is that the DTMF detection some
    times false trigger on voice. This can cause interresting situations
    when all of a sudden a module gets activated.
  </DD>
  
  <DT>EVENT_HANDLER</DT>
  <DD>
    Point out the TCL event handler script to use. Read more about the event
    handling subsystem above.
  </DD>
  
  <DT>RGR_SOUND_DELAY</DT>
  <DD>
    The number of milliseconds to wait after the squelch has been closed before
    a roger beep is played. The beep can be disabled by specifying a value
    of -1.
  </DD>  
  
  <DT>REPORT_CTCSS</DT>
  <DD>
    If set, will report the specified CTCSS frequency upon manual
    identification (* pressed). It is possible to specify fractions using
    "." as decimal comma. Disable this feature by commenting out (#) this
    configuration variable.
  </DD>  
  
  <DT>MACROS</DT>
  <DD>
    Point out a section that contains the macros that should be used by this
    logic core. See the section description for macros below for more info.
  </DD>

  <DT>LINKS</DT>
  <DD>
    Specify the name of a configuration section that contains logic linking
    infomation. There is an example section in the default configuration
    file called [LinkToR4]. Right now only one link can be specified.
    A LINKS variable is only needed in the logic that the link should be
    activated from.
  </DD>
</DL>

The next section is the <b>RepeaterLogic</b> section. This section contains
configuration data for a repeater logic core. The SvxLink server can handle
more than one logic core. The name of the section, which in this case is
RepeaterLogic, must have a corresponding list item in the GLOBAL/LOGICS
config variable for this logic core to be activated. The name "RepeaterLogic"
is not magic. It could be called what ever you want.

<DL>
  <DT>TYPE</DT>
  <DD>
    The type of logic core this is. In this case we are setting up a repeater
    logic core so it should be set to "Repeater".
  </DD>
  
  <DT>RX</DT>
  <DD>
    Specify the configuration section name of the receiver to use. All
    configuration for the receiver is done in the specified configuration
    section.
  </DD>
  
  <DT>TX</DT>
  <DD>
    Specify the configuration section name of the transmitter to use. All
    configuration for the trasceiver is done in the specified configuration
    section.
  </DD>
  
  <DT>MODULES</DT>
  <DD>
    Specify a comma separated list of configuration sections for the modules
    to load.
  </DD>
  
  <DT>CALLSIGN</DT>
  <DD>
    Specify the callsign that should be announced on the radio interface.
  </DD>
  
  <DT>EXEC_CMD_ON_SQL_CLOSE</DT>
  <DD>
    Specify a time, in milliseconds, after squelch close after which entered
    DTMF digits will be executed as a command without the need to send the
    # character. This really only is of use when using a radio that it is
    difficult to send DTMF digits from, like the Yaesu VX-2 handheld.
    The down side of enabling this option is that the DTMF detection some
    times false trigger on voice. This can cause interresting situations
    when all of a sudden a module gets activated.
  </DD>

  <DT>EVENT_HANDLER</DT>
  <DD>
    Point out the TCL event handler script to use. Read more about the event
    handling subsystem above.
  </DD>
  
  <DT>NO_REPEAT</DT>
  <DD>
    Set this to 1 if you do NOT want SvxLink to play back the incoming audio.
    This can be used when the received audio is directly coupled by hardware
    wiring to the transmitter. What you win by doing this is that there is
    zero delay on the repeated audio. When the audio is routed through SvxLink
    there is always an amount of delay. What you loose by doing this is the
    audio processing done by SvxLink. Right now not much is done to the audio
    though. There is a high pass filter that removes frequencies below 300Hz.
    It is used to remove CTCSS tones. In the future there might be more audio
    processing like DTMF muting and compression.
  </DD>
  
  <DT>IDLE_TIMEOUT</DT>
  <DD>
    The number of seconds the repeater has been idle before turning the
    transmitter off.
  </DD>
  
  <DT>OPEN_ON_1750</DT>
  <DD>
    Use this configuration variable if it should be possible to open the
    repeater with a 1750Hz tone burst. Specify the number of milliseconds
    the tone must be asserted before the repeater is opened. A value of
    0 will disable 1750 Hz repeater opening.
  </DD>
  
  <DT>OPEN_ON_CTCSS</DT>
  <DD>
    Use this configuration variable if it should be possible to open the
    repeater with a CTCSS tone (PL). The syntax of the value is
    tone_fq:min_length. The tone frequency is specified in whole Hz and the
    minimum tone length is specified in milliseconds. For examples if a 136.5
    Hz tone must be asserted for two seconds for the repeater to open, the
    value 136:2000 should be specified.
  </DD>
  
  <DT>OPEN_ON_DTMF</DT>
  <DD>
    Use this configuration variable if it should be possible to open the
    repeater with a DTMF digit. Only one digit can be specified. DTMF digits
    pressed when the repeater is down will be ignored.
  </DD>
  
  <DT>OPEN_ON_SQL</DT>
  <DD>
    Use this configuration variable if it should be possible to open the
    repeater just by keeping the squelch open for a while. The value to set
    is the minimum number of millisecons the squelch must be open for the
    repeater to open.
  </DD>
  
  <DT>IDENT_INTERVAL</DT>
  <DD>
    The number of seconds between identification. For now, the repeater will
    only identify itself periodically when it is down.
  </DD>

  <DT>IDLE_SOUND_INTERVAL</DT>
  <DD>
    When the repeater is idle, a sound is played. Specify the interval in
    milliseconds between playing the idle sound. An interval of 0 disables
    the idle sound.
  </DD>  
  
  <DT>RGR_SOUND_DELAY</DT>
  <DD>
    The number of milliseconds to wait after the squelch has been closed before
    a roger beep is played. The beep can be disabled by specifying a value
    of -1.
  </DD>
  
  <DT>REPORT_CTCSS</DT>
  <DD>
    If set, will report the specified CTCSS frequency upon manual
    identification (* pressed). It is possible to specify fractions using
    "." as decimal comma. Disable this feature by commenting out (#) this
    configuration variable.
  </DD>  
  
  <DT>MACROS</DT>
  <DD>
    Point out a section that contains the macros that should be used by this
    logic core. See the section description for macros below for more info.
  </DD>  

  <DT>LINKS</DT>
  <DD>
    Specify the name of a configuration section that contains logic linking
    infomation. There is an example section in the default configuration
    file called [LinkToR4]. Right now only one link can be specified.
    A LINKS variable is only needed in the logic that the link should be
    activated from.
  </DD>
</DL>

A macros section is used to declare macros that can be used by a logic core.
The logic core points out the macros section to use by using the MACROS
configuration variable. A macro is a kind of shortcut that can be used to
decrease the amount of key presses that have to be done to connect to common
EchoLink stations for example. On the radio side, macros are activated by
pressing "D" "macro number" "#". A macros section can look something like the
example below. Note that the module name is case sensitive.

<PRE>
[Macros]
1=EchoLink:9999
2=EchoLink:1234567
9=Parrot:0123456789
</PRE>

For example, pressing DTMF sequence "D1#" will activate the EchoLink module
and connect to the EchoTest conference node.
<P>

A <b>logic linking configuration section</b> is used to specify information for
a link between two SvxLink logics. Such a link can for example be used to
connect a local repeater to a remote repeater using a separate link transceiver.
The link is activated/deactivated using DTMF commands. To be able to define two
SvxLink logics, the computer must be equipped with two sound cards.
When the link is active, all audio received by one logic will be transmitted by
the other logic.<BR/>
<EM>Note:</EM> At the moment only locally received audio will be transmitted to
the other logic. EchoLink audio will for example not go through. This will be
fixed in a future release.

<DL>
  <DT>NAME</DT>
  <DD>
    The name of the link. The default action on activation/deactivation of the
    link is to spell the value of this variable. In other words, a callsign is
    a good value.
  </DD>

  <DT>LOGIC1</DT>
  <DD>
    The name of the first logic core that should be linked.
  </DD>

  <DT>LOGIC2</DT>
  <DD>
    The name of the second logic core that should be linked.
  </DD>

  <DT>COMMAND</DT>
  <DD>
    The command prefix to use to activate/deactivate this link. The full command
    consists of one more digit that is either 0 or 1 where 0 means "deactivate"
    and 1 means "activate". If you for example set COMMAND=94, the received DTMF
    command "941#" will activate the link and "940#" will deactivate the link.
  </DD>
</DL>
<P>

A receiver section is used to specify the configuration for a receiver.
Right now there are two types of receivers: Local and Voter. Type Local is
the normal thing to use for a receiver connected to the sound card. An example
of a configuration section for a Local receiver is shown below. In the default
configuration file there is a Local configuration section called <b>Rx1</b>.

<DL>
  <DT>TYPE</DT>
  <DD>
    Always "Local" for a local receiver.
  </DD>
  
  <DT>AUDIO_DEV</DT>
  <DD>
    Specify the audio device to use. Normally <em>/dev/dsp</em>.
  </DD>

  <DT>SQL_DET</DT>
  <DD>
    Specify the type of squelch detector to use. Possible values are: VOX,
    CTCSS or SERIAL. The <b>VOX</b> squelch detector determines if there is a
    signal present by calculating a mean value of the sound samples. The VOX
    squelch detector behaviour is adjusted with VOX_FILTER_DEPTH and
    VOX_LIMIT.<BR/>
    The <b>CTCSS</b> squelch detector checks for the precense of a tone with
    the specified frequency. The tone frequency is specified with CTCSS_FQ.<br/>
    The <b>SERIAL</b> squelch detector use a pin in a serial port to detect
    if the squelch is open. This squelch detector can be used if the receiver
    have an external hardware indicator of when the squelch is open. Specify
    which serial port/pin to use with SERIAL_PORT and SERIAL_PIN.
  </DD>
  
  <DT>SQL_HANGTIME</DT>
  <DD>
    How long, in milliseconds, the squelch will stay open after the detector
    has indicated that it is closed.
  </DD>
  
  <DT>VOX_FILTER_DEPTH</DT>
  <DD>
    The number of milliseconds to create the mean value over. A small value
    will make the vox react quicker (<500) and larger values will make it
    a little bit more sluggish. A small value is often better.
  </DD>

  <DT>VOX_LIMIT</DT>
  <DD>
    The threshold that the mean value of the samples must exceed for the
    squlech to be considered open. It's hard to say what is a good value.
    Something around 1000 is probably a good value. Set it as low as
    possible without getting the vox to false trigger.
  </DD>

  <DT>CTCSS_FQ</DT>
  <DD>
    If CTCSS (PL,subtone) squelch is used (SQL_DET is set to CTCSS), this
    config variable sets the frequency of the tone to use.
    The tone frequency ranges from 67.0 to 254.1 Hz.
    The detector is not very exact so it will detect tones that is near the
    specified tone. Only whole Hz can be specifid so the value should be in the
    range 67 to 254 Hz.
  </DD>

  <DT>SERIAL_PORT</DT>
  <DD>
    If SQL_DET is set to SERIAL, this config variable determines which serial
    port should be used for hardware squelch input (COS - Carrier Operated
    Squelch).<BR/>
    Note: If the same serial port is used for the PTT, make sure you specify
    exactly the same device name. Otherwise the RX and TX will not be able
    to share the port.<BR/>
    Example: SQL_PORT=/dev/ttyS0
  </DD>

  <DT>SERIAL_PIN</DT>
  <DD>
    If SQL_DET is set to SERIAL, this config variable determines which pin in
    the serial port that should be used for hardware squelch input
    (COS - Carrier Operated Squelch). It is possible to use
    the DCD, CTS, DSR or RI pin. The squelch-open-level must also be specified.
    This is done using the syntax SQL_PIN=PIN:LEVEL, where PIN is one of the
    pins above and LEVEL is either SET or CLEAR.<BR/>
    Example: SQL_PIN=CTS:SET
  </DD>
</DL>


Receiver type "Voter" is a "receiver" that combines multiple receivers and
selects one of them to take audio from when the squelch opens. A real voter
selects the receiver that have the best signal strength. For now, this voter
selects the receiver where the squelch opens first. The plan is to implement
a signal strength detector in the future.

<DL>
  <DT>TYPE</DT>
  <DD>
    Always "Voter" for for a voter.
  </DD>
  
  <DT>RECEIVERS</DT>
  <DD>
    Specify a comma separated list of receivers that the voter should use.
    Example: RECEIVERS=Rx1,Rx2,Rx3
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
    off. Note that this is a low level security mechanism that is meant to
    only kick in if there is a software bug in SvxLink. Just so that the
    transmitter will not transmit indefinately. It is not meant to be used to
    keep people from talking too long.
  </DD>
  
  <DT>TX_DELAY</DT>
  <DD>
    The number of milliseconds (0-1000) to wait after the transmitter has been
    turned on until audio is starting to be transmitted. This can be used to
    compensate for slow TX reaction or remote stations with slow reacting
    squelches.
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


Specific configuration variables for the <b>Parrot</b> module.

<DL>
  <DT>FIFO_LEN</DT>
  <DD>
    The length, in seconds, of the FIFO (First In First Out) audio buffer where
    audio is recorded to.
  </DD>
  
  <DT>REPEAT_DELAY</DT>
  <DD>
    Specify a time, in milliseconds, that the parrot module will wait after
    squelch close before playing back the recorded message. This is mostly a
    way to prevent the parrot module to start sending if the squlech momentarily
    closes on weak signals.
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
    The IP address or name of the EchoLink directory server to use.
  </DD>

  <DT>CALLSIGN</DT>
  <DD>
    The callsign to use to login to the EchoLink directory server.
  </DD>

  <DT>SYSOPNAME</DT>
  <DD>
    The name of the person or club that is responsible for this system.
  </DD>

  <DT>PASSWORD</DT>
  <DD>
    The EchoLink directory server password to use.
  </DD>

  <DT>LOCATION</DT>
  <DD>
    The location of the station.<BR/>
    Note: In the default configuration file the value of this configuration
    variable starts with "[Svx]". This is ofcource not necessary but it's fun
    to see which other stations are running SvxLink.
  </DD>

  <DT>MAX_QSOS</DT>
  <DD>
    The maximum number of stations that can participate in a conference QSO
    on this node. If more stations try to connect, the connect request will
    be rejected.
  </DD>

  <DT>MAX_CONNECTIONS</DT>
  <DD>
    When more stations than specified in MAX_QSOS try to connect, a connection
    will temporarily be established long enough to play a message telling the
    remote station that the connection was rejected. The connection is then
    immediately terminated.
    If the number of connections exceeds MAX_CONNECTIONS, the connect request
    is just ignored. This variable is typically set to MAX_QSOS+1 or more if
    using a large number for MAX_QSOS.
  </DD>

  <DT>LINK_IDLE_TIMEOUT</DT>
  <DD>
    The number of seconds that a connection is idle before disconnection
    will occur. This is to prevent a link to stay open if someone forgets
    to disconnect. Disable this feature by setting this config variable
    to zero (or comment it out).
  </DD>

  <DT>DESCRIPTION</DT>
  <DD>
    A longer description that is sent to remote stations upon connection. This
    description should typically include detailed station information like
    QTH, transceiver frequency/power, antenna, CTCSS tone frequency etc.
  </DD>
</DL>


<?php include("footer.inc"); ?>


