<?php include("header.inc"); ?>

<H2>SvxLink server user documentation</H2>
This part of the documentation describes how to use the svxlink server via the
radio interface. SvxLink is controlled by DTMF (Dual Tone Multi Frequency)
signalling. All commands to the system ends with the number sign (#). It's like
the <em>enter</em> key on a computer. The star key (*) is special. It triggers
the node to identify itself.
<P/>
The system is built up of a core system that handles the transciever interface.
The core system can then be extended by loading modules that handles a specific
voice service. Each module have a unique ID number associated with it. The
association is done during the configuration of the system. To ativate a module,
press its ID number followed by the number sign. The default
configuration specifies the <em>Help</em> module as ID 0, so start by
activating that module and listen to the help messages (i.e. send 0#). Go on to
test the other modules. When a module is activated, send the 0# command to get
help about that module. Description in more detail for the different modules
follow below.

<H3>The Help Module</H3>
The Help module is used to get help about the system as a whole and also help
about each specific module. Send the ID number of the module to get help about.
As always, end each command with the number sign.


<H3>The Parrot Module</H3>
The Parrot module plays back everything you say. This can be used as a simplex
repeater or just to hear how you sound to the other stations. It also tells you
the DTMF digits you press. As always, end all DTMF strings with the number sign.
Exit the module by sending just the module sign.


<H3>The EchoLink Module</H3>
The EchoLink module is used to connect to other EchoLink stations. To connect to
another station, just enter the node number ended with the number sign. To
disconnect, press the number sign. To exit the module, press the number sign
when not connected.


<?php include("footer.inc"); ?>
