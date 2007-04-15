<?php
  $selected="index";
  include("header.inc");
?>

<H2>Installation instructions for Debian 3.1</H2>
<B>Note:</B> This page need to be updated to work with the latest release of
SvxLink.
<P> 
This is an installation script contributed by SM6VQJ / Birger.

<PRE>
#!/bin/bash
# Debian 3.1 - Download and binary install script for svxlink
# 2005-10-28
apt-get update
apt-get install wget alien tcl8.4 tcl8.4-dev libgsm1 -y
wget http://ovh.dl.sourceforge.net/sourceforge/svxlink/echolib-0.10.2-1fc4.i386.rpm
wget http://ovh.dl.sourceforge.net/sourceforge/svxlink/libasync-0.12.1-1fc4.i386.rpm
wget http://dl.atrpms.net/all/libsigc++104-1.0.4-0_1.rhfc4.at.i386.rpm
alien -i -c *.rpm
rm -f *.rpm
</PRE>

<?php include("footer.inc"); ?>
