<?php
  $headercolor = "lightblue";
  include("header.inc");
?>

<TABLE width="95%" align="center">
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
    <B>03 Jul 2003: <I>Project has been registerad at SourceForge</I></B>
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

