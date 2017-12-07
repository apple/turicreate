#!/usr/bin/python -u
#
# This tests custom input callbacks
#
import sys
import libxml2
try:
    import StringIO
    str_io = StringIO.StringIO
except:
    import io
    str_io = io.StringIO

# We implement a new scheme, py://strings/ that will reference this dictionary
pystrings = {
    'catalogs/catalog.xml' :
'''<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE catalog PUBLIC "-//OASIS//DTD Entity Resolution XML Catalog V1.0//EN" "http://www.oasis-open.org/committees/entity/release/1.0/catalog.dtd">
<catalog xmlns="urn:oasis:names:tc:entity:xmlns:xml:catalog">
  <rewriteSystem systemIdStartString="http://example.com/dtds/" rewritePrefix="../dtds/"/>
</catalog>''',

    'xml/sample.xml' :
'''<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE root SYSTEM "http://example.com/dtds/sample.dtd">
<root>&sample.entity;</root>''',

    'dtds/sample.dtd' :
'''
<!ELEMENT root (#PCDATA)>
<!ENTITY sample.entity "replacement text">'''
}

prefix = "py://strings/"
startURL = prefix + "xml/sample.xml"
catURL = prefix + "catalogs/catalog.xml"

def my_input_cb(URI):
    if not(URI.startswith(prefix)):
        return None
    path = URI[len(prefix):]
    if path not in pystrings:
        return None
    return str_io(pystrings[path])


def run_test(desc, docpath, catalog, exp_status="verified", exp_err=[], test_callback=None,
        root_name="root", root_content="replacement text"):
    opts = libxml2.XML_PARSE_DTDLOAD | libxml2.XML_PARSE_NONET | libxml2.XML_PARSE_COMPACT
    actual_err = []

    def my_global_error_cb(ctx, msg):
        actual_err.append((-1, msg))
    def my_ctx_error_cb(arg, msg, severity, reserved):
        actual_err.append((severity, msg))

    libxml2.registerErrorHandler(my_global_error_cb, None)
    try:
        parser = libxml2.createURLParserCtxt(docpath, opts)
        parser.setErrorHandler(my_ctx_error_cb, None)
        if catalog is not None:
            parser.addLocalCatalog(catalog)
        if test_callback is not None:
            test_callback()
        parser.parseDocument()
        doc = parser.doc()
        actual_status = "loaded"
        e = doc.getRootElement()
        if e.name == root_name and e.content == root_content:
            actual_status = "verified"
        doc.freeDoc()
    except libxml2.parserError:
        actual_status = "not loaded"

    if actual_status != exp_status:
        print("Test '%s' failed: expect status '%s', actual '%s'" % (desc, exp_status, actual_status))
        sys.exit(1)
    elif actual_err != exp_err:
        print("Test '%s' failed" % desc)
        print("Expect errors:")
        for s,m in exp_err: print("  [%2d] '%s'" % (s,m))
        print("Actual errors:")
        for s,m in actual_err: print("  [%2d] '%s'" % (s,m))
        sys.exit(1)


# Check that we cannot read custom schema without custom callback
run_test(desc="Loading entity without custom callback",
        docpath=startURL, catalog=None,
        exp_status="not loaded", exp_err=[
            (-1, "I/O "),
            (-1, "warning : "),
            (-1, "failed to load external entity \"py://strings/xml/sample.xml\"\n")
            ])

# Register handler and try to load the same entity
libxml2.registerInputCallback(my_input_cb)
run_test(desc="Loading entity with custom callback",
        docpath=startURL, catalog=None,
        exp_status="loaded", exp_err=[
            (-1, "Attempt to load network entity http://example.com/dtds/sample.dtd"),
            ( 4, "Entity 'sample.entity' not defined\n")
            ])

# Register a catalog (also accessible via pystr://) and retry
run_test(desc="Loading entity with custom callback and catalog",
        docpath=startURL, catalog=catURL)

# Unregister custom callback when parser is already created
run_test(desc="Loading entity and unregistering callback",
        docpath=startURL, catalog=catURL,
        test_callback=lambda: libxml2.popInputCallbacks(),
        exp_status="loaded", exp_err=[
            ( 3, "failed to load external entity \"py://strings/dtds/sample.dtd\"\n"),
            ( 4, "Entity 'sample.entity' not defined\n")
            ])

# Try to load the document again
run_test(desc="Retry loading document after unregistering callback",
        docpath=startURL, catalog=catURL,
        exp_status="not loaded", exp_err=[
            (-1, "I/O "),
            (-1, "warning : "),
            (-1, "failed to load external entity \"py://strings/xml/sample.xml\"\n")
            ])

# But should be able to read standard I/O yet...
run_test(desc="Loading using standard i/o after unregistering callback",
        docpath="tst.xml", catalog=None,
        root_name='doc', root_content='bar')

# Now pop ALL input callbacks, should fail to load even standard I/O
try:
    while True:
        libxml2.popInputCallbacks()
except IndexError:
    pass

run_test(desc="Loading using standard i/o after unregistering all callbacks",
        docpath="tst.xml", catalog=None,
        exp_status="not loaded", exp_err=[
            (-1, "I/O "),
            (-1, "warning : "),
            (-1, "failed to load external entity \"tst.xml\"\n")
            ])

print("OK")
sys.exit(0);
