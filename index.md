---
title: SvxLink by sm0svx
layout: default
---

# The Project
SvxLink is a project that develops software targeting the ham radio community.
It started out as an EchoLink application for Linux back in 2003 but have now
evolved to be something much more advanced.

<!--
# Blog Posts
<ul class="posts">
{% for post in site.posts %}
  <li><span>{{ post.date | date_to_string }}</span> &raquo; <a href="{{ post.url }}">{{ post.title }}</a></li>
{% endfor %}
</ul>
-->

# SvxLink Server
The SvxLink Server is a general purpose voice services system, which when
connected to a transceiver, can act as both an advanced repeater system and can
also operate on a simplex channel. One could call it a radio operating system
since it sits between the hardware (transceiver) and the applications (modules)
and handle basic system services as well as input and output.

SvxLink is very extensible and modular. Voice services are implemented as
modules which are isolated from each other.  Modules can be implemented in
either C++ or TCL. Examples of modules are:

 * Help               -- A help system
 * Parrot             -- Play back everything that is received
 * EchoLink           -- Connect to other EchoLink stations
 * DtmfRepeater       -- Repeater received DTMF digits
 * TclVoiceMail       -- Send voice mail to other local users
 * PropagationMonitor -- Announce propagation warnings from dxmaps.com
 * SelCall            -- Send selective calling sequences by entering DTMF codes

# Qtel
Qtel, the Qt EchoLink client, is a graphical application used to access the
EchoLink network.

# Resources
These are some of the resources connected to SvxLink:

 * [Wiki Pages](https://github.com/sm0svx/svxlink/wiki)
   -- Main documentation
 * [Issue Tracker](https://github.com/sm0svx/svxlink/issues)
   -- Report bugs and feature requests
 * [Download Releases](https://github.com/sm0svx/svxlink/releases)
   -- Download source code releases here
 * [Download Sound Clips](https://github.com/sm0svx/svxlink-sounds-en_US-heather/releases)
   -- Download English sound clip files for SvxLink Server from here
 * [Mailing Lists](http://sourceforge.net/p/svxlink/mailman)
   -- Communicate with other SvxLink users
 * [GitHub Main Page](https://github.com/sm0svx/svxlink)
   -- The project site on GitHub
 * [The SvxLink SourcForge Site](http://sourceforge.net/projects/svxlink/)
   -- Old project site
