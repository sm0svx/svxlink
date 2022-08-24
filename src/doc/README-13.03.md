SvxLink release 13.03 -- 08 mar 2013
------------------------------------

This is the first release of SvxLink that is marked as non-beta. All sofware
version numbers have been set to 1.0.0. This release also mark 10 years of
SvxLink development.

The CTCSS detector have been improved so that it is much quicker and also
more reliable. The receiver voter implementation have also been greatly
improved. An experimental implementation for APRS statistics has been added.
Another useful new feature is an UDP "audio device" which for example can
be used to interface with GNU Radio.



=============================================================================

Complete ChangeLog for SvxLink Server version 1.0.0
---------------------------------------------------

* Moved from sigc++ version 1.2 to version 2.
  Patch contributed by Felix/WU8K.

* The CTCSS detector has been improved. It is now much faster and have
  a lower false trigger rate in its default configuration. The mean
  detection time is now about 200ms where it previously was about 450ms.
  Previous configurations will have to be changed to adapt to the new
  detector. Have a look in the manual page at the new configuration
  variables: CTCSS\_MODE, CTCSS\_OPEN_THRESH, CTCSS\_CLOSE\_THRESH,
  CTCSS\_SNR\_OFFSET, CTCSS\_BPF\_LOW and CTCSS\_BPF\_HIGH. The CTCSS\_THRESH
  configuration variable is obsolete.

* Now possible to set RTS and/or DTR to a static state by configuration
  if needed. Have a look at the SERIAL\_SET\_PINS config variable for the
  LocalRx and LocalTx sections in the svxlink.conf.5 manual page.

* Improved the SigLevTone detector using the same techniques that was
  used to improve the CTCSS decoder.

* Now possible to block incoming Echolink connects from stations if they
  connect too often. New config variable: CHECK\_NR\_CONNECTS.
  Patch from Adi/DL1HRC, reworked by SM0SVX/Tobias.

* Bugfix in Metar information module: SvxLink could crash if two
  requests were made in rapid succession.

* Now possible to have personal greetings on incoming EchoLink connections,
  if desired. Some TCL coding is neccessary to enable it though.

* Improved voter implementation that can vote continously and not only at the
  start of a transmission. By default it will check the signal strength for
  all receivers once per second and switch to the strongest one if the
  difference is large enough. New config variables: REVOTE\_INTERVAL,
  HYSTERESIS, SQL\_CLOSE\_REVOTE\_DELAY, RX\_SWITCH\_DELAY.

* ModuleEchoLink: The text "[listen only] is now prepended to the sysop name
  if listen only mode is active.

* New feature "extended squelch hangtimne" added. It is now possible to
  configure an extended squelch hangtime that is activated when the
  signal strength fall below a given threshold.

* Now SvxLink can send APRS statistics about RX squelch open count,
  TX on count, RX squelch open time and TX on time.
  Patch from Adi / DL1HRC.



Complete ChangeLog for Qtel version 1.0.0
------------------------------------------

* Moved from sigc++ version 1.2 to version 2.
  Patch contributed by Felix/WU8K.



Complete ChangeLog for the EchoLib library version 1.0.0
--------------------------------------------------------

* Moved from sigc++ version 1.2 to version 2.
  Patch contributed by Felix/WU8K.

* Fixed a small bug which could cause a crash in a very strange and
  probably unusual case.

* Bugfix: Remote station name was clipped if it contained a space character.



Complete ChangeLog for the Async library version 1.0.0
------------------------------------------------------

* Moved from sigc++ version 1.2 to version 2.
  Patch contributed by Felix/WU8K.

* Now possible to bind a UDP socket to a given interface.

* New "audio device" which read and write audio from a UDP socket.
  This can for example be used to stream audio to/from GNU Radio.

* Now possible to set config variables in memory using the Config::setValue
  function. It is not possible to write the change back to the config
  file though.

* New class FileReader to read a file in non blocking mode. Contributed
  by Steve / DH1DM.
