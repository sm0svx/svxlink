<?php
  $selected="index";
  include("header.inc");
?>

<H2>Installation instructions for Fedora >= 9</H2>
The SvxLink applications are available in the official Fedora
repository from release 9. To install SvxLink Server type:
<PRE>
yum install svxlink-server
</PRE>
To install Qtel type:
<PRE>
yum install qtel
</PRE>
<P>
To make the SvxLink server start on boot type:
<PRE>
chkconfig svxlink on
</PRE>

Now go back to the installation page and read the
<A href="install.php#post-install">post install stuff</A> chapter.

<?php include("footer.inc"); ?>
