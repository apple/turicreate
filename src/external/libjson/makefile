###############################################################################
#
#  Copyright 2010 Jonathan Wallace. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#      1. Redistributions of source code must retain the above copyright
#         notice, this list of conditions and the following disclaimer.
#
#      2. Redistributions in binary form must reproduce the above copyright
#         notice, this list of conditions and the following disclaimer in the
#         documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY JONATHAN WALLACE ``AS IS'' AND ANY 
#  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JONATHAN WALLACE OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
#  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
#  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
#  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
#  DAMAGE.
#
#  The views and conclusions contained in the software and documentation are
#  those of the authors and should not be interpreted as representing official
#  policies, either expressed or implied, of Jonathan Wallace....
#
#
#  Author Bernhard Fluehmann
#
#  bernhard.fluehmann@patton-inalp.com
#
#
#  TARGETS
#    all
#      Builds either a shared or a static library, depending of the value
#      of the SHARED variable.
#      Default: static
#
#    banner
#      Displays the library version and the target
#
#    install
#      Installs either the shared of the static library, depending of the value
#      of the SHARED variable. Installs the header files as well.
#      Builds the library before installing if it does not exist already.
#      Ececutes ldconfig in case of the shared library.
#      Default locations:
#        library: $(exec_prefix)/$(libdir) => /usr/local/lib
#        headers:   $(prefix)/$(includedir) => /usr/local/include
#
#    install_headers
#      Installs the header files.
#      Default location: $(prefix)/$(includedir) => /usr/local/include
#
#    clean
#      Removes the shared or the static library, depending of the value
#      of the SHARED variable, from this folder and all the folders and objects
#      which were created to build it.
#
#    uninstall
#      Removes the shared or the static library, depending of the value
#      of the SHARED variable from the system. In case of the shared library,
#      removes the symbolic links as well and executes ldconfig.
#
#    uninstall_headers
#      Remove the headers from the system.
#
#  Variables
#    prefix
#      The general installation root directory. Default: /usr/local
#
#    exec_prefix
#      The installation root directory to place executables and libraries.
#      Default: prefix
#
#    libdir
#      The directory where the libraries are installed, as a subdirectory of
#      exec_prefix. Default: lib
#
#    includedir
#      The directory where the header files are installed, as a subdirectory of
#      prefix. Default: include
#
#    srcdir
#      The directory where the object source files are located.
#      Default: Sources
#
#    CXX
#      The compiler to be used. Default: c++
#
#    CXXFLAGS
#      The compiler flags used to compile the object files.
#      Default:
#        cxxflags_default for default
#        cxxflags_small   for BUILD_TYPE=small
#        cxxflags_debug   for BUILD_TYPE=debug
#
#    AR
#      The archiver to be used. Default: ar
#
#    PIC
#      The PIC to be used. Default: PIC
#
#    BUILD_TYPE
#      Used to select a specific set of predefined configuaration parameters.
#      Default:  undefined: The default settings are used
#      Options:
#        small
#        debug
#
#    SHARED
#      Used to select if the library is a shared or a dynamic one
#      Default: undefined: static
#      Options:
#        0:         static
#        1:         shared
#
#
#

# JSON source files to build
objects        = internalJSONNode.o JSONAllocator.o JSONChildren.o \
                 JSONDebug.o JSONIterators.o JSONMemory.o JSONNode.o \
                 JSONNode_Mutex.o JSONPreparse.o JSONStream.o JSONValidator.o \
                 JSONWorker.o JSONWriter.o libjson.o 
OS=$(shell uname)

# Defaults
ifeq ($(OS), Darwin)
	cxxflags_default = -c -O3 -ffast-math -fexpensive-optimizations -DNDEBUG
else
	cxxflags_default = -c -O3 -ffast-math -fexpensive-optimizations -DNDEBUG
endif
cxxflags_small   = -c -Os -ffast-math -DJSON_LESS_MEMORY -DNDEBUG
cxxflags_debug   = -c -g -DJSON_SAFE -DJSON_DEBUG
cxxflags_shared  = -f$(PIC)
libname          = libjson
libname_hdr      := $(libname)
libname_debug    = $(libname)_dbg
suffix_shared    = so
suffix_static    = a
major_version    = 7
minor_version    = 6.0
objdir           = Objects


# Variables
prefix          ?= /usr/local
exec_prefix     ?= $(prefix)
libdir          ?= lib
includedir      ?= include
srcdir          ?= Source
CXX             ?= c++
AR              ?= ar
PIC             ?= PIC
BUILD_TYPE      ?= "default"
SHARED          ?= "1"


# Internal Variables
inst_path        = $(exec_prefix)/$(libdir)
include_path     = $(prefix)/$(includedir)


