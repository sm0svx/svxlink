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
  <LI>A sound clip based system for announcements</LI>
  <LI>Periodic identification</LI>
  <LI>A script based (TCL) event handling system</LI>
  <LI>Roger beep</LI>
  <LI>Trigger manual identification by pressing DTMF *</LI>
  <LI>A simple shortcut, or macro, subsystem</LI>
  <LI>Multiple repeater activation modes: 1750 Hz tone burst, CTCSS, DTMF or
  squelch open.</LI>
  <LI>Periodic "idle sound" when the repeater is up but no signal is being
  received</LI>
  <LI>Possible to define multiple logic cores to connect to multiple
  transceivers</LI>
  <LI>Possible to link logic cores together</LI>
  <LI>Support for multiple receivers using a software voter</LI>
  <LI>Remote receivers can be linked to the logic core via TCP/IP(Internet)</LI>
  <LI>Multiple squelch detectors: Vox, CTCSS, and external (through serial port
  pin)</LI>
  <LI>Experimental deemphasis/preemphasis filer support</LI>
  <LI>DTMF muting</LI>
  <LI>Squelch tail elimination</LI>
  <LI>CTCSS transmit, always or just when there is a signal received</LI>
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
</UL>

<?php include("footer.inc"); ?>
