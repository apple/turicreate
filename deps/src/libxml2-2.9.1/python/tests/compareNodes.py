#!/usr/bin/python -u
import sys
import libxml2

# Memory debug specific
libxml2.debugMemory(1)

#
# Testing XML Node comparison and Node hash-value
#
doc = libxml2.parseDoc("""<root><foo/></root>""")
root = doc.getRootElement()

# Create two different objects which point to foo
foonode1 = root.children
foonode2 = root.children

# Now check that [in]equality tests work ok
if not ( foonode1 == foonode2 ):
    print("Error comparing nodes with ==, nodes should be equal but are unequal")
    sys.exit(1)
if not ( foonode1 != root ):
    print("Error comparing nodes with ==, nodes should not be equal but are equal")
    sys.exit(1)
if not ( foonode1 != root ):
    print("Error comparing nodes with !=, nodes should not be equal but are equal")
if ( foonode1 != foonode2 ):
    print("Error comparing nodes with !=, nodes should be equal but are unequal")

# Next check that the hash function for the objects also works ok
if not (hash(foonode1) == hash(foonode2)):
    print("Error hash values for two equal nodes are different")
    sys.exit(1)
if not (hash(foonode1) != hash(root)):
    print("Error hash values for two unequal nodes are not different")
    sys.exit(1)
if hash(foonode1) == hash(root):
    print("Error hash values for two unequal nodes are equal")
    sys.exit(1)

# Basic tests successful
doc.freeDoc()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print("OK")
else:
    print("Memory leak %d bytes" % (libxml2.debugMemory(1)))
    libxml2.dumpMemory()
