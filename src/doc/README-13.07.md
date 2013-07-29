SvxLink release 13.07 -- 29 Jul 2013
------------------------------------

The biggest news in this release is support for EchoLink proxy. While
implementing that, the handling of server specifications were also improved
so that multiple servers can be configured. These are tried if the currently
used server is not possible to connect to.

Other additions are support for PTT on GPIO and a transmitter shutdown DTMF
command.



=============================================================================

Complete ChangeLog for SvxLink Server version 1.2.0
---------------------------------------------------

* Added support for EchoLink proxy.

* ModuleEchoLink: Now possible to configure multiple servers for the directory
  server. The servers will be tried in order until a working one is found.
  The SERVER config variable has been replaced by the SERVERS config variable
  which is a space separated list of servers. Host names that resolve
  to multiple IP addresses, like servers.echolink.org, will also be correctly
  handled so that each resolved IP address will be tried in order until a
  working one is found.

* Added support for PTT on GPIO. This is for example good for embedded
  systems like the Raspberry Pi where GPIO pins are readily available. Using
  GPIO will eliminate the need for a USB to serial converter.
  Patch contributed by K1FSY / David.

* ModuleEchoLink: New TCL function "is_receiving" that is called when a remote
  EchoLink transmission is started or stopped. The default implementation will
  send a beep over the local transmitter.
  Patch from DL1HRC / Adi.

* Now possible to force the transmitter off using a DTMF command. The new
  configuration variable SHUTDOWN_CMD is used in a logic configuration
  section to set the command that should be used. The node will continue
  to operate as usual with the only difference that the transmitter will
  not be turned on for any event.



Complete ChangeLog for Qtel version 1.2.0
-----------------------------------------

* Updated language translations: Russian (UR3QJW / Volodymyr),
  Japanese (JH1PGO / Masao).

* Added support for EchoLink proxy.

* Now possible to configure multiple servers for the directory server. The
  servers will be tried in order until a working one is found. Servers are
  specified as a space separated list. Host names that resolve to multiple IP
  addresses, like servers.echolink.org, will also be correctly handled so that
  each resolved IP address will be tried in order until a working one is found.



Complete ChangeLog for the EchoLib library version 1.2.0
--------------------------------------------------------

* Added support for EchoLink proxy.

* Now possible to configure multiple servers for the directory server. The
  servers will be tried in order until a working one is found.
  Host names that resolve to multiple IP addresses, like servers.echolink.org,
  will also be correctly handled so that each resolved IP address will be
  tried in order until a working one is found.



Complete ChangeLog for the Async library version 1.1.1
------------------------------------------------------

* Fixed some include directives for Async::CppApplication.

* The Async::TcpClient class now always do a name lookup before trying to
  connect. Previously, when the old lookup was cached, IP addresses that
  changed over time was not handled.
