<?php
  $selected="index";
  include("header.inc");
?>

<H2>Main Screen</H2>
Welcome to the home of SvxLink. 
The SvxLink project aim to develop a flexible general purpose voice services
system for ham radio use. The svxlink server consists of a core that handles the
connection to the tranciever. The transciever audio is connected to the PC
through the sound card and the PTT is controlled by a pin in the serial port.
The voice services are loaded into the core as plugins called modules in SvxLink
lingo. Examples of existing voice services are: Help - a help system, Parrot - a
module that plays back everything you say and EchoLink - connect to other
EchoLink stations. The project also includes an EchoLink client GUI application
(Qtel).
<P/>
The SvxLink server can also act as a repeater controller. However, this mode of
operation has only been briefly tested as of this writing. Hopefully I will get
my hands on some repeater hardware this fall (2004) to test it.
<P/>
EchoLink is an amateur radio invention (well actually it is just a
modified verison of Internet telephony) to link radio transcievers together
over the Internet. You must have an amateur radio license to use
it. The original EchoLink software can be found at
<A href="http://www.echolink.org/">http://www.echolink.org/</A>. However,
this software only supports the Windows operating system and it is
closed source. SvxLink is released under the GPL license. 
<P/>
Qtel is only a EchoLink client program. It does not have the
sysop mode. That is, it can not be connected to a transciver
and act as a link. For the latter, use the svxlink server.
<P/>
SvxLink is known to work under these operating systems (I really hope that
this list will grow in time):
<UL>
  <LI>Red Hat Linux 9</LI>
  <LI>Fedora Core 1</LI>
  <LI>Fedora Core 2</LI>
</UL>
<P/>

<?php include("footer.inc"); ?>

