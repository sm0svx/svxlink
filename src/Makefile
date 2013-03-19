#------------------------------------------------------------------------------
# Filename:             Makefile
# Author:               Tobias Blomberg
# Created:              1997-12-11
# Description:          This is the top-level makefile. Its
#			only purpose is to execute make in the
#			subdirectories in the correct order.
#			
# Targets:		all	  -- Make all
#			clean     -- Cleanup compiler generated files.
#			realclean -- Cleanup most generated files.
#			distclean -- Cleanup all generated files.
#			linecount -- Count the lines of source code.
#			tgz       -- Cleanup and create an archive. The
#				     archive is named 'current subdir'.tgz.
#			makefiles -- Create Makefiles in subdirectories.
#			install   -- Install project files.
#			uninstall -- Uninstall project files.
#     	      	      	rpm   	  -- Create RPMs
#     	      	      	rpmclean  -- Clean up RPMS build area.
#
# Executes:		echo	  -- Echo text to the screen.
#			find	  -- Find files.
#			awk	  -- Text processing tool.
#			tar	  -- Archiver tool.
#			bash	  -- Born Again Shell.
#			make	  -- Make.
#			rm	  -- Remove files.
#			ln	  -- Create softlinks in the filesystem.
#     	      	      	basename  -- Extract the filename part from a path
#     	      	      	rpm   	  -- Create RPM packages
#			perl      -- Text processing
#
#------------------------------------------------------------------------------


#
# Comment this out if you want to see what exactly is done by the
# make files. Used for debugging.
#
.SILENT:



###############################################################################
#
# Nothing to configure here...
#
###############################################################################

ifeq ($(OS),)
  -include ./makefile.root
  ifneq ($(ROOT),)
    -include $(ROOT)/makefile.cfg
    -include $(ROOT)/.config
  endif
endif

include makefile.dirs


# Reverse the order of the supplied directory list
REVDIRS 	= $$(echo $(SUBDIRS) | \
			awk '{ for(i=NF; i>0; i--) { printf $$i " "; } }')

# Name of the archive created with "make tgz"
ARCHIVENAME	= $$(basename $$(pwd)).tgz

# Used to get bold text in echo statements
BOLD		= "\033[1m"

# End bold text
NBOLD		= "\033[0m"

# Create softlink
MKSOFT		= ln -s

# Print directory
PRINT_DIR     	= \
      	      	print_dir() { \
		  $(ECHO) $(BOLD)-------------- $$1 --------------$(NBOLD); \
		}; print_dir

# Remove directory (must be empty)
RMDIR		= remove_dir() { \
		    $(ECHO) --- Removing directory $(BOLD)$$*$(NBOLD)...; \
		    rmdir $$*; \
		  }; remove_dir

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


#
# all	    - Build all targets
# install   - Install applications, header files, libraries, plugins etc
# uninstall - Uninstall what the install target has installed
#
.PHONY:	all install uninstall release
all install uninstall release:	makefiles
ifeq ($(OS),)
	SUBDIRS="$(SUBDIRS)"; \
	for dir in $${SUBDIRS}; do \
	    if [ -d $$dir ]; then \
		$(PRINT_DIR) $(CURRENT_SUBDIR)$$dir ; \
		$(MAKE) CURRENT_SUBDIR="$(CURRENT_SUBDIR)$$dir/" -C $$dir $@; \
		ec=$$?; \
		if [ $$ec -ne 0 ]; then \
		  exit $$ec; \
		fi \
	    fi \
	done
else
	$(MAKE) -f Makefile.win32 vc
endif


#
# linecount - Count how many lines of .c .h and .cpp files.
#
.PHONY:	linecount
linecount:
ifeq ($(OS),)
	$(ECHO) -n $(BOLD)"Lines total sourcecode: "
	find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" \) \
		-exec wc {} \; | awk '{ sum += $$1 } END { printf sum }'
	$(ECHO) $(NBOLD)
endif


#
# clean - Remove files created during compilation (excluding dependency
#         information and cache files used in the makefile structure).
#
.PHONY:	clean
clean:	makefiles
ifeq ($(OS),)
	for dir in $(REVDIRS); do \
	    if [ -d $$dir ]; then \
		$(PRINT_DIR) $(CURRENT_SUBDIR)$$dir ; \
		$(MAKE) CURRENT_SUBDIR="$(CURRENT_SUBDIR)$$dir/" -C $$dir $@; \
	    fi \
	done
else
	$(MAKE) -f Makefile.win32 $@
endif


#
# rpmclean - Remove RPM related directories and files
#
.PHONY:	rpmclean
rpmclean:
ifeq ($(OS),)
	if [ -f makefile.inc ]; then \
	  if [ -n "$(RPM_DIR)" ]; then \
	    $(ECHO) --- Removing RPM related directories...; \
	    $(RM) -rf $(RPM_DIR)/RPMS $(RPM_DIR)/TMP_SPECS $(RPM_DIR)/BUILD \
	      	      $(RPM_DIR)/SRPMS $(RPM_DIR)/SOURCES; \
	  fi \
	fi
endif
	

