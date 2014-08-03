#-----------------------------------------------------------------
# Filename:		makefile.inc
# Author:		Tobias Blomberg
# Created:		1997-12-11
# Description:		This is the central part of this makefile
#			suite. This is the place where all the
#			rules and "magic" variables are specified.
#			All Makefiles should include this file.
#			The only thing in this file you may have
#			to change is the variables LIB, BIN and INC
#			(see below).
#
# Bugs:			* If you are compiling multiple binaries,
#			the dependencies are a little bit "strong".
#			All programs are relinked instead of just
#			one if you make a change in a source file.
#			At least, it's not the other way around...
#-----------------------------------------------------------------


############################################################################
#
# You should NOT need to change anything below this line.
# Trespassers will be shot on sight.
#
############################################################################

ifndef CFG_INCLUDED
  include $(ROOT)/makefile.cfg
endif

ifndef MAKEFILE_CONFIG_INCLUDED
  include $(ROOT)/.config
endif

ifdef INSTALL_ROOT
  $(warning INSTALL_ROOT is deprecated. Use DESTDIR instead)
  DESTDIR=$(INSTALL_ROOT)
endif

LIB		:= $(ROOT)/$(LIB)
INC		:= $(ROOT)/$(INC)
BIN		:= $(ROOT)/$(BIN)
RPM_DIR       	:= $(ROOT)/$(RPM_DIR)

# Path used by gcc to find library/include files. If you want to add a path
# to the list. Use a "+=" after including this file.
#   Ex. LIBPATH += /usr/X11R6/lib
LIBPATH		= -L$(LIB)
INCPATH 	+= -I$(INC)

# Complete QTHEADERS/LIB_QT_HEADERS with the ones generated from ".ui" files.
#QTHEADERS	+= $(UI_FILES:%.ui=%.h)
#LIB_QTHEADERS	+=  $(LIB_UI_FILES:%.ui=%.h)

# All source files
SRCS		= $(LIBSRC) $(PRGSRC) $(notdir $(PROGS:%=%.c)) \
      	      	  $(notdir $(CPPPROGS:%=%.cpp)) $(QTHEADERS:%.h=moc_%.cpp) \
		  $(LIB_QTHEADERS:%.h=moc_%.cpp) $(notdir $(PLUGINS:%=%.c)) \
		  $(PLUGINSRC) $(notdir $(PLUGINS_CPP:%=%.cpp))

# All object files that should go in the lib
LIBOBJS		:= $(LIBSRC:.c=.o) $(LIB_QTHEADERS:%.h=moc_%.o)
LIBOBJS		:= $(LIBOBJS:.cpp=.o)

# All objects, besides the mainfile, needed to link $(PROGS)
PRGOBJS		:= $(PRGSRC:.c=.o) $(QTHEADERS:%.h=moc_%.o)
PRGOBJS		:= $(PRGOBJS:.cpp=.o)

# All binary files compilation of plugins generate.
PLUGINSRCOBJS	:= $(PLUGINSRC:%.c=%.o)
PLUGINSRCOBJS	:= $(PLUGINSRCOBJS:%.cpp=%.o)
PLUGINOBJS    	:= $(PLUGINSRCOBJS) $(notdir $(PLUGINS:%=%.o)) \
		   $(notdir $(PLUGINS_CPP:%=%.o)) plugin_lib.a

# Dependency files to create
DEP_FILES	:= $(SRCS:%.c=depend/%.d)
DEP_FILES     	:= $(DEP_FILES:%.cpp=depend/%.dpp)

# If any LIBNAME defined, create a libfile
ifdef LIBNAME
  LIBFILE	= $(LIB)/lib$(LIBNAME).a
  ifdef SHLIB_MAJOR
    SHLIB_VERSION := $(SHLIB_MAJOR)
    ifdef SHLIB_MINOR
      SHLIB_VERSION := $(SHLIB_VERSION).$(SHLIB_MINOR).0
    endif
  endif
  ifneq ($(strip $(SHLIB_VERSION)),)
    SHLIBFILE	= $(LIB)/lib$(LIBNAME)-$(SHLIB_VERSION).so
  endif
endif

ifdef SHLIB_VERSION
  SHLIB_MAJOR=$(shell echo $(SHLIB_VERSION) | cut -d. -f1)
  SHLIB_MINOR=$(shell echo $(SHLIB_VERSION) | cut -d. -f2)
  SHLIB_FIX=$(shell echo $(SHLIB_VERSION) | cut -d. -f3)
endif


# Dependency list created from the PROJLIBS list
LIBDEP		:= $(subst -l,$(LIB)/lib,$(PROJLIBS))
LIBDEP		:= $(LIBDEP:%=%.a) $(SHLIBFILE)

# Compiler warnings
CWARNINGS	= -Wall -Wstrict-prototypes -Wpointer-arith
# -DNeedVarargsPrototypes -fpcc-struct-return -fno-common

CXXWARNINGS	= -Wall -Wpointer-arith
# -DNeedVarargsPrototypes -fpcc-struct-return -fno-common


