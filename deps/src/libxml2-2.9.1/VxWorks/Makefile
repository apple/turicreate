##----------------------------------------------------------------
##
##-- Filename: Makefile
##
##-- $Date: 2008/02/23 02:56:17 $
##-- $Revision: 1.2 $
##-- $Name:  $
##
##----------------------------------------------------------------

NAME = xml2
TOOL_FAMILY=gnu

include $(WIND_USR)/tool/gnu/make.$(VXCPU)

FLAGS = -Wall -g -Isrc -Isrc/include -D_REENTRANT=1
#FLAGS = -Wall -O2 -Isrc -Isrc/include -D_REENTRANT=1

FLAGS += $(DEFINE_CC) $(CC_ARCH_SPEC) -MD -MP -D_VX_CPU=_VX_$(CPU) -D_VX_TOOL_FAMILY=gnu -D_VX_TOOL=$(TOOL)
ifeq ($(VXTYPE),RTP)
FLAGS += -mrtp -fpic -I$(WIND_USR)/h -I$(WIND_USR)/h/wrn/coreip
else
FLAGS += -D_WRS_KERNEL -I$(WIND_BASE)/target/h -I$(WIND_BASE)/target/h/wrn/coreip
endif

ifeq ($(VXTYPE),RTP)
ifeq ($(CPU),SH32)
LIB_LDFLAGS += -L$(WIND_USR)/lib/sh/SH32/commonle/PIC
else
LIB_LDFLAGS += $(LD_LINK_PATH_ATEND) $(LD_PARTIAL_LAST_FLAGS)
endif
endif

OBJS =	c14n.o catalog.o chvalid.o \
		debugXML.o dict.o DOCBparser.o \
		encoding.o entities.o error.o \
		globals.o \
		hash.o \
		legacy.o list.o \
		parser.o parserInternals.o pattern.o \
		relaxng.o \
		SAX2.o SAX.o schematron.o \
		threads.o tree.o \
		uri.o \
		valid.o \
		xinclude.o xlink.o xmlcatalog.o xmlIO.o \
		xmlmemory.o xmlmodule.o xmlreader.o xmlregexp.o \
		xmlsave.o xmlschemas.o xmlschemastypes.o xmlstring.o \
		xmlunicode.o xmlwriter.o xpath.o xpointer.o

all : lib$(NAME).so

init :
	mkdir -p objs

.PHONY : lib$(NAME).so

lib$(NAME).so : init $(patsubst %.o, objs/%.o, $(OBJS))
	$(CC) $(FLAGS) $(LIB_LDFLAGS) -shared -o $@ $(patsubst %.o, objs/%.o, $(OBJS))

(NAME).out : init $(patsubst %.o, objs/%.o, $(OBJS))
	$(CC) $(FLAGS) -o $@ $(patsubst %.o, objs/%.o, $(OBJS))

objs/%.o: src/%.c
	 $(CC) $(FLAGS) -o $@ -c $<

clean:
	rm -fR *.so objs
