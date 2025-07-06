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

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module


#
# Spell an EchoLink callsign
#
proc spellEchoLinkCallsign {call} {
  global langdir
  if [regexp {^(\w+)-L$} $call ignored callsign] {
    spellWord $callsign
    playSilence 50
    playMsg "link"
  } elseif [regexp {^(\w+)-R$} $call ignored callsign] {
    spellWord $callsign
    playSilence 50
    playMsg "repeater"
  } elseif [regexp {^\*(.+)\*$} $call ignored name] {
    playMsg "conference"
    playSilence 50
    set lc_name [string tolower $name]
    if [file exists "$langdir/EchoLink/conf-$lc_name.wav"] {
      playFile "$langdir/EchoLink/conf-$lc_name.wav"
    } else {
      spellEchoLinkCallsign $name
    }
  } else {
    spellWord $call
  }
}


#
# Executed when a request to list all connected stations is received.
# That is, someone press DTMF "1#" when the EchoLink module is active.
#
proc list_connected_stations {connected_stations} {
  playNumber [llength $connected_stations];
  playSilence 50;
  playMsg "connected_stations";
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
  playMsg "directory_server_offline";
}


#
# Executed when the limit for maximum number of QSOs has been reached and
# an outgoing connection request is received.
#
proc no_more_connections_allowed {} {
  # FIXME: Change the message to something that makes more sense...
  playMsg "link_busy";
}


#
# Executed when a status report is requested. This usually happens at
# manual identification when the user press DTMF "*".
#
proc status_report {} {
  variable num_connected_stations;
  variable module_name;
  global active_module;

  if {$active_module == $module_name} {
    playNumber $num_connected_stations;
    playMsg "connected_stations";
  }
}


#
# Executed when an EchoLink id cannot be found in an outgoing connect request.
#
proc station_id_not_found {station_id} {
  playNumber $station_id;
  playMsg "not_found";
}


#
# Executed when the lookup of an EchoLink callsign fail in an outgoing connect
# request.
#
proc lookup_failed {station_id} {
  playMsg "operation_failed";
}


#
# Executed when a local user tries to connect to the local node.
#
proc self_connect {} {
  playMsg "operation_failed";
}


#
# Executed when a local user tries to connect to a node that is already
# connected.
#
proc already_connected_to {call} {
  playMsg "already_connected_to";
  playSilence 50;
  spellEchoLinkCallsign $call;
}


#
# Executed when an internal error occurs.
#
proc internal_error {} {
  playMsg "operation_failed";
}


#
# Executed when an outgoing connection has been requested.
#
proc connecting_to {call} {
  playMsg "connecting_to";
  spellEchoLinkCallsign $call;
  playSilence 500;
}


#
# Executed when an EchoLink connection has been terminated
#
proc disconnected {call} {
  spellEchoLinkCallsign $call;
  playMsg "disconnected";
  playSilence 500;
}


#
# Executed when an incoming EchoLink connection has been accepted.
#
proc remote_connected {call} {
  playMsg "connected";
  spellEchoLinkCallsign $call;
  playSilence 500;
}


#
# Executed when an outgoing connection has been established.
#   call - The callsign of the remote station
#
proc connected {call} {
  #puts "Outgoing Echolink connection to $call established"
  playMsg "connected";
  playSilence 500;
}


#
# Executed when the list of connected remote EchoLink clients changes
#   client_list - List of connected clients
#
proc client_list_changed {client_list} {
  #foreach {call} $client_list {
  #  puts $call
  #}
}


#
# Executed when the EchoLink connection has been idle for too long. The
# connection will be terminated.
#
proc link_inactivity_timeout {} {
  playMsg "timeout";
}


#
# Executed when a too short connect by callsign command is received
#
proc cbc_too_short_cmd {cmd} {
  spellWord $cmd;
  playSilence 50;
  playMsg "operation_failed";
}


#
# Executed when the connect by callsign function cannot find a match
#
proc cbc_no_match {code} {
  playNumber $code;
  playSilence 50;
  playMsg "no_match";
}


#
# Executed when the connect by callsign list has been retrieved
#
proc cbc_list {call_list} {
  playMsg "choose_station";
  set idx 0;
  foreach {call} $call_list {
    incr idx;
    playSilence 500;
    playNumber $idx;
    playSilence 200;
    spellEchoLinkCallsign $call;
  }
}


#
# Executed when the connect by callsign function is manually aborted
#
proc cbc_aborted {} {
  playMsg "aborted";
}


