#!/usr/bin/python -u
import sys
import libxml2
try:
    import StringIO
    str_io = StringIO.StringIO
except:
    import io
    str_io = io.StringIO

# Memory debug specific
libxml2.debugMemory(1)

def myResolver(URL, ID, ctxt):
    return(str_io("<foo/>"))

libxml2.setEntityLoader(myResolver)

doc = libxml2.parseFile("doesnotexist.xml")
root = doc.children
if root.name != "foo":
    print("root element name error")
    sys.exit(1)
doc.freeDoc()

i = 0
while i < 5000:
    doc = libxml2.parseFile("doesnotexist.xml")
    root = doc.children
    if root.name != "foo":
        print("root element name error")
        sys.exit(1)
    doc.freeDoc()
    i = i + 1


# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print("OK")
else:
    print("Memory leak %d bytes" % (libxml2.debugMemory(1)))
    libxml2.dumpMemory()
