<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">

<HTML>

<HEAD>
  <META http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
  <LINK REL=STYLESHEET TYPE="text/css"
        HREF="stylesheet" TITLE="Basic Styling">
  <TITLE>SvxLink - a Multi Purpose Voice Services System</TITLE>
</HEAD>

<BODY>
<H1 align="center">SvxLink - Multi Purpose Voice Services System</H1>
<HR noshade/>
<TABLE width="100%" border="0" cellspacing="20">
  <COLGROUP>
    <COL width="0*">
    <COL>
  </COLGROUP>
  <TR valign="top"><TD NOWRAP>
  <SPAN class="body-code-snippet-caption">Main Menu</SPAN>
  <DIV class="body-code-snippet">
    <MENUITEM><A href="index.php">Main Screen</A></MENUITEM><BR/>
    <MENUITEM><A href="news.php">Project News</A></MENUITEM><BR/>
    <MENUITEM><A href="screenshots.php">Screenshots</A></MENUITEM><BR/>
    <MENUITEM><A href="install.php">Installation Instructions</A></MENUITEM><BR/>
    <?php if ($selected == "install") { ?>
    <SUBMENUITEM><A href="#prebuilt">Prebuilt binaries</A></SUBMENUITEM><BR/>
    <SUBMENUITEM><A href="#source">Source install</A></SUBMENUITEM></BR>
    <SUBMENUITEM><A href="#post-install">Post install</A></SUBMENUITEM></BR>
    <SUBMENUITEM><A href="#hardware">Hardware</A></SUBMENUITEM></BR>
    <SUBMENUITEM><A href="#audio-level-adj">Audio levels</A></SUBMENUITEM></BR>
    <SUBMENUITEM><A href="#event-subsystem">The Event Handling Subsystem</A></SUBMENUITEM></BR>
    <SUBMENUITEM><A href="#server-config">SvxLink server config</A></SUBMENUITEM></BR>
    <?php } ?>
    <MENUITEM><A href="qtel_usage.php">Qtel User Docs</A></MENUITEM><BR/>
    <MENUITEM><A href="svxlink_usage.php">SvxLink Server User Docs</A></MENUITEM><BR/>
    <MENUITEM><A href="moreinfo.php">Developer Documentation</A></MENUITEM><BR/>
    <MENUITEM><A
    href="https://sourceforge.net/project/showfiles.php?group_id=84813">
      Software Downloads</A></MENUITEM><BR/>
    <MENUITEM><A href="support.php">Support</A></MENUITEM><BR/>
    <?php if ($selected == "support") { ?>
    <SUBMENUITEM><A href="#mailing-lists">Mailing lists</A></SUBMENUITEM><BR/>
    <SUBMENUITEM><A href="#reporting-bugs">Reporting Bugs</A></SUBMENUITEM></BR>
    <?php } ?>
    <MENUITEM><A href="https://sourceforge.net/projects/svxlink/">SourceForge Summary</A></MENUITEM>
  </DIV>
  <P>
  <A href="http://sourceforge.net">
    <img src="http://sourceforge.net/sflogo.php?group_id=84813&amp;type=5"
         width="210" height="62" border="0" alt="SourceForge.net Logo" />
  </A>
  </TD>
  <TD>
  

