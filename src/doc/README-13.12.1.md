SvxLink release 13.12.1 -- 30 Jun 2014
--------------------------------------

This is a bugfix release which fix a number of bugs that were present in the
13.12 release. The most fatal bugs that were fixed caused the voter to crash
the SvxLink Server under certain conditions. Also, an assertion problem
associated with Alsa that seem to mostly appear on the Raspberry Pi was fixed.

=============================================================================

Complete ChangeLog for SvxLink Server version 1.3.1
---------------------------------------------------

* Bugfix: The voter may have had a race condition which caused an assertion
  failure. The code is a little bit more forgiving now.

* Bugfix: The voter could cause an assertion failure if a signal level lower
  than -100 were reported from one of the receivers.

* Bugfix in ModuleTcl example file.



Complete ChangeLog for Qtel version 1.2.1
-----------------------------------------

* Updated Ukrainian translation.



Complete ChangeLog for the EchoLib library version 1.2.1
--------------------------------------------------------

* Nothing new for EchoLib in this release



Complete ChangeLog for the Async library version 1.2.1
------------------------------------------------------

* Some sound hardware cause an Alsa failure which SvxLink previously handled
  as a fatal error (assert) and aborted the application. A warning is now
  printed instead and the sound device is closed and reopened.

* Support added for Opus version < 1.0.

* Bugfix for Async::Config::getValue. The std::ws operator set the fail bit on
  FreeBSD at end of string. This caused some config options to not be read
  correctly.