# Usage check
ifdef CXXFLAGS
ifdef BUILD_TYPE
	$(error CXXFLAGS and BUILD_TYPE are mutually exclusive)
endif
endif

# BUILD_TYPE specific settings
ifeq ($(BUILD_TYPE), small)
	CXXFLAGS     = $(cxxflags_small)
else ifeq ($(BUILD_TYPE), debug)
	CXXFLAGS     = $(cxxflags_debug)
	libname     := $(libname_debug)
else
	CXXFLAGS    ?= $(cxxflags_default)
endif

# SHARED specific settings
ifeq ($(SHARED), 1)
	libname_shared               = $(libname).$(suffix_shared)
	libname_shared_major_version = $(libname_shared).$(major_version)
	lib_target                   = $(libname_shared_major_version).$(minor_version)
	objdir                      := $(objdir)_shared
	CXXFLAGS                    := $(CXXFLAGS) $(cxxflags_shared)
else
	lib_target                   = $(libname).$(suffix_static)
	objdir                      := $(objdir)_static
endif

CXXFLAGS                    := $(CXXFLAGS) $(EXTRA_FLAGS)

# Phony targets
.PHONY: all banner installdirs install install_headers clean uninstall \
        uninstall_headers

# Targets
all: $(lib_target)
	@echo "============================================================"
	@echo "Done"
	@echo "============================================================"

banner:
	@echo "============================================================"
	@echo "libjson version: "$(major_version).$(minor_version) "target: "$(target) "OS: "$(OS)
	@echo "============================================================"

installdirs: banner
	mkdir -p $(objdir)

# Libraries
ifeq ($(SHARED),1)
$(lib_target): banner installdirs $(addprefix $(objdir)/, $(objects))
	@echo "Link "
	cd $(objdir) ; \
	if test "$(OS)" = "Darwin" ; then \
		$(CXX) -shared -Wl,-dylib_install_name -Wl,$(libname_shared_major_version) -o $@ $(objects) ; \
	else \
		$(CXX) -shared -Wl,-soname,$(libname_shared_major_version) -o $@ $(objects) ; \
	fi ; \
	mv -f $@ ../
	@echo "Link: Done"
else
$(lib_target): banner installdirs $(addprefix $(objdir)/, $(objects))
	@echo "Archive"
	cd $(objdir) ; \
	$(AR) -cvq $@ $(objects) ; \
	mv -f $@ ../
	@echo "Archive: Done"
endif

# Compile object files
$(objdir)/%.o: $(srcdir)/%.cpp installdirs
	$(CXX) $< -o $@ $(CXXFLAGS)

ifeq ($(SHARED),1)
install: banner install_headers $(lib_target)
	@echo "Install shared library"
	cp -f ./$(lib_target) $(inst_path)
	cd $(inst_path) ; \
	ln -sf $(lib_target) $(libname_shared_major_version) ; \
	ln -sf $(libname_shared_major_version) $(libname_shared)
	ifneq ($(OS),Darwin)
		ldconfig
	endif
	@echo "Install shared library: Done."
else
install: banner install_headers $(lib_target)
	@echo "Install static library"
	cp -f ./$(lib_target) $(inst_path)
	@echo "Install static library: Done."
endif

install_headers: banner
	@echo "Install header files"
	mkdir -p $(include_path)/$(libname_hdr)
	cp -f ./*.h $(include_path)/$(libname_hdr)
	mkdir -p $(include_path)/$(libname_hdr)/$(srcdir)
	cp -f ./$(srcdir)/*.h $(include_path)/$(libname_hdr)/$(srcdir)
	cp -r ./$(srcdir)/JSONDefs $(include_path)/$(libname_hdr)/$(srcdir)
	chmod -R a+r $(include_path)/$(libname_hdr)
	find  $(include_path)/$(libname_hdr) -type d -exec chmod a+x {} \;
	@echo "Install header files: Done."

clean: banner
	@echo "Clean library and object folder"
	rm -rf $(objdir)
	rm -f $(lib_target)
	@echo "Clean library and object folder: Done"

ifeq ($(SHARED),1)
uninstall: banner uninstall_headers
	@echo "Uninstall shared library"
	rm -f $(inst_path)/$(libname_shared)
	rm -f $(inst_path)/$(libname_shared_major_version)
	rm -f $(inst_path)/$(lib_target)
	ldconfig
	@echo "Uninstall shared library: Done"
else
uninstall: banner uninstall_headers
	@echo "Uninstall static library"
	rm -f $(inst_path)/$(lib_target)
	@echo "Uninstall static library: Done"
endif

uninstall_headers: banner
	@echo "Uninstall header files"
	rm -rf $(include_path)/$(libname)
	@echo "Uninstall header files: Done"
