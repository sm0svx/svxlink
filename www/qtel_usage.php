<?php include("header.inc"); ?>

<H2>Qtel user documentation</H2>
Qtel stands for the "QT EchoLink" client. It is only a EchoLink client
application. There is no "sysop mode". If it is a link you want to run, have a
look at the SvxLink server application.

Qtel is quite a simple application, so most things should be self explainatory.
There are three windows: the main window, the configuration dialog and the
communications dialog.
<P/>
<A href="images/qtel-mainwindow.png">
  <IMG class="floatleft" border="0" alt="[Qtel main window]"
       src="images/qtel-mainwindow-small.png"/>
</A>
The main window consists of four areas. The top-left is where to choose which
class of staions to view. All stations in a class is shown to the top-right.
There are four station classes: Conferences, Links,
Repeaters and (private) Stations. There also is a bookmark class which isn't a
real class. It's a collection of your favorite stations. Just right-click on a
station in any of the other views to add it to the bookmark list.
<P>
At left-bottom is a message area that shows the messages from the EchoLink
directory server. At right-bottom is a list of incoming connections. To accept
an incoming connection, highligt the station and press "Accept".
<BR clear="all"/>
<P/>
<A href="images/qtel-comdialog.png">
  <IMG class="floatright" border="0" alt="[Qtel communications dialog]"
       src="images/qtel-comdialog-small.png"/>
</A>
The communications dialog is used to perform a Qso to another station. To
activate it, double-click on a station line or just press the <em>enter</em> key
when the station is highlighted. To connect to the station, press the
<em>Connect</em> button. When audio is coming in from the remote station, the
green RX indicator will light up. To transmit, press the PTT button. The red TX
indicator will light up. To lock the PTT in transmit, first press and hold the
Ctrl-key and then press the PTT button. To disconnect, press the "Disconnect"
button.
<P/>
In the big, white area in the middle of the window, info messages from the
remote station are shown. Chat messages from the remote station is also shown
here. In the one-line white area below, chat messages can be typed that is sent
to the remote station.
<BR clear="all"/>
<P/>
<A href="images/qtel-settings.png">
  <IMG class="floatleft" border="0" alt="[Qtel settings dialog]"
       src="images/qtel-settings-small.png"/>
</A>
The configuration dialog is brought up the first time the application is
started. After that it can be brought up again by choosing the
<em>Settings/Qtel settings</em> menu. Most fields are self explainatory.
Location is the short string that is shown in the EchoLink directory server
listing. The info message is the one that is sent to the remote station upon
connection.
<P/>
In the <em>Directory Server</em> tab are some settings for the
EchoLink directory server connection. Hover the mouse cursor above each field to
get help about it.
<BR clear="all"/>
<P/>

That is about everything there is to say about the Qtel application. It's quite
simple.
<P/>


<?php include("footer.inc"); ?>
