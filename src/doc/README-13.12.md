SvxLink release 13.12 -- 01 Dec 2013
------------------------------------

The biggest news in this release are: improved logic linking, improved QSO
recorder, support for squelch on GPIO, improved pre- and deemphasis filters.
More details about each change can be found in the ChengeLog below.

Also note that the filename for sound clips have changed a small bit. A sound
clip archive should now be named:

svxlink-sounds-<lang_code>-<voice name>-<samp rate>-<svxlink ver>.tar.bz2

Example:

svxlink-sounds-en_US-heather-16k-13.12.tar.bz2

A new audio codec, Opus (http://www.opus-codec.org/), is now supported for
RemoteTrx links. The Opus codec is a new IETF standard for encoding audio with
bitrates ranging from 6 kb/s to 510 kb/s. The audio quality is very good over
the whole bitrate range. It is even better then AAC, at least below 128kb/s.
To enable SvxLink to use the Opus codec, make sure to install the development
libraries for Opus and recompile.
=============================================================================

Complete ChangeLog for SvxLink Server version 1.3.0
---------------------------------------------------

* Improved support for linking logics together. It is now possible to have
  multiple links active at the same time for a logic. A link can be set to
  be default active. This mean that the link will be activated as soon as the
  application starts and also that the link can be automatically reactivated,
  after a timeout, if manually deactivated.

* Improved the logic shutdown feature. Upon shutdown, any active module is
  deactivated immediately and module activation is not allowed until
  the logic is activated again. This for example means that if the logic is
  shut down and an EchoLink connection comes in, that connection will be
  rejected. Also, no other DTMF commands than the logic activation command
  is allowed when the logic is shut down.
  The command was also renamed to ONLINE_CMD since everything became more
  logical. Calling it SHUTDOWN_CMD was a bit confusing. Should subcommand 1
  mean that the logic is shut down or brought back online?  Naming the command
  ONLINE_CMD make everything clear: 1=online, 0=offline.

* Improved QSO recorder. There now is only one configuration variable,
  QSO_RECORDER, in a logic configuration section. This configuration variable
  is used to set the DTMF command and to point out a configuration section
  where the rest of the QSO recorder configuration is specified.
  The QSO recorder can now close a recording after a configurable recording
  time and start a new one. The maximum time is specified using the MAX_TIME
  config variable. The SOFT_TIME config variable is used to specify an
  interval where the file will be closed on the first detected squelch close.
  This will give more natural cuts between recordings.
  It is now also possible to specify a maximum total size for all files in the
  recording directory. This is done using the MAX_DIRSIZE config variable.
  When the limit have been reached, the oldest file(s) will be deleted.
  The DEFAULT_ACTIVE config variable is used to set the default activation
  state of the QSO recorder. If set to 1, the recorder will be activated upon
  SvxLink startup. If the TIMEOUT variable is set, the activation state will
  return to the one set in DEFAULT_ACTIVE if manually activated/deactivated.
  If less than MIN_TIME milliseconds of audio has been recorded when a file is
  closed, the file will be deleted.
  If the node is idle for more than QSO_TIMEOUT seconds, the file will be
  closed.
  The ENCODER_CMD variable makes it possible to run an external audio
  encoder to compress the recordings.

* Added support for Squelch on GPIO. Use SQL_DET=GPIO and GPIO_SQL_PIN=gpio3
  in your config for GPIO support.
  Patch contributed by DG9OAA / Jonny.

* GPIO pins are now specified with the full name (e.g. gpio3) instead of with
  just the pin number. This enables the use of any pin naming scheme.

* Improved pre- and de-emphasis filters. They were not good at all in the
  previous relese. Now the filters should behave as expected. Also, the
  filters have been designed to be each others complement so that they will
  exactly cancel each other out.

* Now possible to specify DTMF_DEC_TYPE=NONE (or just comment it out) if no
  DTMF decoder is required for a certain receiver. Also made it possible to
  specify NONE for SEL5_DEC_TYPE to make them behave in the same way.

* Bugfix for the REPORT_CTCSS config variable. Depending on the locale
  settings (e.g. the LC_NUMERIC environment variable) the wrong CTCSS
  frequency could be announced.

* Bugfix: Setting DTMF_MUTING=0 did not remove the audio delay, which it
  should do.

* Sound clips are now included for module MetarInfo.



Complete ChangeLog for Qtel version 1.2.0
-----------------------------------------

* Nothing new for Qtel in this release.



Complete ChangeLog for the EchoLib library version 1.2.0
--------------------------------------------------------

* Nothing new for EchoLib in this release



Complete ChangeLog for the Async library version 1.2.0
------------------------------------------------------

* The Async::AudioRecorder class now have some added features. In addition to
  the hard filesize limit there now is a soft limit at which the file will be
  closed when the audio source call flushSamples. A signal will be emitted
  when either of the limits are hit. Also, begin and end timestamps now are
  available.

* The Async::AudioFilter setOutputGain method now take a dB value as argument.

* Added support for the Opus audio codec.

* New class Async::Exec for executing external programs.

