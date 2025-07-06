#
# Read and execute (source) a TCL file
# Check if the given TCL file is readable before trying to source it. Print
# a warning if the file is not readable.
# The file is sourced in the context (frame) in use when calling this function.
# That for example mean that the callers namespace is used.
# If the filename is relative, the path to the file is resolved relative to the
# directory where the calling script file is located.
#
#   path - The file to read
#
proc sourceTcl {path} {
  set script_dir [file dirname [info script]]
  set path [file join "$script_dir" "$path"]
  if [file readable $path] {
    puts "$::logic_name: Loading $path"
    uplevel 1 source $path
  } else {
    puts "*** WARNING: Could not load TCL event file: $path"
  }
}


#
# This procedure will take a TCL filename as argument and source that file from
# one or more paths. All files found will be sourced with the highest priority
# file sorced last.
#
#   filename - The name of a tcl file
#
proc sourceTclWithOverrides {filename} {
  set paths [list \
    "$::basedir/events.d/$filename" \
    "$::langdir/events.d/$filename" \
    "$::langdir/events.d/local/$filename" \
    "$::basedir/events.d/local/$filename" \
    ]
  foreach path $paths {
    if [file readable $path] {
      uplevel 1 sourceTcl $path
    }
  }
}


#
# Get a variable and if it does not exist return the default value
#
#   varname - The name of the variable to get
#   default - The default value to set if the variable is undefined
#
#proc getVar {varname default} {
#  upvar $varname var
#  expr {[info exists var] ? $var : $default}
#}


#
# Mix in all variables and procedures from the given namespace
#
#   ns - The name of the namespace to min in
#
proc mixin {ns} {
  set ns "[uplevel namespace current]::${ns}"
  foreach var [uplevel namespace eval ${ns} info vars ${ns}::*] {
    set varname [namespace tail ${var}]
    uplevel namespace upvar ${ns} ${varname} ${varname}
  }
  uplevel namespace eval $ns namespace export *
  uplevel namespace import -force ${ns}::*
  set path [list {*}[uplevel namespace path] $ns]
  uplevel namespace path [list $path]
}


#
# Find the first file, from a list of candidates, that is readable
#
#   args  - One or more file path glob patterns
#
proc findFirstFileOf {args} {
  foreach path [glob -nocomplain {*}$args] {
    #puts "### $path"
    if [file readable $path] {
      return $path
    }
  }
  return ""
}


#
# Play a message in a certain context. A context can for example be Core,
# EchoLink, Help, Parrot etc. If a sound is not found in the specified context,
# a search in the "Default" context is done.
#
# It's also possible to have local overrides by putting files under a "local"
# directory either directly under the "sounds" directory or under the language
# pack directory. For example if context is "Core" and the language is set to
# "en_US" the following paths will be searched:
#
#   .../sounds/en_US/local/Core/
#   .../sounds/local/Core/
#   .../sounds/en_US/Core/
#   .../sounds/en_US/local/Default/
#   .../sounds/local/Default/
#   .../sounds/en_US/Default/
#
#   context   - The context to look for the sound files in (e.g Default,
#               Parrot etc).
#   msg       - The basename of the file to play
#   warn      - Set to 0 to not print a warning if no sound clip was found
#
proc playMsg {context msg {warn 1}} {
  set filename [findFirstFileOf \
      "$::langdir/local/$context/$msg.{wav,raw,gsm}" \
      "$::basedir/sounds/local/$context/$msg.{wav,raw,gsm}" \
      "$::langdir/$context/$msg.{wav,raw,gsm}" \
      "$::langdir/local/Default/$msg.{wav,raw,gsm}" \
      "$::basedir/sounds/local/Default/$msg.{wav,raw,gsm}" \
      "$::langdir/Default/$msg.{wav,raw,gsm}" \
      ]
  if {$filename != ""} {
    playFile "$filename"
  } else {
    if {$warn} {
      puts "*** WARNING: Could not find audio clip \"$msg\" in context \"$context\"";
    }
    return 0
  }
  return 1
}


#
# Play a range of subcommand description files. The file names must be on the
# format <basename><command number>[ABCD*#]. The last characters are optional.
# Each matching sound clip will be played in sub command number order, prefixed
# with the command number.
#
#   context   - The context to look for the sound files in (e.g Default,
#               Parrot etc).
#   basename  - The common basename for the sound clips to find.
#   header    - A header sound clip to play first
#
proc playSubcommands {context basename {header ""}} {
  set subcmds [glob -nocomplain "$::langdir/$context/$basename*.{wav,raw,gsm}"]
  if {[llength $subcmds] > 0} {
    if {$header != ""} {
      playSilence 500
      playMsg $context $header
    }

    append match_exp {^.*/} $basename {(\d+)([ABCD*#]*)\.}
    foreach subcmd [lsort $subcmds] {
      if [regexp $match_exp $subcmd -> number chars] {
        playSilence 200
        playNumber $number
        spellWord $chars
        playSilence 200
        playFile $subcmd
      }
    }
  }
}


#
# Recursively print the TCL namespace tree
#
#   ns      The namespace name
#   indent  Space characters used to indent
#
proc printNamespaceTree {{ns ::} {indent "  "}} {
  if {$ns == "::"} {
    puts "### TCL namespace tree:"
  }
  foreach child [namespace children $ns] {
    if {$child == "::tcl" || $child == "::oo" || $child == "::zlib"} {
      continue
    }
    set ns [namespace tail $child]
    puts "### ${indent}${ns}"
    printNamespaceTree $child "$indent  "
  }
}


