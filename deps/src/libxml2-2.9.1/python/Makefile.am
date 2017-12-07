# Makefile for libxml2 python library
AUTOMAKE_OPTIONS = 1.4 foreign

SUBDIRS = . tests

docsdir = $(datadir)/doc/libxml2-python-$(LIBXML_VERSION)
dist_docs_DATA = TODO

EXTRA_DIST = 			\
	setup.py		\
	generator.py		\
	libxml.py		\
	libxml2-export.c	\
	libxml2-python-api.xml	\
	libxml2class.py		\
	libxml2class.txt

if WITH_PYTHON
AM_CPPFLAGS = \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/include	\
	-I$(PYTHON_INCLUDES)

python_LTLIBRARIES = libxml2mod.la

libxml2mod_la_SOURCES = libxml.c libxml_wrap.h libxml2-py.h libxml2-py.c types.c
libxml2mod_la_LDFLAGS = $(CYGWIN_EXTRA_LDFLAGS) $(WIN32_EXTRA_LDFLAGS) -module -avoid-version \
        $(top_builddir)/libxml2.la $(CYGWIN_EXTRA_PYTHON_LIBADD) $(WIN32_EXTRA_PYTHON_LIBADD) $(PYTHON_LIBS)

BUILT_SOURCES = libxml2-export.c libxml2-py.h libxml2-py.c

# libxml.c #includes libxml2-export.c
libxml.$(OBJEXT): libxml2-export.c

libxml2.py: $(srcdir)/libxml.py libxml2class.py
	cat $(srcdir)/libxml.py `test -f libxml2class.py || echo $(srcdir)/`libxml2class.py > $@

dist_python_DATA = \
	drv_libxml2.py \
	libxml2.py

CLEANFILES = *.pyc

MAINTAINERCLEANFILES = libxml2.py libxml2class.*

API_DESC = $(top_srcdir)/doc/libxml2-api.xml $(srcdir)/libxml2-python-api.xml
GENERATED = libxml2class.py libxml2class.txt $(BUILT_SOURCES)

$(GENERATED): $(srcdir)/generator.py $(API_DESC)
	$(PYTHON) $(srcdir)/generator.py $(srcdir)
endif

tests test: all
	cd tests && $(MAKE) tests
