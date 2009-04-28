<?php
  $selected="index";
  include("header.inc");

  $rel="090426";
?>

<H2>Installation instructions for openSUSE 11.1</H2>
This instruction was written for the <?php echo $rel ?> release of SvxLink.
<p/>
There are no official binary packages for openSUSE but it is quite easy
to compile SvxLink from source. First, install a couple of
packages that SvxLink depend on. In a terminal, type the
following commands as user 'root'.

<pre>
zypper install make gcc gcc-c++ libsigc++12-devel libgsm-devel popt-devel libgcrypt-devel tcl-devel speex-devel alsa-utils
</pre>
If you need Qtel you also need the QT development libraries.
<pre>
zypper install qt3-devel qt3-devel-tools
</pre>
Download and compile the source code and then install it:
<pre>
wget  http://downloads.sourceforge.net/svxlink/svxlink-<?php echo $rel ?>.tar.gz
tar xvzf svxlink-<?php echo $rel ?>.tar.gz
cd svxlink-<?php echo $rel ?> 
make
make install
</pre>
You also need to download and install the sound files (sounds-<?php echo $rel ?>)
as described in
<a href="http://svxlink.sourceforge.net/install.php#source">the main installation instruction</a>
if you want to run SvxLink Server.
<p/>
Now go back to the installation page and read the
<A href="install.php#post-install">post install stuff</A> chapter.

<?php include("footer.inc"); ?>
