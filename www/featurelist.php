<?php
  $selected="index";
  include("header.inc");
?>

<H2>Feature List</H2>
This page list the features of the SvxLink server and Qtel. It is a quick way to
understand what the applications can be used for.

<H3>The SvxLink Server</H3>

These are the most important features of the SvxLink server application.

<UL>
  <LI>Can act as a repeater controller or just operate on a simplex channel</LI>
  <LI>Plugin based system to load voice services into the SvxLink logic
  core</LI>
  <LI>DTMF controlled</LI>
  <LI>DTMF anti-flutter mode to suppress multiple digit detection when mobile flutter is present or during weak-signal conditions</LI>
  <LI>A sound clip based system for announcements</LI>
  <LI>Periodic identification</LI>
  <LI>A script based (TCL) event handling system</LI>
  <LI>Possible to use left/right stereo channels as two mono channels</LI>
  <LI>Roger beep</LI>
  <LI>Trigger manual identification by pressing DTMF *</LI>
  <LI>A simple shortcut, or macro, subsystem</LI>
  <LI>Repeater logic specific features:</LI>
  <UL>
    <LI>Multiple repeater activation modes: 1750 Hz tone burst, CTCSS, DTMF or
    squelch open.</LI>
    <LI>Periodic "idle sound" when the repeater is up but no signal is being
    received</LI>
    <LI>Announcements are mixed in with normal audio</LI>
    <LI>The volume of announcements is lowered if there are other traffic</LI>
  </UL>
  <LI>Possible to define multiple logic cores to connect to multiple
  transceivers</LI>
  <LI>Possible to link logic cores together</LI>
  <LI>Support for multiple receivers using a software voter</LI>
  <LI>Remote receivers can be linked to the logic core via TCP/IP(Internet)</LI>
  <LI>Remote transmitters can be linked to the logic core via TCP/IP
  (Internet)</LI>
  <LI>Multiple squelch detectors: Vox, CTCSS, signal level and external
  (through serial port pin)</LI>
  <LI>Deemphasis/preemphasis filer support</LI>
  <LI>DTMF muting</LI>
  <LI>Squelch tail elimination</LI>
  <LI>CTCSS transmit, always or just when the squelch is open</LI>
  <LI>A simple help system module</LI>
  <LI>A parrot module that retransmit received audio</LI>
  <LI>An EchoLink module used to connect to other EchoLink nodes</LI>
  <UL>
    <LI>Idle timeout</LI>
    <LI>A callsign lookup/connect by callsign function</LI>
    <LI>List connected stations on demand</LI>
    <LI>Own node id report function</LI>
    <LI>Random connect to link, repeater or conference</LI>
    <LI>Reconnect to last disconnected node</LI>
    <LI>Listen only mode</LI>
  </UL>
  <LI>A voice mail module</LI>
  <UL>
    <LI>Record voice mail for local users</LI>
  </UL>
  <LI>A DTMF repeater module that retransmits received DTMF digits.</LI>
  <UL>
    <LI>Control other DTMF controlled stations on the same frequency</LI>
  </UL>
  <LI>Possible to write new modules in the TCL scripting language</LI>
</UL>


<H3>Qtel - The Qt EchoLink Client</H3>

These are the most important features of the Qtel application.

<UL>
  <LI>Graphical EchoLink client application</LI>
  <LI>Use the Qt widget toolkit</LI>
  <LI>The EchoLink station list is divided into Conferences, Links, Repeaters
  and Stations</LI>
  <LI>A bookmark system</LI>
  <LI>Support for multiple languages (English and Swedish at the moment)</LI>
  <LI>VOX</LI>
</UL>

<?php include("footer.inc"); ?>