# Bash-script to find out the relative path from a given directory
# to the current directory.
REVPATH = \
	calc_revpath() { \
	  other_path=$$(cd $$1; pwd); \
	  path_to=$${other_path\#\#$${PWD%/}}; \
	  unset revpath; \
	  while [ "$$path_to" = "$$other_path" -a -n "$${PWD%/}" ]; do \
	    revpath=`basename $${PWD}`/$${revpath}; \
	    cd ..; \
	    path_to=$${other_path\#\#$${PWD%/}}; \
	  done; \
	  while [ -n "$$path_to" -a "$$path_to" != "/" ]; do \
	    revpath=../$${revpath}; \
	    path_to=$${path_to%%/`basename $$path_to`}; \
	  done; \
	  revpath=$${revpath:-.}; \
	  revpath=$${revpath%/}$${path_to%/}; \
	  echo $${revpath:-.}; \
	}; calc_revpath

# Bash script to install files. It takes two arguments which consist of
# a filename (with optional permissions) and the target directory.
# To inhibit the chown and chgrp operations the environment variable
# NO_CHOWN can be set to 1.
#   Example: $(INSTALL) kalle[750,-,mygroup] /usr/bin
#            kalle is the file to install
#            [750,-,mygroup] sais: use mode 750, preserve owner, set group mygroup.
#            /usr/bin is the target directory
INSTALL = install_file() { \
	    local items file perm owner group modifier target_dir target_file; \
	    items="`echo $$1 | tr '[,]' ' '` - - -"; \
	    file=`echo $$items | cut -d' ' -f1`; \
	    perm=`echo $$items | cut -d' ' -f2`; \
	    owner=`echo $$items | cut -d' ' -f3`; \
	    group=`echo $$items | cut -d' ' -f4`; \
	    modifier="$$(echo $$items | cut -d' ' -f5)"; \
	    if [ -n "$(DESTDIR)" ]; then \
	      target_dir=$(DESTDIR)$$2; \
	    else \
	      target_dir=$$2; \
	    fi; \
	    target_file="$${target_dir}/$$(basename $${file})"; \
	    [ -d $${target_dir} ] || mkdir -p $${target_dir}; \
	    if [ "$${modifier}" = "config" -a -e "$${target_file}" ]; then \
	      $(ECHO) "***" Config file $(BOLD)$${target_file}$(NBOLD) \
	                  already exist. $(BOLD)Skipping$(NBOLD)...; \
	    else \
	      $(ECHO) --- Installing $(BOLD)$$file$(NBOLD) in \
	              $(BOLD)$${target_dir}$(NBOLD)...; \
	      cp -pdf $$file $${target_dir}; \
	      [ -n "$$perm" -a "$$perm" != '-' ] \
	          && chmod $$perm $${target_dir}/`basename $$file`; \
	      [ -z "$$NO_CHOWN" -a -n "$$owner" -a "$$owner" != '-' ] \
	          && { $(CHOWN_H) $$owner $${target_dir}/`basename $$file`; }; \
	      [ -z "$$NO_CHOWN" -a -n "$$group" -a "$$group" != '-' ] \
	          && { $(CHGRP_H) $$group $${target_dir}/`basename $$file`; }; \
	    fi; \
	    return 0; \
	  }; install_file


# Bash script to uninstall files. It takes two arguments which consist of
# the directory to remove the file from and the actual filename(s) to remove.
# The filename may contain both path elements (ignored) and mode/owner info
# (ignored).
#   Example: $(UNINSTALL) /usr/bin nisse/kalle[750,-,mygroup] pelle
#            /usr/bin - is the directory to remove the file from
#            nisse/kalle[750,-,mygroup] - First file to remove (/usr/bin/kalle)
#            pelle - Second file to remove (/usr/bin/pelle)
UNINSTALL = \
	uninstall_file() { \
	  local dir file; \
	  dir=$$1; shift; \
	  dir="$(DESTDIR)$${dir}"; \
	  for file in $$*; do \
	    file=$$(basename $${file} | cut -d'[' -f1); \
	    $(RM) $${dir}/$${file}; \
	  done; \
	  if [ -d "$${dir}" ]; then \
	    rmdir $${dir} 2> /dev/null; \
	  fi; \
	  return 0; \
	}; uninstall_file


# Bash script to create RPM file listings. It takes one argument which is
# the filename (with optional permissions).
#   Example: $(GEN_RPM_ROW) kalle[750,-,mygroup] /usr/bin
#            kalle is the file to install
#            [750,-,mygroup] sais: use mode 750, preserve owner, set group mygroup.
#            /usr/bin is the directory to install the file into
GEN_RPM_ROW = generate_rpm_file_row() { \
	    local items file perm owner group file; \
	    items="`echo $$1 | tr '[,]' ' '` - - - -"; \
	    file=`echo $$items | cut -d' ' -f1`; \
	    file=`basename $$file`; \
	    file="$$2/$$file"; \
	    perm=`echo $$items | cut -d' ' -f2`; \
	    owner=`echo $$items | cut -d' ' -f3`; \
	    group=`echo $$items | cut -d' ' -f4`; \
	    modifier=" %$$(echo $$items | cut -d' ' -f5)"; \
	    if [ "$${modifier}" = " %-" ]; then \
	      modifier=""; \
	    fi; \
	    $(ECHO) "%attr($$perm,$$owner,$$group)$$modifier $$file"; \
	    return 0; \
	  }; generate_rpm_file_row


# Macro used in the "install-extras", "uninstall-extras" and "rpm-extras"
# targets to specify directories.
#   Example: $(DIRSPEC) /usr/share/xyz[-,root,bin]
#            This row means different things depending on which target has
#            been run.
#            install-extras:   Create the directory /usr/share/xyz and set the
#                              owner/group to root/bin.
#            uninstall-extras: Remove the directory /usr/share/xyz
#            rpm-extras:       Include the directory /usr/share/xyz in the
#                              generated RPM and set its attributes to
#                              (-,root,bin).
DIRSPEC = \
	dirspec() { \
	  local items dir perm owner group file; \
	  items="`echo $$1 | tr '[,]' ' '` - - -"; \
	  dir=`echo $$items | cut -d' ' -f1`; \
	  perm=`echo $$items | cut -d' ' -f2`; \
	  owner=`echo $$items | cut -d' ' -f3`; \
	  group=`echo $$items | cut -d' ' -f4`; \
	  if [ -n "$(DESTDIR)" ]; then \
	    target_dir=$(DESTDIR)$${dir}; \
	  else \
	    target_dir=$${dir}; \
	  fi; \
	  if [ "$@" = "install-extras" ]; then \
	    $(MKDIR) $${target_dir} -p; \
	    [ -n "$$perm" -a "$$perm" != '-' ] \
	        && chmod $$perm $${target_dir}; \
	    [ -z "$$NO_CHOWN" -a -n "$$owner" -a "$$owner" != '-' ] \
	        && { $(CHOWN_H) $$owner $${target_dir}; }; \
	    [ -z "$$NO_CHOWN" -a -n "$$group" -a "$$group" != '-' ] \
	        && { $(CHGRP_H) $$group $${target_dir}; }; \
	  elif [ "$@" = "rpm-extras" ]; then \
	    $(ECHO) "%attr($$perm,$$owner,$$group) %dir $$dir" \
	    		>> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	  elif [ "$@" = "uninstall-extras" ]; then \
	    $(RMDIR) $${target_dir}; \
	  else \
	    echo "DIRSPEC macro used in unknown target \"$@\""; \
	    return 1; \
	  fi; \
	  return 0; \
	}; dirspec


# Macro used in the "install-extras", "uninstall-extras" and "rpm-extras"
# targets to specify files.
#   Example: $(FILESPEC) extras/config[-,root,bin] /etc
#            This row means different things depending on which target has
#            been run.
#            install-extras:   Install the file "config" located in the
#                              subdirectory extras into directory /etc and
#                              change the owner/group to root/bin.
#            uninstall-extras: Remove the file "config" from the directory /etc
#            rpm-extras:       Include the file /etc/config in the generated
#                              RPM and set its attributes to (-,root,bin).
FILESPEC = \
	filespec() { \
	  if [ "$@" = "install-extras" ]; then \
	    $(INSTALL) $$1 $$2; \
	  elif [ "$@" = "rpm-extras" ]; then \
	    $(GEN_RPM_ROW) $$1 $$2 >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	  elif [ "$@" = "uninstall-extras" ]; then \
	    $(UNINSTALL) $$2 $$1; \
	  else \
	    echo "FILESPEC macro used in unknown target \"$@\""; \
	    return 1; \
	  fi; \
	  return 0; \
	}; filespec


RM = remove_file() { \
       local options files file; \
       while [ -n "$$1" ]; do \
         if echo $$1 | grep -qE "^-"; then \
	   options="$$options $$1"; \
	 else \
	   files="$$files $$1"; \
	 fi; \
	 shift; \
       done; \
       for file in $$files; do \
         if [ -e "$$file" -o -L "$$file" ]; then \
           $(ECHO) --- Removing $(BOLD)$$file$(NBOLD)...; \
	   rm $$options $$file; \
	 fi \
       done; \
     }; remove_file

# Some useful printing macros
BOLD		= "\033[1m"
NBOLD		= "\033[0m"

#ECHO		= /bin/echo -e
BECHO		= $(ECHO) $(BOLD)txt $(NBOLD)

# Compiler
CFLAGS += $(WARNINGS) $(CWARNINGS) $(INCPATH) -fPIC $(CFLAGS_DEFINES)
CXXFLAGS += $(WARNINGS) $(CXXWARNINGS) $(INCPATH) -fPIC $(CFLAGS_DEFINES)
ifdef BUILD_RELEASE
  CFLAGS += $(RELEASE_CFLAGS)
  CXXFLAGS += $(RELEASE_CFLAGS)
else
  CFLAGS += $(DEBUG_CFLAGS)
  CXXFLAGS += $(DEBUG_CFLAGS)
endif
CCOMP		:= $(CC)
CXXCOMP		:= $(CXX)
CC		= $(ECHO) --- Compiling $(BOLD)$(@:.o=.c)$(NBOLD)...; $(CCOMP) $(CFLAGS)
CXX		= $(ECHO) --- Compiling $(BOLD)$(@:.o=.cpp)$(NBOLD)...; $(CXXCOMP) $(CXXFLAGS)


# Linker
LDFLAGS		+= -rdynamic -Wl,-rpath-link,$(LIB)
LINK		= $(ECHO) --- Linking $(BOLD)$@$(NBOLD)...; $(CCOMP) $(LIBPATH) $(LDFLAGS)
CPPLINK		= $(ECHO) --- Linking $(BOLD)$@$(NBOLD)...; $(CXXCOMP) $(LIBPATH) $(LDFLAGS)

# QtMoc
#QTMOC		= $(QTDIR)/bin/moc

# Qt User Interface Compiler
#QTUIC		= $(QTDIR)/bin/uic

# Create directory
MKDIR		= create_directory() { \
		    $(ECHO) --- Creating directory $(BOLD)$$1$(NBOLD)...; \
                    mkdir $$*; \
                  }; create_directory

# Remove directory
RMDIR		= remove_directory() { \
		    if [ -d $$1 ]; then \
		      $(ECHO) --- Removing directory $(BOLD)$$1$(NBOLD)...; \
                      rmdir $$*; \
		    fi \
                  }; remove_directory

# Rules to make dependency files.
depend/%.dpp:  	%.cpp
	if [ ! -d `dirname $@` ]; then \
		mkdir -p `dirname $@`; \
	fi
	$(ECHO) --- Creating dependency file $(BOLD)$@$(NBOLD)...
	$(CXXCOMP) -M -MG -MT "$*.o $@" $(CXXFLAGS) -MF $@ $< > /dev/null

depend/%.d:	%.c
	if [ ! -d `dirname $@` ]; then \
		mkdir -p `dirname $@`; \
	fi
	$(ECHO) --- Creating dependency file $(BOLD)$@$(NBOLD)...
	$(CCOMP) -M -MG -MT "$*.o $@" $(CFLAGS) -MF $@ $< > /dev/null


%.o:	%.c
	$(CC) -c -o $@ $<

%.o:	%.cpp
	$(CXX) -c -o $@ $<


# Rule to run the Qt Meta Object Compiler on Qt header files.
moc_%.cpp:	%.h
	if grep -q Q_OBJECT $<; then \
	  $(ECHO) --- Running the Meta Object Compiler on $(BOLD)$<$(NBOLD)...;\
	  $(QT_MOC) $< > $@ ;\
	else \
	  $(ECHO) "*** WARNING: Trying to run the Qt Meta Object Compiler on " \
	      	  "a non-qobject file "; \
	  $(ECHO) "             (Q_OBJECT declaration missing): $<"; \
	fi


# Rules to compile files generated by the Qt designer.
ui_%.h:	%.ui
	$(ECHO) "--- Generating "$(BOLD)$@$(NBOLD)" from "$(BOLD)$<$(NBOLD)"..."
	$(QT_UIC) -o $@ $<


#%.cpp:	%.ui
#	$(ECHO) "--- Generating "$(BOLD)$@$(NBOLD)" from "$(BOLD)$<$(NBOLD)"..."
#	$(QT_UIC) -o $@ -impl $(@:.cpp=.h) $<


# Rules to make programs
ifdef PROGS
$(PROGS):	$(notdir $(PROGS:%=%.o)) $(PRGOBJS) $(LIBDEP)
	if [ -n "$(BIN)" -a ! -d "$(BIN)" ]; then \
	  $(MKDIR) $(BIN) -p; \
	fi
	BASENAME=`basename $@`_version; \
	if [ -r $${BASENAME}.h -a -r $${BASENAME}.c ]; then \
		$(ECHO) --- Updating/compiling versionfile $(BOLD)$${BASENAME}.c$(NBOLD)...; \
		cp -f $${BASENAME}.c $${BASENAME}.c.bak; \
		perl -p -e 's/(_build)\s*=\s*([0-9]+)/$$1 . " = " .($$2+1)/e' $${BASENAME}.c \
	        	> new.$${BASENAME}.c; \
		mv new.$${BASENAME}.c $${BASENAME}.c; \
		$(COMP) -c $${BASENAME}.c; \
		$(LINK) -o $@ $(notdir $@).o $(PRGOBJS) $${BASENAME}.o $(PROJLIBS) $(LIBS); \
	else \
		$(LINK) -o $@ $(notdir $@).o $(PRGOBJS) $(PROJLIBS) $(LIBS); \
	fi
endif



ifdef CPPPROGS
$(CPPPROGS):	$(notdir $(CPPPROGS:%=%.o)) $(PRGOBJS) $(LIBDEP)
	if [ -n "$(BIN)" -a ! -d "$(BIN)" ]; then \
	  $(MKDIR) $(BIN) -p; \
	fi
	BASENAME=`basename $@`_version; \
	if [ -r $${BASENAME}.h -a -r $${BASENAME}.c ]; then \
		$(ECHO) --- Updating/compiling versionfile $(BOLD)$${BASENAME}.c$(NBOLD)...; \
		cp -f $${BASENAME}.c $${BASENAME}.c.bak; \
		perl -p -e 's/(_build)\s*=\s*([0-9]+)/$$1 . " = " .($$2+1)/e' $${BASENAME}.c \
	        	> new.$${BASENAME}.c; \
		mv new.$${BASENAME}.c $${BASENAME}.c; \
		$(COMP) -c $${BASENAME}.c; \
		$(CPPLINK) -o $@ $(notdir $@).o $(PRGOBJS) $@_version.o $(PROJLIBS) $(LIBS); \
	else \
		$(CPPLINK) -o $@ $(notdir $@).o $(PRGOBJS) $(PROJLIBS) $(LIBS); \
	fi
endif


# The "all" target. Used if no other target is specified
.PHONY: all release
all release:	$(PRE_TARGETS) $(LIBFILE) $(SHLIBFILE) $(PROGS) \
	$(CPPPROGS) $(PLUGINS:%=%.so) $(PLUGINS_CPP:%=%.so) $(POST_TARGETS)



# For debugging purposes...
.PHONY: debug
debug:
	echo ROOT             = $(ROOT)
	echo LIB              = $(LIB)
	echo INC              = $(INC)
	echo BIN              = $(BIN)
	echo DEP_FILES        = $(DEP_FILES)
	echo PROGS            = $(PROGS)
	echo PRGOBJS          = $(PRGOBJS)
	echo LIBDEP           = $(LIBDEP)
	echo LIBPATH          = ${LIBPATH}
	echo INCPATH          = ${INCPATH}
	echo CFLAGS           = ${CFLAGS}
	echo LDFLAGS          = ${LDFLAGS}
	echo LIBOBJS          = $(LIBOBJS)
	echo SRCS             = $(SRCS)
	echo QTHEADERS        = $(QTHEADERS)
	echo LIB_QTHEADERS    = $(LIB_QTHEADERS)
	echo PLUGINS          = $(PLUGINS)
	echo PLUGINSRC        = $(PLUGINSRC)
	echo EXPINC           = $(EXPINC)
	echo RPM_NAME         = $(RPM_NAME)
	echo PLUGINS_INSTALL  = $(PLUGINS_INSTALL)
	echo PLUGINOBJS       = $(PLUGINOBJS)
	echo SHLIB_VERSION    = $(SHLIB_VERSION)
	echo SHLIBFILE        = $(SHLIBFILE)
	echo LIB_INSTALL_DIR  = $(LIB_INSTALL_DIR)
	echo INC_INSTALL_DIR  = $(INC_INSTALL_DIR)
	echo BIN_INSTALL_DIR  = $(BIN_INSTALL_DIR)
	echo SBIN_INSTALL_DIR = $(SBIN_INSTALL_DIR)

	
# Cleanup directory from files generated by compiling, etc
.PHONY: clean
clean::
	$(RM) $(POST_TARGETS) $(LIBOBJS) $(PRGOBJS) $(LIBFILE) \
		$(notdir $(PROGS:%=%.o)) $(notdir $(PROGS:%=%_version.o)) \
		$(notdir $(CPPPROGS:%=%_version.o)) $(notdir $(CPPPROGS:%=%.o)) \
		$(PROGS) $(CPPPROGS) core $(SHLIBFILE) \
		$(LIB)/lib$(LIBNAME).so.$(SHLIB_MAJOR).$(SHLIB_MINOR) \
		$(LIB)/lib$(LIBNAME).so $(PLUGINOBJS) $(PRE_TARGETS) \
		$(PLUGINS:%=%.so) $(PLUGINS_CPP:%=%.so)
	QTFILES="$(QTHEADERS:%.h=moc_%.cpp) $(LIB_QTHEADERS:%.h=moc_%.cpp)"; \
	for file in $${QTFILES}; do \
	  if [ -f $$file ] && grep -qi "meta object code" $$file; then \
	    $(RM) $$file ;\
	  fi \
	done
	$(RM) $(UI_FILES:%.ui=ui_%.h) $(LIB_UI_FILES:%.ui=ui_%.h)


# Cleanup the dependency files and exported includefiles too.
.PHONY: realclean
realclean::	clean
	$(RM) $(DEP_FILES) $(EXPINC:%=$(INC)/%) *~ ./makefile.root \
		$(ARCHIVENAME)
	$(RM) -rf depend
	$(RM) -rf version
	$(RM) -f .config


# Create exported include files
.PHONY: expinc
expinc:	$(EXPINC:%=$(INC)/%)

$(EXPINC:%=$(INC)/%):
	$(ECHO) --- Creating $(BOLD)$@$(NBOLD)...
	if [ -n "$(INC)" -a ! -d "$(INC)" ]; then \
	  $(MKDIR) $(INC) -p; \
	fi
	$(RM) $@
	ln -s $$($(REVPATH) $$(dirname $@))/$$(basename $@) $@



# Create the library file
ifdef LIBFILE
$(LIBFILE):	$(LIBOBJS) $(STATIC_LIBS:%=$(LIB)/lib%.a)
	$(RM) $(LIBFILE)
	$(ECHO) --- Creating archive $(BOLD)$@$(NBOLD)...
	if [ -n "$(LIB)" -a ! -d "$(LIB)" ]; then \
	  $(MKDIR) $(LIB) -p; \
      	fi
	$(AR) cq $(LIBFILE) $(LIBOBJS)
	if [ -n "$(STATIC_LIBS)" ]; then \
	  mkdir __tmp__lib__; \
	  cd __tmp__lib__; \
	  STATIC_LIBS="$(STATIC_LIBS)"; \
	  for file in $${STATIC_LIBS}; do \
	    $(AR) x ../$(LIB)/lib$${file}.a; \
	  done; \
	  cd ..; \
	  $(AR) cq $(LIBFILE) __tmp__lib__/*.o; \
	  rm -rf __tmp__lib__; \
	fi
endif


# Create the shared library file and all version-links.
ifdef SHLIBFILE
$(SHLIBFILE):	$(LIBOBJS)
	$(RM) $(SHLIBFILE) \
	      	$(LIB)/lib$(LIBNAME).so.$(SHLIB_MAJOR).$(SHLIB_MINOR) \
		$(LIB)/lib$(LIBNAME).so
	#$(ECHO) --- Creating shared object library $(BOLD)$@$(NBOLD)...
	#ld -o $@ -G -h lib$(LIBNAME).so.$(SHLIB_MAJOR) $(LIBOBJS)
	#ld -o $@ -soname=lib$(LIBNAME).so.$(SHLIB_MAJOR).$(SHLIB_MINOR) \
	#      	-shared $(LIBPATH) $(LIBOBJS) $(LIB_LIBS)
	$(LINK) -shared -o $@ -Wl,-soname,lib$(LIBNAME).so.$(SHLIB_MAJOR).$(SHLIB_MINOR) $(LIBPATH) \
	      	$(LIBOBJS) $(LIB_LIBS)
	ln -s $(notdir $@) \
	      	$(LIB)/lib$(LIBNAME).so.$(SHLIB_MAJOR).$(SHLIB_MINOR)
	ln -s $(@:$(LIB)/%=%) $(LIB)/lib$(LIBNAME).so
endif


ARCHIVENAME	= `basename \`pwd\``.tgz


.PHONY: tgz
tgz:	realclean
	export current_dir_name=`basename \`pwd\``; \
	export archive_name="$${current_dir_name}.tgz"; \
	$(ECHO) --- Creating archive $(BOLD)$${archive_name}$(NBOLD)...; \
	cd ..; \
	tar czf $${current_dir_name}/$${archive_name} \
		--exclude $${archive_name} $${current_dir_name}; \
	cd $${current_dir_name};



# Install files
.PHONY: install install-extras
install:	all install-extras
	PROGS_INSTALL="$(PROGS_INSTALL)"; \
	for file in $${PROGS_INSTALL}; do \
	  $(INSTALL) $$file $(BIN_INSTALL_DIR); \
	done
	DAEMONS_INSTALL="$(DAEMONS_INSTALL)"; \
	for file in $${DAEMONS_INSTALL}; do \
	  $(INSTALL) $$file $(SBIN_INSTALL_DIR); \
	done
	INC_INSTALL="$(INC_INSTALL)"; \
	for file in $${INC_INSTALL}; do \
	  $(INSTALL) $$file $(INC_INSTALL_DIR); \
	done
	if [ -n "$(LIB_INSTALL)" ]; then \
	  file=`echo $(LIB_INSTALL) | cut -d'[' -f1`; \
	  perm="[`echo '$(LIB_INSTALL)[-,-,-]' | cut -d'[' -f2`"; \
	  $(INSTALL) $(LIB)/lib$${file}.a$$perm $(LIB_INSTALL_DIR); \
	  if [ -n "$(SHLIBFILE)" ]; then \
	    $(INSTALL) $(SHLIBFILE)$$perm $(LIB_INSTALL_DIR); \
	    $(INSTALL) $(LIB)/lib$${file}.so.$(SHLIB_MAJOR).$(SHLIB_MINOR)$$perm \
	      	    $(LIB_INSTALL_DIR); \
	    $(INSTALL) $(LIB)/lib$${file}.so$$perm $(LIB_INSTALL_DIR); \
	  fi \
	fi
	PLUGINS_INSTALL="$(PLUGINS_INSTALL)";\
	for file in $${PLUGINS_INSTALL}; do \
	  $(INSTALL) $$file $(PLUGIN_INSTALL_DIR); \
	done
	DOC_INSTALL="$(DOC_INSTALL)";\
	for file in $${DOC_INSTALL}; do \
	  $(ECHO) "*** ERROR: The DOC_INSTALL macro is obsoleted and should" \
	          "not be used anymore. Use the \"install-extras\" and" \
		  "\"rpm-extras\" targets instead."; \
	  exit 1; \
	done
	if [ -n "$(RC_SCRIPT)" ]; then \
	  $(ECHO) "*** ERROR: The RC_SCRIPT macro is obsoleted and should" \
	          "not be used anymore. Use the \"install-extras\" and" \
		  "\"rpm-extras\" targets instead."; \
	  exit 1; \
	fi


# Uninstall files
.PHONY: uninstall uninstall-extras
uninstall:	uninstall-extras
	$(UNINSTALL) $(BIN_INSTALL_DIR) $(PROGS_INSTALL)
	$(UNINSTALL) $(SBIN_INSTALL_DIR) $(DAEMONS_INSTALL)
	$(UNINSTALL) $(INC_INSTALL_DIR) $(INC_INSTALL)
	if [ -n "$(LIB_INSTALL)" ]; then \
	  file=`echo $(LIB_INSTALL) | cut -d'[' -f1`; \
	  $(UNINSTALL) $(LIB_INSTALL_DIR) lib$${file}.a; \
	  if [ -n "$(SHLIBFILE)" ]; then \
	    $(UNINSTALL) $(LIB_INSTALL_DIR) $(SHLIBFILE); \
	    $(UNINSTALL) $(LIB_INSTALL_DIR) \
	      	    lib$${file}.so.$(SHLIB_MAJOR).$(SHLIB_MINOR); \
	    $(UNINSTALL) $(LIB_INSTALL_DIR) lib$${file}.so$$perm; \
	  fi \
	fi
	$(UNINSTALL) $(PLUGIN_INSTALL_DIR) $(PLUGINS_INSTALL)
	if [ -n "$(DOC_INSTALL)" ]; then \
	  $(ECHO) "*** ERROR: The DOC_INSTALL macro is obsoleted and should" \
	          "not be used anymore. Use the \"install-extras\" and" \
		  "\"rpm-extras\" targets instead."; \
	  exit 1; \
	fi
	if [ -n "$(RC_SCRIPT)" ]; then \
	  $(ECHO) "*** ERROR: The RC_SCRIPT macro is obsoleted and should" \
	          "not be used anymore. Use the \"install-extras\" and" \
		  "\"rpm-extras\" targets instead."; \
	  exit 1; \
	fi


.PHONY: rpm rpm-base rpm-extras
rpm:  	rpm-base rpm-extras

rpm-base:
	if [ -n "$(RPM_NAME)" ]; then \
	  [ -d $(RPM_DIR)/BUILD ] || mkdir -p $(RPM_DIR)/BUILD; \
	  [ -d $(RPM_DIR)/RPMS ] || mkdir -p $(RPM_DIR)/RPMS; \
	  [ -d $(RPM_DIR)/SOURCES ] || mkdir -p $(RPM_DIR)/SOURCES; \
	  [ -d $(RPM_DIR)/SPECS ] || mkdir -p $(RPM_DIR)/SPECS; \
	  [ -d $(RPM_DIR)/SRPMS ] || mkdir -p $(RPM_DIR)/SRPMS; \
	  [ -d $(RPM_DIR)/TMP_SPECS ] || mkdir -p $(RPM_DIR)/TMP_SPECS; \
	  if [ ! -r "$(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec" ]; then \
	    if [ ! -r "$(RPM_DIR)/SPECS/$(RPM_NAME).tmpl" ]; then \
	      $(ECHO) "*** Could not find SPEC template:" \
	              "$(RPM_DIR)/SPECS/$(RPM_NAME).tmpl"; \
	      exit 1; \
	    fi; \
	    echo "BuildRoot: /tmp/rpm-tmp-install" \
	            > $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	    . $(ROOT)/versions; \
	    VERSION_VAR=$$(awk '/^Version:/ { print $$2 }' \
	      	    < $(RPM_DIR)/SPECS/$(RPM_NAME).tmpl); \
	    RELEASE_VAR=$$(awk '/^Release: *[A-Za-z]/ { print $$2 }' \
	      	    < $(RPM_DIR)/SPECS/$(RPM_NAME).tmpl); \
	    if [ -n "$${VERSION_VAR}" ]; then \
	      VERSION=$$(eval echo \$${$${VERSION_VAR}}); \
	    else \
	      $(ECHO) "*** WARNING: None or illegal version variable specified." \
	              "Version defaults to \"nover\"..."; \
	      VERSION="nover"; \
	    fi; \
	    if [ -z "$${VERSION}" ]; then \
	      VERSION="nover"; \
	    fi; \
	    if [ -n "$${RELEASE_VAR}" ]; then \
	      RELEASE=$$(eval echo \$${$${RELEASE_VAR}}); \
	    else \
	      $(ECHO) "*** WARNING: None or illegal release variable specified." \
	              "Release defaults to 1..."; \
	      RELEASE=1; \
	    fi; \
	    if [ -z "$${RELEASE}" ]; then \
	      $(ECHO) "*** WARNING: Release variable \"$${RELEASE_VAR}\" not defined." \
	              "Defaulting to 1..."; \
	      RELEASE=1; \
	    fi; \
	    sed "s/^\(Version:.*\)$${VERSION_VAR}/\1$${VERSION}/; \
	      	 s/^\(Release:.*\)$${RELEASE_VAR}/\1$${RELEASE}/" \
	      	    < $(RPM_DIR)/SPECS/$(RPM_NAME).tmpl \
	            >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	    echo "%files" >> "$(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec"; \
	  fi; \
	  make DESTDIR=/tmp/rpm-tmp-install NO_CHOWN=1 install; \
	  PROGS_INSTALL="$(PROGS_INSTALL)"; \
	  for file in $${PROGS_INSTALL}; do \
	    $(GEN_RPM_ROW) $$file $(BIN_INSTALL_DIR) \
	            >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	  done; \
	  DAEMONS_INSTALL="$(DAEMONS_INSTALL)"; \
	  for file in $${DAEMONS_INSTALL}; do \
	    $(GEN_RPM_ROW) $$file $(SBIN_INSTALL_DIR) \
	            >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	  done; \
	  if [ -n "$(LIB_INSTALL)" ]; then \
	    file=`echo $(LIB_INSTALL) | cut -d'[' -f1`; \
	    perm="[`echo '$(LIB_INSTALL)[-,-,-]' | cut -d'[' -f2`"; \
	    $(GEN_RPM_ROW) lib$${file}.a$$perm $(LIB_INSTALL_DIR) \
	            >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	    if [ -n "$(SHLIBFILE)" ]; then \
	      $(GEN_RPM_ROW) $(SHLIBFILE)$$perm $(LIB_INSTALL_DIR) \
		      >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	      $(GEN_RPM_ROW) lib$${file}.so.$(SHLIB_MAJOR).$(SHLIB_MINOR)$$perm \
	              $(LIB_INSTALL_DIR) \
		      >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	      $(GEN_RPM_ROW) lib$${file}.so$$perm $(LIB_INSTALL_DIR) \
	              >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	    fi \
	  fi; \
	  PLUGINS_INSTALL="$(PLUGINS_INSTALL)"; \
	  for file in $${PLUGINS_INSTALL}; do \
	    $(GEN_RPM_ROW) $$file $(PLUGIN_INSTALL_DIR) \
	          >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
	  done; \
	fi



#	  for file in $(INC_INSTALL); do \
#	    $(GEN_RPM_ROW) $$file $(INC_INSTALL_DIR) \
#	          >> $(RPM_DIR)/TMP_SPECS/$(RPM_NAME).spec; \
#	  done; \



#
# Targets for compiling plugins
#

ifneq ($(strip $(PLUGINSRC)),)
PLUGIN_ARCHIVE=plugin_lib.a
endif

plugin_lib.a:	$(PLUGINSRCOBJS)
	$(RM) $@
	$(AR) -cq $@ $(PLUGINSRCOBJS)

$(PLUGINS:%=%.so):	$(notdir $(PLUGINS:%=%.o)) $(PLUGIN_ARCHIVE)
	if [ -n "$(findstring $(notdir $(@:%.so=%)),$?)" \
	     -o -n "$(findstring $(PLUGIN_ARCHIVE),$?)" ]; then \
	  $(LINK) -shared -o $@ $(notdir $(@:%.so=%).o) $(PLUGIN_ARCHIVE) \
		$(PLUGIN_LIBS); \
	fi


$(PLUGINS_CPP:%=%.so):	$(notdir $(PLUGINS_CPP:%=%.o)) $(PLUGIN_ARCHIVE)
	if [ -n "$(findstring $(notdir $(@:%.so=%)),$?)" \
	     -o -n "$(findstring $(PLUGIN_ARCHIVE),$?)" ]; then \
	  $(LINK) -shared -o $@ $(notdir $(@:%.so=%).o) $(PLUGIN_ARCHIVE) \
		$(PLUGIN_LIBS); \
	fi


INTERMEDIATE_FILES  = $(UI_FILES:%.ui=ui_%.h) $(LIB_UI_FILES:%.ui=ui_%.h) \
		$(QTHEADERS:%.h=moc_%.cpp) $(LIB_QTHEADERS:%.h=moc_%.cpp)

ifneq ($(strip $(INTERMEDIATE_FILES)),)
.SECONDARY:	$(INTERMEDIATE_FILES)
endif


#
# Include dependencies
#
ifeq ($(INCLUDE_DEPS),yes)
  ifneq ($(strip $(DEP_FILES)),)
    include $(DEP_FILES)
  endif
endif

$(ROOT)/.config:	$(ROOT)/create_config.sh
	echo -e --- Creating $(BOLD)$@$(NBOLD)...
	$(ROOT)/create_config.sh $@

.config:
	if [ -x ./create_config ]; then \
	  echo -e --- Creating $(BOLD)$@$(NBOLD)...; \
	  rm -f $@; \
	  ./create_config $@; \
	  echo "MAKEFILE_LOCAL_CONFIG_INCLUDED = 1" >> $@; \
	fi

# Generate version files
version/%.h: $(ROOT)/versions
	if [ ! -d version ]; then \
	  $(MKDIR) version -p; \
	fi; \
	. $(ROOT)/versions; \
	if [ -n "$${$*}" ]; then \
	  VERSION=$${$*}; \
	else \
	  $(ECHO) "*** WARNING: Version entry missing for \"$*\""; \
	  VERSION="?.?.?"; \
	fi; \
	TARGET="version/$*.h"; \
	if ! grep -q \"$${VERSION}\" $${TARGET} > /dev/null 2>&1; then \
	  $(ECHO) --- Creating $(BOLD)$${TARGET}$(NBOLD)...; \
	  echo "#ifndef $*_VERSION" > $${TARGET}; \
	  echo "#define $*_VERSION \"$$VERSION\"" >> $${TARGET}; \
	  echo "#endif /* $*_VERSION */" >> $${TARGET}; \
	fi \


#
# This file has not been truncated
#
