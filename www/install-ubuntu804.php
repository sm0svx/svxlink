<?php
  $selected="index";
  include("header.inc");
?>

<H2>Installation instructions for Ubuntu 8.04</H2>
This instruction was written for the 080730 release of SvxLink.
<p/>
There are no binary packages for Ubuntu but it is quite easy
to compile SvxLink from source. First, install a couple of
packages that SvxLink depend on.

<pre>
sudo apt-get update
sudo apt-get install alsa-utils g++ libsigc++-1.2-dev \
		libgsm1-dev libpopt-dev tcl-dev make libspandsp-dev
</pre>
If you need Qtel you also need the QT development libraries.
<pre>
sudo apt-get install libqt3-mt-dev
</pre>
Download and compile the source code and then install it:
<pre>
wget  http://downloads.sourceforge.net/svxlink/svxlink-080730.tar.gz
tar xvzf svxlink-080730.tar.gz
cd svxlink-080730
make
make install
</pre>
You also need to download and install the sound files (sounds-080730)
as described in
<a href="http://svxlink.sourceforge.net/install.php#source">the main installation instruction</a>
if you want to run SvxLink Server.
<p/>
Now go back to the installation page and read the
<A href="install.php#post-install">post install stuff</A> chapter.

<?php include("footer.inc"); ?>
