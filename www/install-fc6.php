<?php
  $selected="index";
  include("header.inc");
?>

<H2>Installation instructions for Fedora Core 6</H2>
Start by downloading the folowing SvxLink RPM:s from the SourceForge
<A gref="http://sourceforge.net/project/showfiles.php?group_id=84813">download site</A>:
<B>echolib-X.Y.Z-1fc6.i386.rpm</B>, <B>libasync-X.Y.Z-1fc6.i386.rpm</B>,
<B>svxlink-server-X.Y.Z-1fc6.i386.rpm</B>, <B>qtel-X.Y.Z-1fc6.i386.rpm</B>.
Be sure to download the Fedora Core 6 (fc6) files. Sorry, no direct links since the
SoureForge download site behaved a bit strange when I tried to link to the files.
<P>
Install the files by using the "rpm -Uvh" command.

<PRE>
rpm -Uvh echolib-X.Y.Z-1fc6.i386.rpm
rpm -Uvh libasync-X.Y.Z-1fc6.i386.rpm
rpm -Uvh svxlink-server-X.Y.Z-1fc6.i386.rpm
rpm -Uvh qtel-X.Y.Z-1fc6.i386.rpm
</PRE>

<P>
To make the SvxLink server start on boot, use:
<PRE>
chkconfig svxlink on
</PRE>

Now go back to the installation page and read the
<A href="install.php#post-install">post install stuff</A> chapter.

<?php include("footer.inc"); ?>
