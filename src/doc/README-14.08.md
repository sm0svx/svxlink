SvxLink release 14.08 -- 02 Aug 2014
------------------------------------
This release does not introduce any really big news but there have been a
rather long list of smaller changes done since the last major release about
eight months ago.

PTT_TYPE now have to be given in a Tx configuration section: SerialPin, GPIO
Dummy or NONE is supported. The last two do the same thing. The format of the
SERIAL_PIN configuration variable have changed. Instead of the old format,
PINNAME:LEVEL, a exclamation mark is used to indicate inverted operation.
Inverted operation have been added to GPIO PTT/squelch as well.  The same
convension is used here with an exclamation mark indicating inverted operation.

An important thing to know about is a change done for a local TX if it is
configured to transmit CTCSS. Previously, when setting CTCSS_LEVEL in a LocalTx
section, the voice level was adjusted down by the same amount that the CTCSS
level was adjusted up.  This is just confusing so I removed that compensation.
You may need to readjust the transmit level due to this change.

Other new configuration variables/features: CLOSE_ON_SEL5, disconnect a
specific EchoLink station, SIGLEV_BOGUS_THRESH, millisecond log timestamps,
MUTE_TX_ON_RX, REJECT_CONF (reject EchoLink conference connections),
BIND_ADDR (use a specific network interface for EchoLink). 

New EchoLink event handler functions: chat_received, squelch_open. The EchoLink
receive timeout handning have been improved to work better with Apple devices.

Log printouts are now being flushed to file before exit. Previously, if an
error occurred during startup, no printouts would show up in the log causing
confusion.

One big difference for those building SvxLink from source code is that CMake is
the official build system now. How to build SvxLink using CMake is described in
the INSTALL file that can be found in the root of the source tree.
The old buildsystem is still there but it will probably be removed in the next
release if there are no problems with the CMake buildsystem.

There is one known larger problem left in this release which was also in the
previous release. The EchoLink module may all of a sudden stop talking to the
EchoLink directory server causing it go offline until SvxLink is restarted.

The full list of changes and more details can be found below. Information on
configuration of new features can be found in the manual pages.

=============================================================================

Complete ChangeLog for SvxLink Server version 1.4.0
---------------------------------------------------

* The PTT hardware type must now be specified using the PTT_TYPE configuration
  variable in a TX section. Specify "SerialPin" for using a pin in the serial
  port, "GPIO" to use a pin in a GPIO port. Set PTT_TYPE to "Dummy" or "NONE"
  to not use any PTT hardware at all. It is an error to not specify PTT_TYPE.

* New config variable for repeater logics: CLOSE_ON_SEL5 which make it
  possible to close the repeater using a selective calling sequence.
  Patch contributed by Adi / DL1HRC.

* Now possible to have active low functionality on the PTT pin for GPIO. This
  is configured in the same way as for serial ports, by prefixing the gpio
  pin specification with an exclamation mark (!) in the PTT_PIN config.
  Example: PTT_PIN=!gpio5

* ModuleEchoLink: It now is possible to disconnect one specific station by
  choosing from a list of callsigns. The disconnect by callsign feature
  is activated using subcommand 7 while the EchoLink module is active.
  This feature was contributed by Martin/DF1AMB.

* ModuleEchoLink: New TCL event handling function: "chat_received" which
  is called when a remote chat message is received.

* ModuleEchoLink: New TCL event handling function: "squelch_open" which is
  called when the local receiver squelch open and closes. This function can be
  used to for example send a roger beep to the remote station when the squelch
  closes.
  Patch contributed by DL1HRC / Adi.

* Previously the upper threshold for the signal level reported from the noise
  signal level detector was hardcoded to 120. It can now be set using the
  configuration variable SIGLEV_BOGUS_THRESH. By default it is disabled.

* Now possible to make log entries have milliseconds in the timestamp. Use the
  code "%f" in the TIMESTAMP_FORMAT configuration variable to make that happen.

* New configuration variable for SimplexLogic, MUTE_TX_ON_RX, which can be set
  to allow transmission during reception. This may be desired if the logic is
  connected to some full duplex device.

* Now possible to prefix the GPIO_SQL_PIN config variable value with an
  exclamation mark (!) to get inverted operation for the GPIO squelch.

* Changed syntax for SERIAL_PIN in an RX section to match other config
  variables better. Now just the pin name need to be specified. If inverted
  operation is desired, the pin name may be prefixed with an exclamation mark
  (!). The old syntax, SERIAL_PIN=PINNAME:LEVEL, is still supported but a
  warning is printed if that form is used.

* ModuleEchoLink: New config variable REJECT_CONF which when set will reject
  connections from stations that are in conference mode.

* ModuleEchoLink: New config variable BIND_ADDR which can be set to bind the
  EchoLink UDP sockets to a specific IP-address/interface. This may be
  needed if the computer is fitted with more than one ethernet interface and
  EchoLink should only be used on one of them.

* Previously, when setting CTCSS_LEVEL in a LocalTx section, the voice level
  was adjusted down by the same amount that the CTCSS level was adjusted up.
  This is just confusing so I removed that compensation.

* Bugfix in the Voter: If the squelch were open while quitting, a crash could
  occur.

* Bugfix in logfile handling: SvxLink/RemoteTrx would crash if the filesystem
  was full while trying to write to the logfile.

* Flushing logs at exit.
  There have always been a problem with the log handling in SvxLink that if an
  error occurred during startup, the error printouts would not end up in the
  log. In fact, nothing would end up in the log, leaving the user confused.
  Now the log is flushed before exit so that all printouts end up in the log.



Complete ChangeLog for Qtel version 1.2.1
-----------------------------------------

* Nothing new for Qtel in this release



Complete ChangeLog for the EchoLib library version 1.3.0
--------------------------------------------------------

* Now possible to bind to a specific IP address to use a specific network
  interface.

* Improved EchoLink RX timeout handling.
  The previous EchoLink RX timeout handling was a little bit too simplistic.
  It could not handle clients sending larger audio packets, like Apple devices
  seem to do. The new algorithm adapts the timeout value to how much audio
  data have been received, making the timeout longer if a larger audio packet
  is received.



Complete ChangeLog for the Async library version 1.3.0
------------------------------------------------------

* Fixed Async::AudioDeviceUDP so that audio output is paced instead of writing
  as fast as possible.

* Added a NULL audio encoder and decoder that can be used when one does not
  want audio to be sent at all.

* Added the ability to bind TCP client and server sockets to a specific
  IP-address.

* Serial port settings are now not restored unless they have been explicitly
  changed using the Serial::setParams function.

* The serial port TX/RX buffers are now only flushed if explicitly specified
  in the open call.

* The IpAddress class now have an input stream operator.

* Bugfix in Async::{AudioSelector,DnsLookup,AudioSplitter}: Important code had
  been placed within assert statements. By default CMake set compiler options
  that remove all assert statements when compiling in release mode. This
  caused some things to stop working when compiling for release.

* Now possible to change the buffer size of a TCP connection using
  TcpConnection::setRecvBufLen.
