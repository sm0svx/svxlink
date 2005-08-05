###############################################################################
#
# EchoLink module event handlers
#
###############################################################################

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
    if [file exists "$basedir/EchoLink/conf-$call.raw"] {
      playFile "$basedir/EchoLink/conf-$call.raw";
    } else {
      spellWord $name;
    }
  } else {
    spellWord $call;
  }
}


#
# Executed when 
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
# Executed when 
#
proc directory_server_offline {} {
  playMsg "EchoLink" "directory_server_offline";
}


#
# Executed when 
#
proc no_more_connections_allowed {} {
  # FIXME: Change the message to something that makes more sense...
  playMsg "EchoLink" "link_busy";
}


#
# Executed when 
#
proc status_report {} {
  variable num_connected_stations;
  playNumber $num_connected_stations;
  playMsg "EchoLink" "connected_stations";
}


#
# Executed when 
#
proc station_id_not_found {station_id} {
  playNumber $station_id;
  playMsg "EchoLink" "not_found";
}


#
# Executed when 
#
proc lookup_failed {station_id} {
  playMsg "EchoLink" "operation_failed";
}


#
# Executed when 
#
proc self_connect {} {
  playMsg "EchoLink" "operation_failed";
}


#
# Executed when 
#
proc already_connected_to {call} {
  playMsg "EchoLink" "already_connected_to";
  playSilence 50;
  spellEchoLinkCallsign $call;
}


#
# Executed when 
#
proc internal_error {} {
  playMsg "EchoLink" "operation_failed";
}


#
# Executed when 
#
proc connecting_to {call} {
  playMsg "EchoLink" "connecting_to";
  spellEchoLinkCallsign $call;
  playSilence 500;
}


#
# Executed when 
#
proc disconnected {call} {
  spellEchoLinkCallsign $call;
  playMsg "EchoLink" "disconnected";
  playSilence 500;
}


#
# Executed when 
#
proc remote_connected {call} {
  playMsg "EchoLink" "connected";
  spellEchoLinkCallsign $call;
  playSilence 500;
}


#
# Executed when 
#
proc connected {} {
  playMsg "EchoLink" "connected";
  playSilence 500;
}


#
# Executed when 
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
