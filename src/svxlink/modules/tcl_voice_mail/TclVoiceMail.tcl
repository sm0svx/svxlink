###############################################################################
#
# TclVoiceMail module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleVoiceMail] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval TclVoiceMail {

#
# Extract the module name from the current namespace
#
set module_name [namespace tail [namespace current]];

#
# The user id of the currently logged in user
#
set userid "";

#
# The recepient of a message being recorded.
#
set rec_rcpt "";

#
# The current state of the VoiceMail module
#
set state "idle";

#
# The directory where the voice mails are stored
#
set recdir "/var/spool/svxlink/voice_mail";

#
# Configuration file names
#
set cfg_etc "/etc/TclVoiceMail.conf";
set cfg_home "$env(HOME)/.svxlink/TclVoiceMail.conf";


#
# Read configuration file
#
if [file exists $cfg_etc] {
  source $cfg_etc;
} elseif [file exists $cfg_home] {
  source $cfg_home;
} else {
  puts "*** ERROR: Could not find a configuration file in module \"$module_name\". Tried \"$cfg_etc\" and \"$cfg_home\"";
}


#
# Read the specified user configuration variable for the specified user ID
#
proc id2var {id var} {
  variable users;
  array set user [split $users($id) " ="];
  if {[array names user -exact $var] != ""} {
    return $user($var);
  } else {
    return "";
  }
}


#
# An "overloaded" playMsg that eliminates the need to write the module name
# as the first argument.
#
proc playMsg {msg} {
  variable module_name;
  ::playMsg $module_name $msg;
}


#
# A convenience function for printing out information prefixed by the
# module name
#
proc printInfo {msg} {
  variable module_name;  
  puts "$module_name: $msg";
}


#
# Executed when this module is being activated
#
proc activating_module {} {
  variable state;
  variable module_name;
  
  Module::activating_module $module_name;
  playMsg "login";
  setState "login";
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  variable userid;
  variable module_name;
  
  Module::deactivating_module $module_name;
  set userid "";
  setState "idle";
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  variable module_name;
  abortRecording;
  Module::timeout $module_name;
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  variable module_name;
  Module::play_help $module_name;
}


#
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
proc dtmf_digit_received {char duration} {
  #printInfo "DTMF digit $char received with duration $duration milliseconds";
}


#
# Executed when a DTMF command is received
#
proc dtmf_cmd_received {cmd} {
  variable state;
  
  #printInfo "DTMF command received: $cmd";
  
  if {$state == "login"} {
    cmdLogin $cmd;
  } elseif {$state == "logged_in"} {
    if {$cmd == ""} {
      deactivateModule;
    } elseif {$cmd == "0"} {
      cmdPlayQuickHelp;
    } elseif {$cmd == "1"} {
      cmdPlayNextNewMessage $cmd;
    } elseif {[regexp {^2} $cmd]} {
      cmdRecordMessage $cmd;
    } else {
      playNumber $cmd;
      playMsg "unknown_command";
    }
  } elseif {[regexp {^rec_\w+} $state]} {
    cmdRecordMessage $cmd;
  } elseif {[regexp {^pnm_\w+} $state]} {
    cmdPlayNextNewMessage $cmd;
  } else {
    playMsg "operation_failed";
  }
}


