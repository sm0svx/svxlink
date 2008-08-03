<?php
  $selected="install";
  include("header.inc");
?>

<A name="prebuilt"><h2>Installation from prebuilt binaries</h2></A>
SvxLink has been developed under Fedora Linux. It should be easy to install
under Fedora since there are binary packages available. Also, since the
development is done under Fedora it's most tested under that distribution.
<P>
SvxLink have a few dependencies. Most of them should be installed by default on
a normal Linux workstation. The Qtel application requires X/Qt but the svxlink
server is a pure console application. Other dependencies: libsigc++ (only
version 1.2 will work), libgsm, libtcl, libpopt, libspandsp.
<P>
The SvxLink distribution contains a couple of RPM:s:

<DL>
  <DT>libasync</DT>
  <DD>Required. Base library functions.</DD>
  <DT>echolib</DT>
  <DD>Required for EchoLink support</DD>
  <DT>svxlink-server</DT>
  <DD>The SvxLink server application</DD>
  <DT>qtel</DT>
  <DD>The Qt EchoLink Client application</DD>
  <DT>xxx-devel packages</DT>
  <DD>Development packages. Only needed if you are going to develop applications
  based on the libraries in the SvxLink project.
</DL>

<P>
Specific installation instructions for different distributions:

<UL>
  <LI><A href="install-f9.php">Fedora 9</A></LI>
  <LI><A href="install-f8.php">Fedora 8</A></LI>
  <LI><A href="install-fc6.php">Fedora Core 6</A></LI>
  <LI><A href="install-deb31.php">Debian 3.1</A></LI>
</UL>
<P>
Now continue below reading the
<A href="#post-install">post install stuff</A> chapter.


<A name="source"><h2>Installation from source</h2></A>
If you are not running one of the distributions that there are prebuilt
binaries for, you will have to build the whole thing from source. You will still
need to satisfy the dependencies specified above. That is
<A href="http://ftp.gnome.org/pub/GNOME/sources/libsigc++/1.2/">libsigc++ 1.2</A>,
<A href="http://kbs.cs.tu-berlin.de/~jutta/toast.html">gsm</A>,
<A href="http://www.tcl.tk/software/tcltk/download.html">tcl</A> and
<A href="http://www.soft-switch.org/downloads/spandsp/">spandsp</A>.
Maybe you can find prebuilt binaries for these libs for your distribution.
Otherwise you will just have to compile them as well.
<P>
To compile Qtel, the <A href="http://www.trolltech.com/">Qt widget toolkit</A>
and the X window system are needed. There is a good chance that these will
already be installed on your system. If Qt is not installed, find a prebuilt
package or compile it from source. If the X window system is not installed,
you're on your own...
<P>
Now download the sources for SvxLink from the
<A href="http://sourceforge.net/project/showfiles.php?group_id=84813&package_id=114319">
sourceforge download area</A>. Find the
<em>svxlink-YYMMDD.tar.gz</em>
with the latest date. If you are going to run the svxlink-server, you will also need the
<A href="https://sourceforge.net/project/showfiles.php?group_id=84813&package_id=114321">sounds-YYMMDD.tar.gz</A>
with the matching date. Find a good spot to unpack and compile the source and cd into that directory.
Then do the following (install must be done as user root):
<PRE>
tar xvzf svxlink-YYMMDD.tar.gz
cd svxlink-YYMMDD
make
make install
</PRE>
If you are going to run the SvxLink server, unpack the sound files in a good
location. A good location could for example be /usr/share/svxlink/. As user
root, do the following:
<PRE>
cd /usr/share/svxlink
tar xvzf /path-to-wherever-you-put-the-tar-file/sounds-YYMMDD.tar.gz
</PRE>
<P>
Specific installation instructions for different distributions:
<UL>
  <LI><A href="install-ubuntu804.php">Ubuntu 8.04</A></LI>
  <LI><A href="install-deb40.php">Debian 4.0</A></LI>
</UL>
<P>
Now continue below reading the
<A href="#post-install">post install stuff</A> chapter.
<P>


