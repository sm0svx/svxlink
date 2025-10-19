###############################################################################
#
# Global procedures and associated variables
#
###############################################################################

# Ensure that everything is loaded in the global namespace
namespace eval :: {

# Internal variable used by the override procedure
variable overridden_cnt 0

# Set up a variable used to identify the source of printouts
variable print_prefix ${::logic_name}
if [info exists ::module_name] {
  set print_prefix ${::module_name}
}


#
# A convenience function for printing out a debug message prefixed by the
# logic or module name
#
#   msg The message to print
#
proc printDebug {msg} {
  puts "### ${::print_prefix}: ${msg}"
}


#
# A convenience function for printing out information prefixed by the
# logic or module name
#
#   msg The message to print
#
proc printInfo {msg} {
  puts "${::print_prefix}: ${msg}"
}
printInfo "Loading [file normalize [info script]]"


#
# A convenience function for printing out a notice prefixed by the
# logic or module name
#
#   msg The message to print
#
proc printNotice {msg} {
  puts "NOTICE\[${::print_prefix}\]: ${msg}"
}


#
# A convenience function for printing out a warning message prefixed by a tag
# containing the logic or module name
#
#   msg The message to print
#
proc printWarning {msg} {
  puts "*** WARNING\[${::print_prefix}\]: ${msg}"
}


#
# A convenience function for printing out a error message prefixed by a tag
# containing the logic or module name
#
#   msg The message to print
#
proc printError {msg} {
  puts "*** ERROR\[${::print_prefix}\]: ${msg}"
}


#
# Prefix an in-proc proc declaration with 'local' to ensure that it is not
# visible outside of the containing procedure.
#
#   proc - The literal "proc"
#   name - The name of the function to declare
#   args - The argument list for the function to declare
#   body - The body of the function
#
#proc local {"proc" name args body} {
#  regsub -all @name {
#    set __@name {}
#    trace add variable __@name unset {rename @name "" ;#}
#  } $name xbody
#  uplevel 1 $xbody
#  proc $name $args $body
#}


#
# Prefix a procedure with 'override' to override an existing procedure.
# The purpose of using this procedure is that it provides the overriding
# procedure with access to the overridden procedure. The overridden procedure
# can be called using $SUPER.
# Even when the $SUPER procedure is not used from the overriding procedure it is
# good to always use 'override' when overriding a procedure. Then it will be
# ensured that there actually is a function to override. That is good for
# future compatibility and for finding bugs.
#
#   proc  - The literal "proc"
#   pname - The name of the function to override
#   pargs - The argument list for the function to override
#   pbody - The body of the overriding function
#
proc override {pliteral pname pargs pbody} {
  variable overridden_cnt

  if {$pliteral != "proc"} {
    error "missing proc literal: should be \"override proc pname pargs pbody\""
  }
  if {[llength [uplevel info procs $pname]] == 0} {
    error "no '$pname' procedure: cannot override a non-existing procedure"
  }
  set super_name overridden_[incr overridden_cnt]_$pname
  set decl "
    rename $pname $super_name
    proc $pname {$pargs} {
      set SUPER $super_name
      $pbody
    }"
  #puts $decl
  eval uplevel {$decl}
}


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
    printInfo "Loading $path"
    uplevel 1 source $path
  } else {
    printWarning "Could not load TCL event file: $path"
  }
}


#
# This procedure will take a TCL filename as argument and source that file from
# one or more override paths. All files found will be sourced with the highest
# priority file sorced last.
#
#   filename - The name of a tcl file
#
proc sourceTclOverrides {filename} {
  set paths [list \
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
# This procedure will take a TCL filename as argument and source that file from
# the main and override paths. All files found will be sourced with the highest
# priority file sorced last.
# If no file is found in any of the paths, the function will error out.
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
  set found 0
  foreach path $paths {
    if [file readable $path] {
      uplevel 1 sourceTcl $path
      set found 1
    }
  }
  if {!$found} {
    error "Could not source TCL file '$filename'"
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
    #printDebug "$path"
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
      printWarning "Could not find audio clip '$msg' in context '$context'"
    }
    return 0
  }
  return 1
}


#
# Recursively print the TCL namespace tree
#
#   ns      The namespace name
#   indent  Space characters used to indent
#
proc printNamespaceTree {{ns ::} {indent "  "}} {
  if {$ns == "::"} {
    printDebug "TCL namespace tree:"
  }
  foreach child [namespace children $ns] {
    if {$child == "::tcl" || $child == "::oo" || $child == "::zlib"} {
      continue
    }
    set ns [namespace tail $child]
    printDebug "${indent}${ns}"
    printNamespaceTree $child "$indent  "
  }
}


#
# Process the given event.
# All TCL modules should use this function instead of calling playMsg etc
# directly. The module code should only contain the logic, not the handling
# of the events.
#
#   ev   - An event
#   args - namespaces
#
proc processEvent {args ev} {
  set cmd ${ev}
  if {[llength $args] > 0} {
    set cmd [join ${args} ::]::${cmd}
  }
  if {[string range $cmd 0 1] != "::"} {
    set cmd ::${::logic_name}::${cmd}
  }
  eval ${cmd}
}


# end of namespace
}