#
# Executed when an out of range index is entered in the connect by callsign
# list
#
proc cbc_index_out_of_range {idx} {
  playNumber $idx;
  playSilence 50;
  playMsg "idx_out_of_range";
}


#
# Executed when there are more than nine matches in the connect by
# callsign function
#
proc cbc_too_many_matches {} {
  playMsg "too_many_matches";
}


#
# Executed when no station have been chosen in 60 seconds in the connect
# by callsign function
#
proc cbc_timeout {} {
  playMsg "aborted";
}


#
# Executed when the disconnect by callsign list has been retrieved
#
proc dbc_list {call_list} {
  playMsg "disconnect_by_callsign";
  playSilence 200
  playMsg "choose_station";
  set idx 0;
  foreach {call} $call_list {
    incr idx;
    playSilence 500;
    playNumber $idx;
    playSilence 200;
    spellEchoLinkCallsign $call;
  }
}


#
# Executed when the disconnect by callsign function is manually aborted
#
proc dbc_aborted {} {
  playMsg "disconnect_by_callsign";
  playMsg "aborted";
}


#
# Executed when an out of range index is entered in the disconnect by callsign
# list
#
proc dbc_index_out_of_range {idx} {
  playNumber $idx;
  playSilence 50;
  playMsg "idx_out_of_range";
}


#
# Executed when no station have been chosen in 60 seconds in the disconnect
# by callsign function
#
proc dbc_timeout {} {
  playMsg "disconnect_by_callsign";
  playMsg "timeout";
}


#
# Executed when a local user enter the DTMF code for playing back the
# local node ID.
#
proc play_node_id {my_node_id} {
  playMsg "node_id_is";
  playSilence 200;
  if { $my_node_id != 0} {
    playNumber $my_node_id;
  } else {
    playMsg "unknown";
  }
}


#
# Executed when an entered command failed or have bad syntax.
#
proc command_failed {cmd} {
  spellWord $cmd;
  playMsg "operation_failed";
}


#
# Executed when an unrecognized command has been received.
#
proc unknown_command {cmd} {
  spellWord $cmd;
  playMsg "unknown_command";
}


#
# Executed when the listen only feature is activated or deactivated
#   status    - The current status of the feature (0=deactivated, 1=activated)
#   activate  - The requested new status of the feature
#               (0=deactivate, 1=activate)
#
proc listen_only {status activate} {
  variable module_name;

  if {$status == $activate} {
    playMsg "listen_only";
    playMsg [expr {$status ? "already_active" : "not_active"}];
  } else {
    puts "$module_name: [expr {$activate ? "Activating" : "Deactivating"}]\
          listen only mode.";
    playMsg [expr {$activate ? "activating" : "deactivating"}];
    playMsg "listen_only";
  }
}


#
# Executed when an outgoing connection is rejected. This can happen if
# REJECT_OUTGOING and/or ACCEPT_OUTGOING has been setup.
#
proc reject_outgoing_connection {call} {
  spellEchoLinkCallsign $call;
  playSilence 50;
  playMsg "reject_connection";
}


#
# Executed when a transmission from an EchoLink station is starting
# or stopping
#   rx   - 1 if receiving or 0 if not
#   call - The callsign of the remote station
#
proc is_receiving {rx call} {
  if {[getConfigValue $::logic_name LOCAL_RGR_SOUND 1] && !$rx} {
    playTone 1000 100 100
  }
}


#
# Executed when a chat message is received from a remote station
#
#   msg -- The message text
#
# WARNING: This is a slightly dangerous function since unexepected input
# may open up a security flaw. Make sure that the message string is handled
# as unknown data that can contain anything. Check it thoroughly before
# using it. Do not run SvxLink as user root.
proc chat_received {msg} {
  #puts $msg
}


#
# Executed when an info message is received from a remote station
#
#   call -- The callsign of the sending station
#   msg  -- The message text
#
# WARNING: This is a slightly dangerous function since unexepected input
# may open up a security flaw. Make sure that the message string is handled
# as unknown data that can contain anything. Check it thoroughly before
# using it. Do not run SvxLink as user root.
proc info_received {call msg} {
  #puts "$call: $msg"
}


#
# Executed when a configuration variable is updated at runtime
#
proc config_updated {tag value} {
  #puts "Configuration variable updated: $tag=$value"
}


# end of namespace
}

#
# This file has not been truncated
#
