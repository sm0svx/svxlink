<?php
  $selected="index";
  include("header.inc");
?>

<H2>Installation instructions for Fedora 8</H2>
This instruction was written for the 080102 release of SvxLink.
<P>
The easiest way to install SvxLink is to use the installation script.
It will only work on Fedora 8. Download the script and run it by typing
<PRE>
sh install_f8.sh
</PRE>
If that fails for some reason, follow the manual description below.
<P>
Start by downloading the latest versions of the following SvxLink RPM:s from the
SourceForge
<A href="http://sourceforge.net/project/showfiles.php?group_id=84813">download site</A>:
<A href="http://downloads.sourceforge.net/svxlink/libasync-0.15.0-1.fc8.i386.rpm">libasync</A>,
<A href="http://downloads.sourceforge.net/svxlink/echolib-0.12.1-1.fc8.i386.rpm">echolib</A>,
<A href="http://downloads.sourceforge.net/svxlink/svxlink-server-0.9.0-1.fc8.i386.rpm">svxlink-server</A> and/or
<A href="http://downloads.sourceforge.net/svxlink/qtel-0.10.1-1.fc8.i386.rpm">qtel</A>.
<P>
Import the public key that the SvxLink RPM:s are signed with, if you've not
already done so.
<PRE>
rpm --import http://svxlink.sourceforge.net/RPM-GPG-KEY-sm0svx
</PRE>
<P>
Install the files by using the "yum localinstall" command or if you already
have SvxLink installed, use "yum localupdate" instead. The advantage of using
yum to install the RPM:s is that all packages that SvxLink depend on will be
automatically downloaded and installed.
<PRE>
yum localinstall libasync-0.15.0-1.fc8.i386.rpm echolib-0.12.1-1.fc8.i386.rpm qtel-0.10.1-1.fc8.i386.rpm svxlink-server-0.9.0-1.fc8.i386.rpm
</PRE>
<P>
To make the SvxLink server start on boot, use:
<PRE>
chkconfig svxlink on
</PRE>

Now go back to the installation page and read the
<A href="install.php#post-install">post install stuff</A> chapter.

<?php include("footer.inc"); ?>
