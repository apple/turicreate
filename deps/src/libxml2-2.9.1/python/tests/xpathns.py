#!/usr/bin/python -u
#
import libxml2

expect=' xmlns:a="urn:whatevar"'

# Memory debug specific
libxml2.debugMemory(1)

d = libxml2.parseDoc("<a:a xmlns:a='urn:whatevar'/>")
res=""
for n in d.xpathEval("//namespace::*"):
    res = res + n.serialize()
d.freeDoc()

if res != expect:
    print("test5 failed: unexpected output")
    print(res)
del res
del d
del n
# Memory debug specific
libxml2.cleanupParser()

if libxml2.debugMemory(1) == 0:
    print("OK")
else:
    print("Memory leak %d bytes" % (libxml2.debugMemory(1)))
    libxml2.dumpMemory()