#
# realclean - Remove everything created by the makefiles except the
#             softlinks.
#
.PHONY:	realclean
realclean:	rpmclean makefiles
ifeq ($(OS),)
	for dir in $(REVDIRS); do \
	    if [ -d $$dir ]; then \
		$(PRINT_DIR) $(CURRENT_SUBDIR)$$dir ; \
		$(MAKE) CURRENT_SUBDIR="$(CURRENT_SUBDIR)$$dir/" -C $$dir $@; \
	    fi \
	done
	$(RM) *~ $(ARCHIVENAME) .config
	if [ ! -r makefile.inc ]; then \
	  $(RM) makefile.root; \
	fi
else
	$(MAKE) -f Makefile.win32 $@
endif


#
# distclean - Remove everything created by the makefiles including
#             softlinks so that the source tree looks exactly like
#             when it was first unpacked.
#
.PHONY:	distclean
distclean:	realclean
ifeq ($(OS),)
	for file in $$(find . -type l -name Makefile); do \
	  $(RM) $$file; \
	done
	if [ -r makefile.inc ]; then \
	  if [ -d $(BIN) ] && [ -z "$$(ls -A $(BIN))" ]; then \
	    $(RMDIR) $(BIN); \
	  fi; \
	  if [ -d $(LIB) ] && [ -z "$$(ls -A $(LIB))" ]; then \
	    $(RMDIR) $(LIB); \
	  fi; \
	  if [ -d $(INC) ] && [ -z "$$(ls -A $(INC))" ]; then \
	    $(RMDIR) $(INC); \
	  fi \
	fi
else
	$(MAKE) -f Makefile.win32 $@
endif


#
# tgz - Create a tar-archive of the subdirectory
#
.PHONY:	tgz
tgz:	makefiles realclean
ifeq ($(OS),)
	export current_dir_name=$$(basename $$(pwd)); \
	export archive_name="$${current_dir_name}.tgz"; \
	$(ECHO) --- Creating archive $(BOLD)$${archive_name}$(NBOLD)...; \
	cd ..; \
	tar czf $${current_dir_name}/$${archive_name} \
		--exclude $${archive_name} $${current_dir_name}; \
	cd $${current_dir_name}
endif


#
# makefiles - Create the makefile structure (softlinks).
#
.PHONY:	makefiles
makefiles:
ifeq ($(OS),)
	if [ "$(ROOT)" = "." ]; then \
	  ROOT=..; \
	else \
	  ROOT=$(ROOT)/..; \
	fi; \
	SUBDIRS="$(SUBDIRS)"; \
	for dir in $${SUBDIRS}; do \
	  if [ -d $$dir -a ! -r $$dir/Makefile ]; then \
	    if [ -r $$dir/makefile.dirs ]; then \
	      ( \
	        cd $$dir; $(MKSOFT) $${ROOT}/Makefile; \
		$(MAKE) makefiles; \
	      ); \
	    else \
		( cd $$dir; $(MKSOFT) $${ROOT}/makefile.sub Makefile ); \
	    fi \
	  fi \
	done
endif


#
# rpm - Create RPMs
#
.PHONY: rpm
rpm:
ifeq ($(OS),)
	if [ -z "$(MADE_FROM_TOP)" -a ! -f makefile.inc ]; then \
	  $(ECHO) "*** This target must be made from the project top source" \
	  	  "directory!"; \
	  exit 1; \
	fi
	if [ -z "$(RPM_DIR)" ]; then \
	  $(ECHO) "*** Variable RPM_DIR not set in makefile.cfg!"; \
	  exit 1; \
	fi
	if [ ! -r $(ROOT)/versions ]; then \
	  $(ECHO) "*** ERROR: Version file missing: \"$(ROOT)/versions\"." \
	      	  "Aborting..."; \
	  exit 1; \
	fi
	if [ -f makefile.inc ]; then \
	  rm -f $(RPM_DIR)/TMP_SPECS/*; \
	fi
	SUBDIRS="$(SUBDIRS)"; \
	for dir in $${SUBDIRS}; do \
	    if [ -d $$dir ]; then \
		$(PRINT_DIR) $(CURRENT_SUBDIR)$$dir ; \
		$(MAKE) CURRENT_SUBDIR="$(CURRENT_SUBDIR)$$dir/" \
		     MADE_FROM_TOP=1 -C $$dir $@; \
	    fi \
	done
	if [ -f makefile.inc ]; then \
	  export HOME=$(RPM_DIR); \
	  echo %_topdir $(RPM_DIR) > $(RPM_DIR)/.rpmmacros; \
	  echo %_unpackaged_files_terminate_build 0 >> $(RPM_DIR)/.rpmmacros; \
	  rpmbuild -bb $(RPM_DIR)/TMP_SPECS/*; \
	  rm -rf /tmp/rpm-tmp-install; \
	fi
endif


#
# Create a cache file containing the relative path to the project root
#
makefile.root:
	( \
	  export WRITETO=$$(pwd); \
	  if [ -r makefile.inc ]; then \
	    ROOT="."; \
	  else \
	    ROOT=".."; \
	    cd ..; \
	    while [ ! -r makefile.inc ]; do \
	      cd ..; \
	      ROOT="$${ROOT}/.."; \
	    done; \
	  fi; \
	  echo "ROOT=$${ROOT}" > $${WRITETO}/$@; \
	  echo "export ROOT" >> $${WRITETO}/$@; \
	)


#
# Create the .config file if it does not exist
#
$(ROOT)/.config:	$(ROOT)/create_config.sh
	/bin/echo --- Creating $@...
	$(ROOT)/create_config.sh $@



#
# This file has not been truncated
#
