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
There is also an "anti-flutter" mode of operation. This mode of operation make
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
The system is built up of a core system that handles the transciever interface.
The core system can then be extended by loading modules that handles a specific
voice service. Each module have a unique ID number associated with it. The
association is done during the configuration of the system. To ativate a module,
press its ID number followed by the number sign. The default
configuration specifies the <em>Help</em> module as ID 0, so start by
activating that module and listen to the help messages (i.e. send 0# or A0C). Go
on to test the other modules. When a module is activated, send the 0# command to
get help about that module. To exit a module, just send a # (or AC). Description
in more detail for the different modules follow below.

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
The EchoLink module is used to connect to other EchoLink stations. To connect to
another station, just enter the node number ended with the number sign. To
disconnect, press the number sign. To exit the module, press the number sign
when not connected.
<P>
To get more information on the EchoLink system, have a look at the
<A href="http://www.echolink.org">EchoLink homepage</A>.


<?php include("footer.inc"); ?>