<A name="post-install"><h2>Post install stuff</h2></A>
<strong>Note:</strong> For Alsa based systems (like Fedora Core >= 2), the Alsa
OSS emulation is used for sound I/O. There is a bug in the emulation layer
which will make SvxLink/Qtel fail. To work around this bug, there is an
environment variable called ASYNC_AUDIO_NOTRIGGER. This is by default set
to 1 to activate the workaround since most modern Linux distributions now
use Alsa by default. If you get into trouble with the audio output, try
setting it to 0. For the SvxLink server that is done in /etc/sysconfig/svxlink
if you are using the provided start script.
<P>
For Qtel set it manually on the command line, in a start script or in the
login script for your shell. The instructions below assume you are using
bash (usually the default).
<pre>
export ASYNC_AUDIO_NOTRIGGER=0
qtel &
</pre>
Or on one line:
<pre>
ASYNC_AUDIO_NOTRIGGER=0 qtel &
</pre>
<P>
The environment variable setting will be lost on logout so the <em>export</em>
line is best put into the file ".bash_profile", which can be found in your home
directory.
<P>
Note that setting this environment variable when it is not needed can make
SvxLink/Qtel to stop working. Only set it if you have audio problems.
<P>
<strong>Note:</strong> Make sure that no other audio applications are running
at the same time as SvxLink/Qtel. If another application has opened the sound
device, SvxLink/Qtel will hang until the device is closed by the other
application. Especially, if you are having problems with SvxLink/Qtel hanging,
check for sound servers like <em>artsd</em> and the like. However, this is
less of a problem now when most distributions are using the Alsa sound layer.
<P>
If you only are going to run Qtel, first read the
<A href="#audio-level-adj">Audio level adjustment</A> chapter
and then go directly to the <A href="qtel_usage.php">Qtel User Docs</A>.
<P>
If you are going to run the svxlink server, read on.
<P>


<A name="hardware"><h2>Hardware</h2></A>
To run the SvxLink server, some kind of hardware is needed to connect the
computer to the transciever. At the moment I am using an
<A href="http://lea.hamradio.si/~s57nan/ham_radio/svx_intf/">interface designed by Aleks, s54s,</A>
which is fully isolated with transformers and opto-couplers. For a long time I
just used a simple direct, non-isolated, connection between the transceiver
and the computer. This also work well but I have actually fried one sound
card this way.
<P>
WB0RXX/Tim has constructed an interface circuit that he use with his
SvxLink system. The schematic can be found
<A href="http://montevideocomputers.com/hacem.org/pdf/Svxlink_interface.pdf">here</A>.
<P>
Typical EchoLink hardware might work with SvxLink as well. Have a look at the
<A href="http://www.echolink.org/interfaces.htm">EchoLink interfaces</A> page.
However, I have not tried any of these so there are no guarantees.<BR/>
Please tell me if you get any of the interfaces working or if they are not
working.
<P>


<A name="audio-level-adj"><h2>Audio level adjustment</h2></A>
There are no audio level controls in SvxLink server nor Qtel. The levels must
be adjusted with an external tool like aumix, kmix, alsamixer or whatever your
favourite mixer is. Start one of the mixers and locate the controls to use for
adjusting the levels. The output level is adjusted using the two sliders Pcm
and Vol. The input level is adjusted using the Capture (sometimes called
IGain) slider, not the Mic or Input (line-in) slider. The latter two are used
to adjust the monitoring level of the two inputs. Set these two to zero.
Select to use either the microphone or the line-in input. Set the Pcm, Vol
and Mic/Input sliders half way up. Adjust the levels according to the
instructions below.
<P>
To adjust the levels in Qtel, start by connecting to the *ECHOTEST* server. This
EchoLink server will echo back everything you send to it. Right after the
connection has been established, a greeting message will be played back. Adjust
the speaker level to a comfortable level using the Pcm and Vol sliders. Press
the PTT and say something and listen how it comes back. Adjust the Capture
slider until you are satisfied.
<P>
To adjust the levels for the SvxLink server, first set the PEAK_METER
configuration variable to 1 in the receiver configuration section.
Start the SvxLink server up and press *# on
the keyboard. This will make the svxlink server identify itself. The # is
only needed for the *-command when entering commands on the keyboard. When
doing it from the radio, a * followed by a squelch close will trigger an
identification. Do this a couple of times and adjust the Pcm and Vol sliders
to the highest volume possible without distorsion.
<P>
To adjust the audio input level, start by opening the squelch on the receiver
so that SvxLink just hear noise. Pull the audio input gain sliders
up until you see messages about distorion printed on the console. Then lower
the audio gain until no distorsion messages are printed. If you cannot make
SvxLink print distorsion messages, the input level is too low. You should try
to fix this on the analogue side but it is possible to use the PREAMP
configuration variable in the receiver section to fix it.
<P>
Now, activate the parrot module by pressing 1# on
the keyboard. Use another transmitter to make a short transmission. Listen to
the recorded audio and make sure it sounds good. Now try to transmit some DTMF
digits and see if the digits are detected. If not, try to adjust the input
level up or down and try again. Try all 16 digits: 0-9, *, #, A, B ,C, D.
<P/>
As a rule of thumb, try to not pull the sliders over 90%. Most sound cards
will distort the signal if the level is set too high. Instead, use the PREAMP
configuration variable if received audio is too low.
<P>


