###############################################################################
#
# EchoLink module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleEchoLink] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval EchoLink {


#
# This variable is updated by the EchoLink module when a station connects or
# disconnects.
#
variable num_connected_stations 0;


#
# Executed when this module is being activated
#
proc activating_module {} {
  Module::activating_module "EchoLink";
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  Module::deactivating_module "EchoLink";
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  Module::timeout "EchoLink";
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  Module::play_help "EchoLink";
}


#
# Spell an EchoLink callsign
#
proc spellEchoLinkCallsign {call} {
  global basedir;
  if [regexp {^(\w+)-L$} $call ignored callsign] {
    spellWord $callsign;
    playMsg "EchoLink" "link";
  } elseif [regexp {^(\w+)-R$} $call ignored callsign] {
    spellWord $callsign;
    playMsg "EchoLink" "repeater";
  } elseif [regexp {^\*(\w+)\*$} $call ignored name] {
    playMsg "EchoLink" "conference";
    playSilence 50;
    set name [string tolower $name];
    if [file exists "$basedir/EchoLink/conf-$name.raw"] {
      playFile "$basedir/EchoLink/conf-$name.raw";
    } else {
      spellWord $name;
    }
  } else {
    spellWord $call;
  }
}


#
# Executed when a request to list all connected stations is received.
# That is, someone pressed DTMF "1#" when the EchoLink modules was active.
#
proc list_connected_stations {connected_stations} {
  playNumber [llength $connected_stations];
  playSilence 50;
  playMsg "EchoLink" "connected_stations";
  foreach {call} "$connected_stations" {
    spellEchoLinkCallsign $call;
    playSilence 250;
  }
}


#
# Executed when someone tries to setup an outgoing EchoLink connection but
# the directory server is offline due to communications failure.
#
proc directory_server_offline {} {
  playMsg "EchoLink" "directory_server_offline";
}


#
# Executed when the limit for maximum number of QSOs has been reached and
# an outgoing connection request is received.
#
proc no_more_connections_allowed {} {
  # FIXME: Change the message to something that makes more sense...
  playMsg "EchoLink" "link_busy";
}


#
# Executed when a status report is requested. This usually happens at
# manual identification when the user press DTMF "*".
#
proc status_report {} {
  variable num_connected_stations;
  playNumber $num_connected_stations;
  playMsg "EchoLink" "connected_stations";
}


#
# Executed when an EchoLink id cannot be found in an outgoing connect request.
#
proc station_id_not_found {station_id} {
  playNumber $station_id;
  playMsg "EchoLink" "not_found";
}


#
# Executed when the lookup of an EchoLink callsign fail in an outgoing connect
# request.
#
proc lookup_failed {station_id} {
  playMsg "EchoLink" "operation_failed";
}


#
# Executed when a local user tries to connect to the local node.
#
proc self_connect {} {
  playMsg "EchoLink" "operation_failed";
}


#
# Executed when a local user tries to connect to a node that is already
# connected.
#
proc already_connected_to {call} {
  playMsg "EchoLink" "already_connected_to";
  playSilence 50;
  spellEchoLinkCallsign $call;
}


#
# Executed when an internal error occurs.
#
proc internal_error {} {
  playMsg "EchoLink" "operation_failed";
}


#
# Executed when an outgoing connection has been requested.
#
proc connecting_to {call} {
  playMsg "EchoLink" "connecting_to";
  spellEchoLinkCallsign $call;
  playSilence 500;
}


#
# Executed when an EchoLink connection has been terminated
#
proc disconnected {call} {
  spellEchoLinkCallsign $call;
  playMsg "EchoLink" "disconnected";
  playSilence 500;
}


#
# Executed when an incoming EchoLink connection has been accepted.
#
proc remote_connected {call} {
  playMsg "EchoLink" "connected";
  spellEchoLinkCallsign $call;
  playSilence 500;
}


#
# Executed when an outgoing connection has been established.
#
proc connected {} {
  playMsg "EchoLink" "connected";
  playSilence 500;
}


#
# Executed when the EchoLink connection has been idle for too long. The
# connection will be terminated.
#
proc link_inactivity_timeout {} {
  playMsg "EchoLink" "timeout";
}




#-----------------------------------------------------------------------------
# The events below are for remote EchoLink announcements. Sounds are not
# played over the local transmitter but are sent to the remote station.
#-----------------------------------------------------------------------------

#
# Executed when an incoming connection is accepted
#
proc remote_greeting {} {
  playSilence 1000;
  playMsg "EchoLink" "greeting";
}


#
# Executed when an incoming connection is rejected
#
proc reject_remote_connection {} {
  playSilence 1000;
  playMsg "EchoLink" "reject_connection";
  playSilence 1000;
}


#
# Executed when the inactivity timer times out
#
proc remote_timeout {} {
  playMsg "EchoLink" "timeout";
  playSilence 1000;
}



# end of namespace
}

#
# This file has not been truncated
#
