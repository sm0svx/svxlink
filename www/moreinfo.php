<?php
  $selected="moreinfo";
  include("header.inc");
?>

<H2>Developer Documentation</H2>
This page contains some more technical information about the SvxLink project
and its associated libraries.
<P/>
The EchoLink protocols are implemented in the EchoLib library. It is
a C++ library that was written at the same time as the Qtel GUI. The goal is
to make EchoLib platform independent and easy to use. The API is fairly well
documented. EchoLib can be used to write both GUI application (in Qt) and
server applications with no Qt dependency.
<P/>
The EchoLink library use the Async library as its application framework.
This is a library that also was written during the development of Qtel.
For more information about this library, check out the documentation below.
<P/>

<B>Library documentation:</B><BR/>
<A href="doc/async/html/index.html">Documentation for the Async library</A><BR />
<A href="doc/echolib/html/index.html">Documentation for the EchoLib library</A><BR />

<?php include("footer.inc"); ?>
