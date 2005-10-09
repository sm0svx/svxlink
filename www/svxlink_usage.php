<?php
  $selected="svxlink_usage";
  include("header.inc");
?>

<H2>SvxLink server user documentation</H2>
This part of the documentation describes how to use the svxlink server via the
radio interface. SvxLink is controlled by DTMF (Dual Tone Multi Frequency)
signalling. All commands to the system ends with the number sign (#). It's like
the <em>enter</em> key on a computer. However, the sysop have an option to make
commands execute on squelch close so that the number sign is not needed.
<P>
There also is an "anti-flutter" mode of operation. This mode of operation make
DTMF detection perform well when the signal is weak or there is mobile flutter
present. This mode is activated on a per command basis. To activate, start each
command with an "A" (think: Activate Anti-flutter). Enter the digits, replacing
duplicate digits with a "B" (11=1B, 111=1B1, 1111=1B1B etc). End with a "C"
(think: Conclude).
Some examples below.
<PRE>
AC          <-- Empty command. Same as just pressing #.
A1C         <-- Command executed: 1
A12B3C      <-- Command executed: 1223
A12B23C     <-- Command executed: 12223
</PRE>
So what good is this ?  Well, this way of coding commands will allow digits to
be "double detected" without affecting the end result, which can happen if there
is mobile flutter present on the received signal. Consider the examples below.
<PRE>
ACCC        <-- Empty command. Same as just pressing #.
A111C       <-- Command executed: 1
A11122BB3CC <-- Command executed: 1223
AA12B2233C  <-- Command executed: 12223
</PRE>
Exactly the same result as the first example, even though some digits were
detected multiple times.
<P>
The star key (*) is special. It triggers the node to identify itself.
<P>
The "D" key is used to activate a pre programmed macro or short cut. A macro is
used to reduce the number of DTMF codes that have to be sent. For example if the
macro 5 have been defined to connect to the *ECHOTEST* node only D5# have to be
pressed instead of 2#9999#. It is the node sysop that defines the macros. Check
with your local node sysop which macros are setup. If you connect to an
EchoLink node very often it can be nice to setup a macro for it. Ask your sysop
to do that.
<P>
The system is built up of a core system that handles the transciever interface.
The core system can then be extended by loading modules that handles a specific
voice service. Each module have a unique ID number associated with it. The
association is done during the configuration of the system. To activate a
module, press its ID number followed by the number sign. The default
configuration specifies the <em>Help</em> module as ID 0, so start by
activating that module and listen to the help messages (i.e. send 0# or A0C).
Go on to test the other modules. When a module is activated, send the 0#
command to get help about that module. To exit a module, just send a #
(or AC). Description in more detail for the different modules follow below.

<H3>The Help Module</H3>
The Help module is used to get help about the system as a whole and also help
about each specific module. Send the ID number of the module to get help about.
As always, end each command with the number sign.


<H3>The Parrot Module</H3>
The Parrot module plays back everything you say. This can be used as a simplex
repeater or just to hear how you sound to the other stations. It also tells you
the DTMF digits you press. As always, end all DTMF strings with the number sign.
Exit the module by sending just the number sign.


<H3>The EchoLink Module</H3>
The EchoLink module is used to connect to other EchoLink stations. To connect
to another station, just enter the node number ended with the number sign. To
disconnect, press the number sign. To exit the module, press the number sign
when not connected. Pressing 1# will tell you the callsign of all connected
stations.
<P>
To get more information on the EchoLink system, have a look at the
<A href="http://www.echolink.org">EchoLink homepage</A>.


<H3>The TclVoiceMail Module</H3>
This module implements a simple voice mail system to be used by the local
node users. It is not possible (yet) to send voice mails to users at other
nodes. To be able to use the voice mail module you must have a personal
login ID and password. Contact your sysop to get these.
<P>
When the module is activated (default 3#) you will be prompted to enter your
login ID and password. The login ID always is three digits long. The password
can be between one to seven digits long. Enter your login ID and password in
one sequence like 123456# if your login ID is 123 and your password is 456#.
After that, context menus will guide you through using the system. An empty
command (#) will abort the current operation.
<P>
To start recording a voice mail command 2# is used. The system will then ask
you for a recepient. You can combine these two steps by entering the user ID
directly after the command. For example if you want to record a voice mail
for user 123, send the sequence 2123#.
<P>
The recommended way of encoding user IDs is the "phone method". Many phones
have letters on the key pad like: 2=ABC, 3=DEF, 4=GHI, 5=JKL, 6=MNO,
7=PQRS, 8=TUV, 9=WXYZ. To encode the callsign SM0SVX, use the three last
letters and map them to digits. Then we will get the user ID 789. In case of
a collition, add 1. For example AFC and ADB will both map to 232. One
possible mapping is then AFC=232 and ADB=233.
<P>
To deactivate the module, just press # when the main menu is active.

<?php include("footer.inc"); ?>

