<?php
  $headercolor = "#CCCCFF";
  $selected = "news";
  include("header.inc");
?>

<H2>Project News</H2>
<TABLE width="95%" align="center">
  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>2 Dec 2005: <I>Version 051202 released</I></B>
  <TD/></TR>
  <TR><TD>
    Ok, time for another release. There was just too much small things
    in the ChangeLogs to wait with a release any longer. Hopefully, this
    release is bug free ;-)
    <UL>
      <LI>
        Bugfix: The SvxLink server would crash when recording voice mails
        on other distributions than Fedora.
      </LI>
      <LI>
        A new "connect by call" feature in the EchoLink module.
      </LI>
      <LI>
        The voice mail module now can send a notification e-mail when
        a new voice mail arrives for a user. First implementation by
        PA3FNT, improved by me (SM0SVX).
      </LI>
      <LI>
        Timestamps in the log file.
      </LI>
      <LI>
        Now possible to use both DTR and RTS together for PTT, which is
        required by some interface boards.
      </LI>
    </UL>
    Be sure to check out the
    <A href="http://sf.net/project/shownotes.php?release_id=375500">ChangeLog</A>
    for more detailed information.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>9 Oct 2005: <I>Version 051009 released</I></B>
  <TD/></TR>
  <TR><TD>
    The biggest reason for this release is that a couple of serious bugs
    have been fixed. The most important changes are:
    <UL>
      <LI>
        Bugfix in the networking code which caused the svxlink server to
        crash or hang.
      </LI>
      <LI>
        New module Tcl which make it possible to write simpler modules in
        the TCL scripting language.
      </LI>
      <LI>
        New module TclVoiceMail, which is a simple voice mail system.
      </LI>
    </UL>
    Be sure to check out the
    <A href="http://sf.net/project/shownotes.php?release_id=362318">ChangeLog</A>
    for more detailed information.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>14 Aug 2005: <I>Version 050814 released</I></B>
  <TD/></TR>
  <TR><TD>
    Hmmm... Let's face it. I will not be able to release more than two times per
    year but I guess you have all noticed that already. Sooo... here is a brand
    new release for you to play with.<BR/>
    For all of you running Qtel nothing have happened besides some bugfixes in
    libraries that are shared between Qtel and the SvxLink server.
    The SvxLink server is still the focus for my interest. The most important
    changes are:
    <UL>
      <LI>
      	DNS lookups could sometimes block SvxLink which could cause dropouts
	in the sound. DNS lookups are now done as a background job.
      </LI>
      <LI>
      	SvxLink could some times be silently logged out from the EchoLink
	directory server on communications error. Now an error message will
	be emitted and the directory server connection will be restored.
      </LI>
      <LI>
      	There is a new, more flexible, event handling subsystem for audio
	announcements. All events triggers the execution of a TCL function.
	The TCL function is then responsible for emitting the proper sounds.
	Have a look at the <A href="install.php#event-subsystem">installation
	documentation</A> for more information.
      </LI>
      <LI>
      	The repeater controller logic is now fully working and have been tested
	since march with good results. It is running a real repeater at SK3GW-R.
      </LI>
      <LI>
      	It is now possible to link two SvxLink logics together if you have two
	sound cards in the computer. An example usage of this is to connect the
	local repeater to a remote repeater via a linking transceiver.
	Connection/disconnection is done with DTMF commands.
      </LI>
      <LI>
      	A couple of configuration variables have changed since the last release.
	It's all in the
	<A href="http://sf.net/project/shownotes.php?release_id=349143">ChangeLog</A>.
      </LI>
      <LI>As usual a bunch of smaller bugs have been fixed.</LI>
    </UL>
    Be sure to check out the
    <A href="http://sf.net/project/shownotes.php?release_id=349143">ChangeLog</A>
    for more detailed information.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>25 Mar 2005: <I>Version 050325 released</I></B>
  <TD/></TR>
  <TR><TD>
    Time for another release. It's been six months since the last release.
    It didn't happen very much during the end of the last year so there really
    was no reason to make a new release. The last months I have been working
    a little bit on the SvxLink server. What's new then:
    <UL>
      <LI>
      	The repeater controller logic has now been tested for more than one
	month without any big problems. However, there are some audio issus
	still to be worked out.
      </LI>
      <LI>COS - Carrier Operated Squelch</LI>
      <LI>A macro (or short cut) system</LI>
      <LI>Qtel now have a connect sound for incoming connections</LI>
      <LI>Bugfixes</LI>
    </UL>
    Check out the
    <A href="http://sf.net/project/shownotes.php?release_id=315589">ChangeLog</A>
    for more detailed information.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>26 Sep 2004: <I>Version 040926 released</I></B>
  <TD/></TR>
  <TR><TD>
    Hmmm... I aimed at making one release per month. I guess I failed...
    There was just too much fun stuff to poke around with so the thing would
    never achive release quality. And there were some bugs that just refused to
    go away. But now it's here; the best release ever of SvxLink :-)
    <P>
    As with the last release, most effort have gone into the development of the
    SvxLink server. The biggest news in this release are:
    <UL>
      <LI>The EchoLink module now supports multiple connections</LI>
      <LI>Anti-flutter mode for better DTMF detection</LI>
      <LI>A bunch of bug fixes and new small features</LI>
      <LI>Improved documentation</LI>
    </UL>
    Qtel didn't change much. The IP-address in the station list have been
    replaced with the node ID. This is of more use I think.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>27 May 2004: <I>Version 040527 released</I></B>
  <TD/></TR>
  <TR><TD>
    So, finally I have assembled a new release of SvxLink. One of the news is
    improved support for Alsa OSS emulation. Binary packages are now also
    available for Fedora Core 2.
    <P>
    Most work has gone into the development of the SvxLink server. It now can
    act as a repeater, DTMF detection has been improved, CTCSS squelch has
    been implemented, improved sound clips.
    <P>
    Unfortunately I have no real repeater hardware to try my software on so the
    repeater logic has not been tested in a real situation. It would be really
    interesting if someone would want to try it on real hardware.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>17 Apr 2004: <I>Good and bad news</I></B>
  <TD/></TR>
  <TR><TD>
    The good news are that I'm back from my skiing holiday and no bones are
    broken.
    <P>
    The bad news is that I found a serious bug in the EchoLink communications
    svxlink server module. Connecting EchoLink stations only get a timeout.
    This bug does not affect Qtel. It's fixed in CVS but I have not compiled
    a new release yet.
    <P>
    I also discovered that the DTMF detector is a little sensitive to speech.
    It detects random digits when someone is speaking on the channel.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>07 Apr 2004: <I>Web pages touch up and more documentation</I></B>
  <TD/></TR>
  <TR><TD>
    Updated the web pages a bit and moved the menus from the top to the left.
    Also added some user documentation for Qtel and the SvxLink server.
    <P/>
    I will be gone skiing for a week now so if I don't answer your mails that's
    why. I hope to find some mails from people that have tried the SvxLink
    server or Qtel when I come home :-)
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>05 Apr 2004: <I>More RPMS and documentation</I></B>
  <TD/></TR>
  <TR><TD>
    RPMS are now available for Red Hat Linux 9. I have also whipped together
    some installation documents. No usage document yet though. Try pressing 0#
    when you have the svxlink server running to get some help.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>04 Apr 2004: <I>We have transciever connection!</I></B>
  <TD/></TR>
  <TR><TD>
    So, here goes. The first try at EchoLink "sysop mode" grew into a
    pretty general voice services system for ham radio use. The svxlink
    server is a standalone server that is connected to a transciever.
    Voice services modules can then be loaded into the system. Existing
    modules right now are: a help system, a parrot module that plays back
    everything you say to it and an EchoLink connection module.
    <P>
    There is a lack of documentation at the moment but I'll try to get some
    up as soon as I can. Only binary packages for Fedora Core 1 exist at the
    moment. Source code is of course available as well. Go to the download
    section to get your hands on the new goodies.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>16 Mar 2004: <I>Still alive...</I></B>
  <TD/></TR>
  <TR><TD>
    I just want to say that the project is still alive. It is just a little
    slow. I am now working on a stand alone link daemon. It is operational
    but there are still some work to be done to make it really usable. I will
    check in some code some time the following weeks.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>18 Nov 2003: <I>Cleaned up the web pages a bit</I></B>
  <TD/></TR>
  <TR><TD>
    The web pages should be operational now. It should be possible to
    check out from CVS and compile.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>03 Jul 2003: <I>Project has been registered at SourceForge</I></B>
  <TD/></TR>
  <TR><TD>
    The project has now been registered at
    <A href="https://sourceforge.net/projects/svxlink/">SourceForge</A>.
    I will start moving everything over there as soon as I have figured
    out how everything works.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>30 Jun 2003: <I>Site is searchable via Google</I></B>
  <TD/></TR>
  <TR><TD>
    I saw that this site has now appeared in the Google search engine.
    I guess people will start finding this page then. I have not made
    any announcements yet because of a problem I encountered. The problem
    is with audio boards/OSS drivers that does not support playing sounds in
    mono. I have made a fix so that it should work in stereo mode as well.
    However, when I tested this the mic gain was _very_ high. I could not
    make it work properly. I have probably made something really stupid but
    I can't figure out what. Please tell me if you know.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>04 May 2003: <I>Website is up</I></B>
  <TD/></TR>
  <TR><TD>
    Put up the website to prepare for the first release of Qtel.
  </TD></TR>
  
  <TR><TD><BR/></TD></TR>

  <TR><TD bgcolor="<?php echo $headercolor; ?>">
    <B>08 Mar 2003: <I>First rows hacked</I></B>
  <TD/></TR>
  <TR><TD>
    The day when it all began. The first rows of code were written.
  </TD></TR>
</TABLE>

<?php include("footer.inc"); ?>