<A name="server-config"><h2>SvxLink server configuration</h2></A>
During the svxlink-server package installation the <em>/etc/svxlink.conf</em>
default configuration file is installed. Module configuration files are placed
under /etc/svxlink.d. Edit the configuration files to your liking.
<P>
There is a voice greeting message sent to connecting echolink stations. This
message should be replaced with a personal one. I have used the <em>rec</em>
application that is included in the <em>sox</em> package to record the sounds.
Lately I have started using <em>audacity</em> to record sound clips.
Any tool capable of recording raw sound files using 8kHz sampling rate and 16
bit signed samples is ok to use. Another alternative is to record the sound
files using your favorite tool and then convert them to the correct format using
the sox application. The rec application should be used as shown below.
<PRE>
  rec -r8000 -sw /usr/share/svxlink/sounds/EchoLink/greeting.raw
  play -r8000 -sw /usr/share/svxlink/sounds/EchoLink/greeting.raw
</PRE>
<P>
Further configuration information can be found in the manual page
<A href="man/man5/svxlink.conf.5.html">svxlink.conf</A>. There are also
manual pages for the <A href="man/man1/svxlink.1.html">svxlink application</A>
and its modules
(<A href="man/man5/ModuleHelp.conf.5.html">ModuleHelp</A>,
<A href="man/man5/ModuleParrot.conf.5.html">ModuleParrot</A>,
<A href="man/man5/ModuleEchoLink.conf.5.html">ModuleEchoLink</A>
<A href="man/man5/ModuleDtmfRepeater.conf.5.html">ModuleDtmfRepeater</A>).
<P>
To set up a remote receiver, have a look at the
<A href="man/man1/remotetrx.1.html">remotetrx</A> and the
<A href="man/man5/remotetrx.conf.5.html">remotetrx.conf</A> manual pages.
<P>
After the configuration has been done, start the server by typing
<em>svxlink</em> at the command prompt. It is possible to simulate DTMF input by
pressing the 0-9, A-D, *, # keys. Have a look at the
<A href="svxlink_usage.php">user documentation</A> to begin testing the server.
To get help about command line options, start the svxlink server with the
<em>--help</em> switch.
<P>
When everything is configured and working, start the SvxLink server using the
/etc/init.d/svxlink start script. A logfile will be put in /var/log/svxlink.
<P>
<strong>Note:</strong> To start the svxlink server in the background, use the
<em>--daemon</em> switch. Do not use "&amp;". This will make the server hang
when trying to read from standard input.
<P>


<A name="event-subsystem"><h2>The Event Handling Subsystem</h2></A>
This chapter can be skipped on the first reading. Come back here to read this
chapter when you feel like customizing your SvxLink server or adding your own
features.
<P>
There are a lot of sounds that should be played as a response to an event in
the SvxLink system. To make these sounds as configurable as possible there is
a programmable event handling subsystem. The programming language chosen for
this is <A href="http://www.tcl.tk/">TCL (Tool Command Language)</A>.
For each event in SvxLink there is
a corresponding TCL function that is called when the event occurs. In this
function the normal action is to play a sound or a couple of sound clips. It is
of course also possible to use the full power of TCL to make all sorts of things
happen. For example execution of an external application, reading files with
information (e.g. DX, weather data etc), time based events (e.g. only do this
when the time is...).
<P>
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
<P>
There are a couple of functions that can be used within a TCL function to make
things happen in the SvxLink core.
<DL>
  <DT>playFile filename</DT>
  <DD>
    Play the file pointed out by <i>filename</i>. The filename may be given
    with an absolute path or a path relative to the directory where the
    events.tcl script is at. The format of the file must be 16 bit signed raw
    samples. A wav-file can be converted to the correct format by using the
    "sox" application:
    <PRE>sox filename.wav -r8000 -sw filename.raw</PRE>
  </DD>

  <DT>playTone fq amplitude length</DT>
  <DD>
    Play a tone with the specified frequency (Hz), amplitude (0-1000) and
    length (milliseconds).
  </DD>

  <DT>playSilence length</DT>
  <DD>
    Play the specified number of milliseconds of silence. This can be used to
    trim the spacing between audio clips.
  </DD>

  <DT>recordStart filename</DT>
  <DD>
    Start recording of received audio to a file. Only one recording at a time
    can be active.
  </DD>

  <DT>recordStop</DT>
  <DD>
    Stop a previously started recording.
  </DD>
</DL>

There are also a couple of convenience functions implemented in TCL:

<DL>
  <DT>playMsg context name</DT>
  <DD>
    This function also play a file. It actually use the playFile function to
    play the file. The path to the file to play is formed as
    follows: <em>event.tcl directory/context/name.raw</em>. If no matching
    sound clip is found in the directory specified by "context", the
    Default directory will be searched.
  </DD>
  
  <DT>spellWord word</DT>
  <DD>
    Spell the specified word using a phonetic alphabet.
  </DD>
  
  <DT>playNumber number</DT>
  <DD>
    Spell the specified number digit by digit.
  </DD>
  
  <DT>playTwoDigitNumber number</DT>
  <DD>
    Play a number consisting of two digits from 00-99.
  </DD>
  
  <DT>playTime hour minute</DT>
  <DD>
    Play the specified time. For example "playTime 13 35" will produce
    "One Thirtyfive PM".
  </DD>
</DL>
<P>


<?php include("footer.inc"); ?>