#
# Executed when the squelch open or close. If it's open is_open is set to 1,
# otherwise it's set to 0.
#
proc squelch_open {is_open} {
  variable state;
  variable recdir;
  variable rec_rcpt;
  variable rec_timestamp;
  variable userid;
  variable mail_from_addr;
  variable mail_from_name;
  variable mail_subj;
  variable mail_msg;
  variable CFG_ID;
  variable ::Logic::CFG_CALLSIGN;
  
  if {$is_open} {set str "OPEN"} else { set str "CLOSED"};
  #printInfo "The squelch is $str";

  if {$state == "rec_subject"} {
    if {$is_open} {
      set rec_rcpt_call [id2var $rec_rcpt call];
      set subj_filename "$recdir/$rec_rcpt_call/$rec_timestamp";
      append subj_filename "_$userid.subj";
      printInfo "Recording subject to file: $subj_filename";
      recordStart $subj_filename;
    } else {
      recordStop;
      playMsg "rec_message";
      setState "rec_message";
    }
  } elseif {$state == "rec_message"} {
    set rec_rcpt_call [id2var $rec_rcpt call];
    set subj_filename "$recdir/$rec_rcpt_call/$rec_timestamp";
    append subj_filename "_$userid.subj";
    set mesg_filename "$recdir/$rec_rcpt_call/$rec_timestamp";
    append mesg_filename "_$userid.mesg";
    if {$is_open} {
      printInfo "Recording message to file: $mesg_filename";
      recordStart $mesg_filename;
    } else {
      recordStop;
      playMsg "rec_done";
      #playFile $subj_filename;
      #playSilence 1000;
      #playFile $mesg_filename;
      #playSilence 1000;
      playSilence 500;
      set email [id2var $rec_rcpt email];
      if {$email != ""} {
        printInfo "Sending notification e-mail to \"$email\"";
      	eval set msg \"$mail_msg\";
      	#exec mail -s "$mail_subj" $email -- -f $mail_from_addr \
	#	-F "$mail_from_name" << "$msg" &;
      	exec mutt -s "$mail_subj" $email \
		-e "set from=\"$mail_from_addr\"" \
		-e "set realname=\"$mail_from_name\"" \
		<< "$msg" &;
      }
      set rec_rcpt "";
      setState "logged_in";
    }
  }
}


#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
proc all_msgs_written {} {
  #printInfo "all_msgs_written called...";
}


#
# Executed when the state of this module should be reported on the radio
# channel. Typically this is done when a manual identification has been
# triggered by the user by sending a "*".
# This function will only be called if this module is active.
#
proc status_report {} {
  variable users;
  variable recdir;
  
  #printInfo "status_report called...";
  
  set user_list {};
  foreach userid [lsort [array names users]] {
    set call [id2var $userid call];
    if {[llength [glob -nocomplain -directory "$recdir/$call" *.subj]] > 0} {
      lappend user_list $call;
    }
  }
  if {[llength $user_list] > 0} {
    playMsg "messages_for";
    foreach call $user_list {
      spellWord $call;
      playSilence 250;  
    }
  }
}


proc setState {new_state} {
  variable state;
  set state $new_state;
  if {$state == "logged_in"} {
    playMsg "logged_in_menu";
  }
}

proc cmdLogin {cmd} {
  variable recdir;
  variable userid;
  variable users;
  variable state;
  
  if {$cmd == ""} {
    printInfo "Aborting login";
    playMsg "aborted";
    playSilence 500;
    deactivateModule;
    return;
  }
  
  set userid [string range $cmd 0 2];
  if {[array names users -exact "$userid"] != ""} {
    array set user [split $users($userid) " ="];
    set passwd [string range $cmd 3 end];
    if {$passwd == $user(pass)} {
      printInfo "User $user(call) logged in with password $user(pass)";
      spellWord $user(call);
      playMsg "login_ok";
      if {[file exists "$recdir/$user(call)"] != 1} {
        file mkdir "$recdir/$user(call)";
      }
      setState "logged_in";
    } else {
      printInfo "Wrong password ($passwd) for user $user(call)";
      playMsg "wrong_userid_or_password";
      playMsg "login";
    }
  } else {
    printInfo "Could not find user id $userid";
    playMsg "wrong_userid_or_password";
    playMsg "login";
  }
}


proc cmdPlayQuickHelp {} {
  #puts "cmdPlayQuickHelp";
  playMsg "help";
}


