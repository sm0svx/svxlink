<?php
  $selected="index";
  include("header.inc");

  $rel="080730";
?>

<H2>Installation instructions for Debian 4.0</H2>
This instruction was written for the <?php echo $rel ?> release of SvxLink.
<p/>
There are no binary packages for Debian but it is quite easy
to compile SvxLink from source. First, install a couple of
packages that SvxLink depend on. In a terminal, type the
following commands as user 'root'.

<pre>
apt-get update
apt-get install alsa-utils g++ make libsigc++-1.2-dev libgsm-dev libpopt-dev tcl8.4-dev libspandsp-dev
</pre>
If you need Qtel you also need the QT development libraries.
<pre>
sudo apt-get install libqt3-mt-dev
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
