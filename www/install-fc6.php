<?php
  $selected="index";
  include("header.inc");
?>

<H2>Installation instructions for Fedora Core 6</H2>
Start by downloading the latest versions of the following SvxLink RPM:s from the
SourceForge
<A href="http://sourceforge.net/project/showfiles.php?group_id=84813">download site</A>:
<B>echolib</B>, <B>libasync</B>, <B>svxlink-server</B> and/or <B>qtel</B>.
Be sure to download the Fedora Core 6 (fc6) files. Sorry, no direct links since the
SoureForge download site behaved a bit strange when I tried to link to the files.
<P>
Install the Livna yum repository configuration file if you've not already
done so. The Livna repository contains the GSM audio compression RPM which
is required by SvxLink.
<PRE>
rpm -ivh http://rpm.livna.org/livna-release-6.rpm
</PRE>
<P>
Import the public key that the SvxLink RPM:s are signed with.
<PRE>
rpm --import http://svxlink.sourceforge.net/RPM-GPG-KEY-sm0svx
</PRE>
<P>
Install the files by using the "yum localinstall" command or if you already
have SvxLink installed, use "yum localupdate" instead. The advantage of using
yum to install the RPM:s is that all packages that SvxLink depends on will be
automatically downloaded and installed.
<PRE>
yum localinstall libasync-0.14.0-1.fc6.i386.rpm echolib-0.12.0-1.fc6.i386.rpm qtel-0.10.0-1.fc6.i386.rpm svxlink-server-0.8.0-1.fc6.i386.rpm
</PRE>
<P>
To make the SvxLink server start on boot, use:
<PRE>
chkconfig svxlink on
</PRE>

Now go back to the installation page and read the
<A href="install.php#post-install">post install stuff</A> chapter.

<?php include("footer.inc"); ?>
