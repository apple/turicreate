noinst_PROGRAMS	= gjobread

AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(srcdir)/include
AM_CFLAGS = $(THREAD_CFLAGS) $(Z_CFLAGS)
LDADD = $(RDL_LIBS) $(STATIC_BINARIES) $(top_builddir)/libxml2.la $(THREAD_LIBS) $(Z_LIBS) $(ICONV_LIBS) -lm $(WIN32_EXTRA_LIBADD)

gjobread_SOURCES=gjobread.c