proc cmdPlayNextNewMessage {cmd} {
  variable recdir;
  variable userid;
  variable state;

  #puts "cmdPlayNextNewMessage";
  set call [id2var $userid call];
  set subjects [glob -nocomplain -directory "$recdir/$call" *.subj];
  set subjects [lsort -ascii -increasing $subjects];
  if {$state == "logged_in"} {
    set msg_cnt [llength $subjects];
    playNumber $msg_cnt;
    playMsg "new_messages";
    playSilence 500;
    printInfo "$msg_cnt new messages for $call";
    if {$msg_cnt > 0} {
      set basename [file rootname [lindex $subjects 0]];
      playFile "$basename.subj";
      playSilence 1000;
      playFile "$basename.mesg";
      playSilence 1000;
      playMsg "pnm_menu";
      setState "pnm_menu";
    }
  } elseif {$state == "pnm_menu"} {
    set basename [file rootname [lindex $subjects 0]];
    if {$cmd == "0"} {
      playMsg "pnm_menu";
    } elseif {$cmd == "1"} {
      printInfo "Deleting message $basename";
      file delete "$basename.subj" "$basename.mesg";
      playMsg "message_deleted";
      setState "logged_in";
    } elseif {$cmd == "2"} {
      printInfo "Reply to and delete message $basename";
      file delete "$basename.subj" "$basename.mesg";
      playMsg "message_deleted";
      regexp {\d{8}_\d{6}_(\d+)$} $basename -> sender;
      setState "rec_reply";
      cmdRecordMessage "x$sender";
    } elseif {$cmd == "3"} {
      printInfo "Replay message $basename";
      playFile "$basename.subj";
      playSilence 1000;
      playFile "$basename.mesg";
      playSilence 1000;
      playMsg "pnm_menu";
    } elseif {$cmd == ""} {
      printInfo "Aborted operation play next message";
      playMsg "aborted";
      setState "logged_in";
    } else {
      printInfo "Unknown command: $cmd";
      playNumber $cmd;
      playMsg "unknown_command";
    }
  }
}


proc abortRecording {} {
  variable recdir;
  variable rec_rcpt;
  variable rec_timestamp;
  variable userid;

  if {$rec_rcpt != ""} {
    printInfo "Aborted recording";
    set rec_rcpt_call [id2var $rec_rcpt call];
    set subj_filename "$recdir/$rec_rcpt_call/$rec_timestamp";
    append subj_filename "_$userid.subj";
    set mesg_filename "$recdir/$rec_rcpt_call/$rec_timestamp";
    append mesg_filename "_$userid.mesg";
    file delete $subj_filename $mesg_filename;
    set rec_rcpt "";
  }
}


proc cmdRecordMessage {cmd} {
  variable state;
  variable users;
  variable rec_rcpt;
  variable recdir;
  variable rec_timestamp;
  variable userid;
  
  #puts "cmdRecordMessage";
  
  if {($state == "logged_in") || ($state == "rec_reply")} {
    set rec_timestamp [clock format [clock seconds] -format "%Y%m%d_%H%M%S"];
    setState "rec_rcpt";
    if {[string length $cmd] == 1} {
      playMsg "rec_enter_rcpt";
    } else {
      cmdRecordMessage [string range $cmd 1 end];
    }
  } elseif {$state == "rec_rcpt"} {
    if {$cmd == ""} {
      printInfo "Aborted operation send new message";
      playMsg "aborted";
      setState "logged_in";
    } elseif {[array names users -exact "$cmd"] != ""} {
      array set user [split $users($cmd) " ="];
      printInfo "Sending voice mail to $user(call)";
      playMsg "rec_sending_to";
      spellWord $user(call);
      playSilence 500;
      playMsg "rec_subject";
      set rec_rcpt $cmd;
      set rec_rcpt_call [id2var $rec_rcpt call];
      if {[file exists "$recdir/$rec_rcpt_call"] != 1} {
        file mkdir "$recdir/$rec_rcpt_call";
      }
      setState "rec_subject";
    } else {
      printInfo "Could not find user id $cmd";
      playNumber $cmd;
      playMsg "unknown_userid";
      playSilence 500;
      playMsg "rec_enter_rcpt";
    }
  } else {
    playMsg "aborted";
    abortRecording;
    setState "logged_in";
  }
}


proc cmdPlayMessage {msg} {
  printInfo "cmdPlayMessage $msg";
}



# end of namespace
}


#
# This file has not been truncated
#
