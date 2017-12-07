import libxml2mod
import types
import sys

# The root of all libxml2 errors.
class libxmlError(Exception): pass

# Type of the wrapper class for the C objects wrappers
def checkWrapper(obj):
    try:
        n = type(_obj).__name__
        if n != 'PyCObject' and n != 'PyCapsule':
            return 1
    except:
        return 0
    return 0

#
# id() is sometimes negative ...
#
def pos_id(o):
    i = id(o)
    if (i < 0):
        return (sys.maxsize - i)
    return i

#
# Errors raised by the wrappers when some tree handling failed.
#
class treeError(libxmlError):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class parserError(libxmlError):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class uriError(libxmlError):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class xpathError(libxmlError):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

class ioWrapper:
    def __init__(self, _obj):
        self.__io = _obj
        self._o = None

    def io_close(self):
        if self.__io == None:
            return(-1)
        self.__io.close()
        self.__io = None
        return(0)

    def io_flush(self):
        if self.__io == None:
            return(-1)
        self.__io.flush()
        return(0)

    def io_read(self, len = -1):
        if self.__io == None:
            return(-1)
        try:
            if len < 0:
                ret = self.__io.read()
            else:
                ret = self.__io.read(len)
        except Exception:
            import sys
            e = sys.exc_info()[1]
            print("failed to read from Python:", type(e))
            print("on IO:", self.__io)
            self.__io == None
            return(-1)

        return(ret)

    def io_write(self, str, len = -1):
        if self.__io == None:
            return(-1)
        if len < 0:
            return(self.__io.write(str))
        return(self.__io.write(str, len))

class ioReadWrapper(ioWrapper):
    def __init__(self, _obj, enc = ""):
        ioWrapper.__init__(self, _obj)
        self._o = libxml2mod.xmlCreateInputBuffer(self, enc)

    def __del__(self):
        print("__del__")
        self.io_close()
        if self._o != None:
            libxml2mod.xmlFreeParserInputBuffer(self._o)
        self._o = None

    def close(self):
        self.io_close()
        if self._o != None:
            libxml2mod.xmlFreeParserInputBuffer(self._o)
        self._o = None

class ioWriteWrapper(ioWrapper):
    def __init__(self, _obj, enc = ""):
#        print "ioWriteWrapper.__init__", _obj
        if type(_obj) == type(''):
            print("write io from a string")
            self.o = None
        elif type(_obj).__name__ == 'PyCapsule':
            file = libxml2mod.outputBufferGetPythonFile(_obj)
            if file != None:
                ioWrapper.__init__(self, file)
            else:
                ioWrapper.__init__(self, _obj)
            self._o = _obj
#        elif type(_obj) == types.InstanceType:
#            print(("write io from instance of %s" % (_obj.__class__)))
#            ioWrapper.__init__(self, _obj)
#            self._o = libxml2mod.xmlCreateOutputBuffer(self, enc)
        else:
            file = libxml2mod.outputBufferGetPythonFile(_obj)
            if file != None:
                ioWrapper.__init__(self, file)
            else:
                ioWrapper.__init__(self, _obj)
            self._o = _obj

    def __del__(self):
#        print "__del__"
        self.io_close()
        if self._o != None:
            libxml2mod.xmlOutputBufferClose(self._o)
        self._o = None

    def flush(self):
        self.io_flush()
        if self._o != None:
            libxml2mod.xmlOutputBufferClose(self._o)
        self._o = None

    def close(self):
        self.io_flush()
        if self._o != None:
            libxml2mod.xmlOutputBufferClose(self._o)
        self._o = None

#
# Example of a class to handle SAX events
#
class SAXCallback:
    """Base class for SAX handlers"""
    def startDocument(self):
        """called at the start of the document"""
        pass

    def endDocument(self):
        """called at the end of the document"""
        pass

    def startElement(self, tag, attrs):
        """called at the start of every element, tag is the name of
           the element, attrs is a dictionary of the element's attributes"""
        pass

    def endElement(self, tag):
        """called at the start of every element, tag is the name of
           the element"""
        pass

    def characters(self, data):
        """called when character data have been read, data is the string
           containing the data, multiple consecutive characters() callback
           are possible."""
        pass

    def cdataBlock(self, data):
        """called when CDATA section have been read, data is the string
           containing the data, multiple consecutive cdataBlock() callback
           are possible."""
        pass

    def reference(self, name):
        """called when an entity reference has been found"""
        pass

    def ignorableWhitespace(self, data):
        """called when potentially ignorable white spaces have been found"""
        pass

    def processingInstruction(self, target, data):
        """called when a PI has been found, target contains the PI name and
           data is the associated data in the PI"""
        pass

    def comment(self, content):
        """called when a comment has been found, content contains the comment"""
        pass

    def externalSubset(self, name, externalID, systemID):
        """called when a DOCTYPE declaration has been found, name is the
           DTD name and externalID, systemID are the DTD public and system
           identifier for that DTd if available"""
        pass

    def internalSubset(self, name, externalID, systemID):
        """called when a DOCTYPE declaration has been found, name is the
           DTD name and externalID, systemID are the DTD public and system
           identifier for that DTD if available"""
        pass

    def entityDecl(self, name, type, externalID, systemID, content):
        """called when an ENTITY declaration has been found, name is the
           entity name and externalID, systemID are the entity public and
           system identifier for that entity if available, type indicates
           the entity type, and content reports it's string content"""
        pass

    def notationDecl(self, name, externalID, systemID):
        """called when an NOTATION declaration has been found, name is the
           notation name and externalID, systemID are the notation public and
           system identifier for that notation if available"""
        pass

    def attributeDecl(self, elem, name, type, defi, defaultValue, nameList):
        """called when an ATTRIBUTE definition has been found"""
        pass

    def elementDecl(self, name, type, content):
        """called when an ELEMENT definition has been found"""
        pass

    def entityDecl(self, name, publicId, systemID, notationName):
        """called when an unparsed ENTITY declaration has been found,
           name is the entity name and publicId,, systemID are the entity
           public and system identifier for that entity if available,
           and notationName indicate the associated NOTATION"""
        pass

    def warning(self, msg):
        #print msg
        pass

    def error(self, msg):
        raise parserError(msg)

    def fatalError(self, msg):
        raise parserError(msg)

#
# This class is the ancestor of all the Node classes. It provides
# the basic functionalities shared by all nodes (and handle
# gracefylly the exception), like name, navigation in the tree,
# doc reference, content access and serializing to a string or URI
#
class xmlCore:
    def __init__(self, _obj=None):
        if _obj != None:
            self._o = _obj;
            return
        self._o = None

    def __eq__(self, other):
        if other == None:
            return False
        ret = libxml2mod.compareNodesEqual(self._o, other._o)
        if ret == None:
            return False
        return ret == True
    def __ne__(self, other):
        if other == None:
            return True
        ret = libxml2mod.compareNodesEqual(self._o, other._o)
        return not ret
    def __hash__(self):
        ret = libxml2mod.nodeHash(self._o)
        return ret

    def __str__(self):
        return self.serialize()
    def get_parent(self):
        ret = libxml2mod.parent(self._o)
        if ret == None:
            return None
        return nodeWrap(ret)
    def get_children(self):
        ret = libxml2mod.children(self._o)
        if ret == None:
            return None
        return nodeWrap(ret)
    def get_last(self):
        ret = libxml2mod.last(self._o)
        if ret == None:
            return None
        return nodeWrap(ret)
    def get_next(self):
        ret = libxml2mod.next(self._o)
        if ret == None:
            return None
        return nodeWrap(ret)
    def get_properties(self):
        ret = libxml2mod.properties(self._o)
        if ret == None:
            return None
        return xmlAttr(_obj=ret)
    def get_prev(self):
        ret = libxml2mod.prev(self._o)
        if ret == None:
            return None
        return nodeWrap(ret)
    def get_content(self):
        return libxml2mod.xmlNodeGetContent(self._o)
    getContent = get_content  # why is this duplicate naming needed ?
    def get_name(self):
        return libxml2mod.name(self._o)
    def get_type(self):
        return libxml2mod.type(self._o)
    def get_doc(self):
        ret = libxml2mod.doc(self._o)
        if ret == None:
            if self.type in ["document_xml", "document_html"]:
                return xmlDoc(_obj=self._o)
            else:
                return None
        return xmlDoc(_obj=ret)
    #
    # Those are common attributes to nearly all type of nodes
    # defined as python2 properties
    #
    import sys
    if float(sys.version[0:3]) < 2.2:
        def __getattr__(self, attr):
            if attr == "parent":
                ret = libxml2mod.parent(self._o)
                if ret == None:
                    return None
                return nodeWrap(ret)
            elif attr == "properties":
                ret = libxml2mod.properties(self._o)
                if ret == None:
                    return None
                return xmlAttr(_obj=ret)
            elif attr == "children":
                ret = libxml2mod.children(self._o)
                if ret == None:
                    return None
                return nodeWrap(ret)
            elif attr == "last":
                ret = libxml2mod.last(self._o)
                if ret == None:
                    return None
                return nodeWrap(ret)
            elif attr == "next":
                ret = libxml2mod.next(self._o)
                if ret == None:
                    return None
                return nodeWrap(ret)
            elif attr == "prev":
                ret = libxml2mod.prev(self._o)
                if ret == None:
                    return None
                return nodeWrap(ret)
            elif attr == "content":
                return libxml2mod.xmlNodeGetContent(self._o)
            elif attr == "name":
                return libxml2mod.name(self._o)
            elif attr == "type":
                return libxml2mod.type(self._o)
            elif attr == "doc":
                ret = libxml2mod.doc(self._o)
                if ret == None:
                    if self.type == "document_xml" or self.type == "document_html":
                        return xmlDoc(_obj=self._o)
                    else:
                        return None
                return xmlDoc(_obj=ret)
            raise AttributeError(attr)
    else:
        parent = property(get_parent, None, None, "Parent node")
        children = property(get_children, None, None, "First child node")
        last = property(get_last, None, None, "Last sibling node")
        next = property(get_next, None, None, "Next sibling node")
        prev = property(get_prev, None, None, "Previous sibling node")
        properties = property(get_properties, None, None, "List of properies")
        content = property(get_content, None, None, "Content of this node")
        name = property(get_name, None, None, "Node name")
        type = property(get_type, None, None, "Node type")
        doc = property(get_doc, None, None, "The document this node belongs to")

    #
    # Serialization routines, the optional arguments have the following
    # meaning:
    #     encoding: string to ask saving in a specific encoding
    #     indent: if 1 the serializer is asked to indent the output
    #
    def serialize(self, encoding = None, format = 0):
        return libxml2mod.serializeNode(self._o, encoding, format)
    def saveTo(self, file, encoding = None, format = 0):
        return libxml2mod.saveNodeTo(self._o, file, encoding, format)

    #
    # Canonicalization routines:
    #
    #   nodes: the node set (tuple or list) to be included in the
    #     canonized image or None if all document nodes should be
    #     included.
    #   exclusive: the exclusive flag (0 - non-exclusive
    #     canonicalization; otherwise - exclusive canonicalization)
    #   prefixes: the list of inclusive namespace prefixes (strings),
    #     or None if there is no inclusive namespaces (only for
    #     exclusive canonicalization, ignored otherwise)
    #   with_comments: include comments in the result (!=0) or not
    #     (==0)
    def c14nMemory(self,
                   nodes=None,
                   exclusive=0,
                   prefixes=None,
                   with_comments=0):
        if nodes:
            nodes = [n._o for n in nodes]
        return libxml2mod.xmlC14NDocDumpMemory(
            self.get_doc()._o,
            nodes,
            exclusive != 0,
            prefixes,
            with_comments != 0)
    def c14nSaveTo(self,
                   file,
                   nodes=None,
                   exclusive=0,
                   prefixes=None,
                   with_comments=0):
        if nodes:
            nodes = [n._o for n in nodes]
        return libxml2mod.xmlC14NDocSaveTo(
            self.get_doc()._o,
            nodes,
            exclusive != 0,
            prefixes,
            with_comments != 0,
            file)

    #
    # Selecting nodes using XPath, a bit slow because the context
    # is allocated/freed every time but convenient.
    #
    def xpathEval(self, expr):
        doc = self.doc
        if doc == None:
            return None
        ctxt = doc.xpathNewContext()
        ctxt.setContextNode(self)
        res = ctxt.xpathEval(expr)
        ctxt.xpathFreeContext()
        return res

#    #
#    # Selecting nodes using XPath, faster because the context
#    # is allocated just once per xmlDoc.
#    #
#    # Removed: DV memleaks c.f. #126735
#    #
#    def xpathEval2(self, expr):
#        doc = self.doc
#        if doc == None:
#            return None
#        try:
#            doc._ctxt.setContextNode(self)
#        except:
#            doc._ctxt = doc.xpathNewContext()
#            doc._ctxt.setContextNode(self)
#        res = doc._ctxt.xpathEval(expr)
#        return res
    def xpathEval2(self, expr):
        return self.xpathEval(expr)

    # Remove namespaces
    def removeNsDef(self, href):
        """
        Remove a namespace definition from a node.  If href is None,
        remove all of the ns definitions on that node.  The removed
        namespaces are returned as a linked list.

        Note: If any child nodes referred to the removed namespaces,
        they will be left with dangling links.  You should call
        renconciliateNs() to fix those pointers.

        Note: This method does not free memory taken by the ns
        definitions.  You will need to free it manually with the
        freeNsList() method on the returns xmlNs object.
        """

        ret = libxml2mod.xmlNodeRemoveNsDef(self._o, href)
        if ret is None:return None
        __tmp = xmlNs(_obj=ret)
        return __tmp

    # support for python2 iterators
    def walk_depth_first(self):
        return xmlCoreDepthFirstItertor(self)
    def walk_breadth_first(self):
        return xmlCoreBreadthFirstItertor(self)
    __iter__ = walk_depth_first

    def free(self):
        try:
            self.doc._ctxt.xpathFreeContext()
        except:
            pass
        libxml2mod.xmlFreeDoc(self._o)


#
# implements the depth-first iterator for libxml2 DOM tree
#
class xmlCoreDepthFirstItertor:
    def __init__(self, node):
        self.node = node
        self.parents = []
    def __iter__(self):
        return self
    def next(self):
        while 1:
            if self.node:
                ret = self.node
                self.parents.append(self.node)
                self.node = self.node.children
                return ret
            try:
                parent = self.parents.pop()
            except IndexError:
                raise StopIteration
            self.node = parent.next

#
# implements the breadth-first iterator for libxml2 DOM tree
#
class xmlCoreBreadthFirstItertor:
    def __init__(self, node):
        self.node = node
        self.parents = []
    def __iter__(self):
        return self
    def next(self):
        while 1:
            if self.node:
                ret = self.node
                self.parents.append(self.node)
                self.node = self.node.next
                return ret
            try:
                parent = self.parents.pop()
            except IndexError:
                raise StopIteration
            self.node = parent.children

#
# converters to present a nicer view of the XPath returns
#
def nodeWrap(o):
    # TODO try to cast to the most appropriate node class
    name = libxml2mod.type(o)
    if name == "element" or name == "text":
        return xmlNode(_obj=o)
    if name == "attribute":
        return xmlAttr(_obj=o)
    if name[0:8] == "document":
        return xmlDoc(_obj=o)
    if name == "namespace":
        return xmlNs(_obj=o)
    if name == "elem_decl":
        return xmlElement(_obj=o)
    if name == "attribute_decl":
        return xmlAttribute(_obj=o)
    if name == "entity_decl":
        return xmlEntity(_obj=o)
    if name == "dtd":
        return xmlDtd(_obj=o)
    return xmlNode(_obj=o)

def xpathObjectRet(o):
    otype = type(o)
    if otype == type([]):
        ret = list(map(xpathObjectRet, o))
        return ret
    elif otype == type(()):
        ret = list(map(xpathObjectRet, o))
        return tuple(ret)
    elif otype == type('') or otype == type(0) or otype == type(0.0):
        return o
    else:
        return nodeWrap(o)

#
# register an XPath function
#
def registerXPathFunction(ctxt, name, ns_uri, f):
    ret = libxml2mod.xmlRegisterXPathFunction(ctxt, name, ns_uri, f)

#
# For the xmlTextReader parser configuration
#
PARSER_LOADDTD=1
PARSER_DEFAULTATTRS=2
PARSER_VALIDATE=3
PARSER_SUBST_ENTITIES=4

#
# For the error callback severities
#
PARSER_SEVERITY_VALIDITY_WARNING=1
PARSER_SEVERITY_VALIDITY_ERROR=2
PARSER_SEVERITY_WARNING=3
PARSER_SEVERITY_ERROR=4

#
# register the libxml2 error handler
#
def registerErrorHandler(f, ctx):
    """Register a Python written function to for error reporting.
       The function is called back as f(ctx, error). """
    import sys
    if 'libxslt' not in sys.modules:
        # normal behaviour when libxslt is not imported
        ret = libxml2mod.xmlRegisterErrorHandler(f,ctx)
    else:
        # when libxslt is already imported, one must
        # use libxst's error handler instead
        import libxslt
        ret = libxslt.registerErrorHandler(f,ctx)
    return ret

class parserCtxtCore:

    def __init__(self, _obj=None):
        if _obj != None:
            self._o = _obj;
            return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeParserCtxt(self._o)
        self._o = None

    def setErrorHandler(self,f,arg):
        """Register an error handler that will be called back as
           f(arg,msg,severity,reserved).

           @reserved is currently always None."""
        libxml2mod.xmlParserCtxtSetErrorHandler(self._o,f,arg)

    def getErrorHandler(self):
        """Return (f,arg) as previously registered with setErrorHandler
           or (None,None)."""
        return libxml2mod.xmlParserCtxtGetErrorHandler(self._o)

    def addLocalCatalog(self, uri):
        """Register a local catalog with the parser"""
        return libxml2mod.addLocalCatalog(self._o, uri)


class ValidCtxtCore:

    def __init__(self, *args, **kw):
        pass

    def setValidityErrorHandler(self, err_func, warn_func, arg=None):
        """
        Register error and warning handlers for DTD validation.
        These will be called back as f(msg,arg)
        """
        libxml2mod.xmlSetValidErrors(self._o, err_func, warn_func, arg)


class SchemaValidCtxtCore:

    def __init__(self, *args, **kw):
        pass

    def setValidityErrorHandler(self, err_func, warn_func, arg=None):
        """
        Register error and warning handlers for Schema validation.
        These will be called back as f(msg,arg)
        """
        libxml2mod.xmlSchemaSetValidErrors(self._o, err_func, warn_func, arg)


class relaxNgValidCtxtCore:

    def __init__(self, *args, **kw):
        pass

    def setValidityErrorHandler(self, err_func, warn_func, arg=None):
        """
        Register error and warning handlers for RelaxNG validation.
        These will be called back as f(msg,arg)
        """
        libxml2mod.xmlRelaxNGSetValidErrors(self._o, err_func, warn_func, arg)


def _xmlTextReaderErrorFunc(xxx_todo_changeme,msg,severity,locator):
    """Intermediate callback to wrap the locator"""
    (f,arg) = xxx_todo_changeme
    return f(arg,msg,severity,xmlTextReaderLocator(locator))

class xmlTextReaderCore:

    def __init__(self, _obj=None):
        self.input = None
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeTextReader(self._o)
        self._o = None

    def SetErrorHandler(self,f,arg):
        """Register an error handler that will be called back as
           f(arg,msg,severity,locator)."""
        if f is None:
            libxml2mod.xmlTextReaderSetErrorHandler(\
                self._o,None,None)
        else:
            libxml2mod.xmlTextReaderSetErrorHandler(\
                self._o,_xmlTextReaderErrorFunc,(f,arg))

    def GetErrorHandler(self):
        """Return (f,arg) as previously registered with setErrorHandler
           or (None,None)."""
        f,arg = libxml2mod.xmlTextReaderGetErrorHandler(self._o)
        if f is None:
            return None,None
        else:
            # assert f is _xmlTextReaderErrorFunc
            return arg

#
# The cleanup now goes though a wrapper in libxml.c
#
def cleanupParser():
    libxml2mod.xmlPythonCleanupParser()

#
# The interface to xmlRegisterInputCallbacks.
# Since this API does not allow to pass a data object along with
# match/open callbacks, it is necessary to maintain a list of all
# Python callbacks.
#
__input_callbacks = []
def registerInputCallback(func):
    def findOpenCallback(URI):
        for cb in reversed(__input_callbacks):
            o = cb(URI)
            if o is not None:
                return o
    libxml2mod.xmlRegisterInputCallback(findOpenCallback)
    __input_callbacks.append(func)

def popInputCallbacks():
    # First pop python-level callbacks, when no more available - start
    # popping built-in ones.
    if len(__input_callbacks) > 0:
        __input_callbacks.pop()
    if len(__input_callbacks) == 0:
        libxml2mod.xmlUnregisterInputCallback()

# WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
#
# Everything before this line comes from libxml.py
# Everything after this line is automatically generated
#
# WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

#
# Functions from module HTMLparser
#

def htmlCreateMemoryParserCtxt(buffer, size):
    """Create a parser context for an HTML in-memory document. """
    ret = libxml2mod.htmlCreateMemoryParserCtxt(buffer, size)
    if ret is None:raise parserError('htmlCreateMemoryParserCtxt() failed')
    return parserCtxt(_obj=ret)

def htmlHandleOmittedElem(val):
    """Set and return the previous value for handling HTML omitted
       tags. """
    ret = libxml2mod.htmlHandleOmittedElem(val)
    return ret

def htmlIsScriptAttribute(name):
    """Check if an attribute is of content type Script """
    ret = libxml2mod.htmlIsScriptAttribute(name)
    return ret

def htmlNewParserCtxt():
    """Allocate and initialize a new parser context. """
    ret = libxml2mod.htmlNewParserCtxt()
    if ret is None:raise parserError('htmlNewParserCtxt() failed')
    return parserCtxt(_obj=ret)

def htmlParseDoc(cur, encoding):
    """parse an HTML in-memory document and build a tree. """
    ret = libxml2mod.htmlParseDoc(cur, encoding)
    if ret is None:raise parserError('htmlParseDoc() failed')
    return xmlDoc(_obj=ret)

def htmlParseFile(filename, encoding):
    """parse an HTML file and build a tree. Automatic support for
      ZLIB/Compress compressed document is provided by default if
       found at compile-time. """
    ret = libxml2mod.htmlParseFile(filename, encoding)
    if ret is None:raise parserError('htmlParseFile() failed')
    return xmlDoc(_obj=ret)

def htmlReadDoc(cur, URL, encoding, options):
    """parse an XML in-memory document and build a tree. """
    ret = libxml2mod.htmlReadDoc(cur, URL, encoding, options)
    if ret is None:raise treeError('htmlReadDoc() failed')
    return xmlDoc(_obj=ret)

def htmlReadFd(fd, URL, encoding, options):
    """parse an XML from a file descriptor and build a tree. """
    ret = libxml2mod.htmlReadFd(fd, URL, encoding, options)
    if ret is None:raise treeError('htmlReadFd() failed')
    return xmlDoc(_obj=ret)

def htmlReadFile(filename, encoding, options):
    """parse an XML file from the filesystem or the network. """
    ret = libxml2mod.htmlReadFile(filename, encoding, options)
    if ret is None:raise treeError('htmlReadFile() failed')
    return xmlDoc(_obj=ret)

def htmlReadMemory(buffer, size, URL, encoding, options):
    """parse an XML in-memory document and build a tree. """
    ret = libxml2mod.htmlReadMemory(buffer, size, URL, encoding, options)
    if ret is None:raise treeError('htmlReadMemory() failed')
    return xmlDoc(_obj=ret)

#
# Functions from module HTMLtree
#

def htmlIsBooleanAttr(name):
    """Determine if a given attribute is a boolean attribute. """
    ret = libxml2mod.htmlIsBooleanAttr(name)
    return ret

def htmlNewDoc(URI, ExternalID):
    """Creates a new HTML document """
    ret = libxml2mod.htmlNewDoc(URI, ExternalID)
    if ret is None:raise treeError('htmlNewDoc() failed')
    return xmlDoc(_obj=ret)

def htmlNewDocNoDtD(URI, ExternalID):
    """Creates a new HTML document without a DTD node if @URI and
       @ExternalID are None """
    ret = libxml2mod.htmlNewDocNoDtD(URI, ExternalID)
    if ret is None:raise treeError('htmlNewDocNoDtD() failed')
    return xmlDoc(_obj=ret)

#
# Functions from module SAX2
#

def SAXDefaultVersion(version):
    """Set the default version of SAX used globally by the
      library. By default, during initialization the default is
      set to 2. Note that it is generally a better coding style
      to use xmlSAXVersion() to set up the version explicitly for
       a given parsing context. """
    ret = libxml2mod.xmlSAXDefaultVersion(version)
    return ret

def defaultSAXHandlerInit():
    """Initialize the default SAX2 handler """
    libxml2mod.xmlDefaultSAXHandlerInit()

def docbDefaultSAXHandlerInit():
    """Initialize the default SAX handler """
    libxml2mod.docbDefaultSAXHandlerInit()

def htmlDefaultSAXHandlerInit():
    """Initialize the default SAX handler """
    libxml2mod.htmlDefaultSAXHandlerInit()

#
# Functions from module catalog
#

def catalogAdd(type, orig, replace):
    """Add an entry in the catalog, it may overwrite existing but
      different entries. If called before any other catalog
      routine, allows to override the default shared catalog put
       in place by xmlInitializeCatalog(); """
    ret = libxml2mod.xmlCatalogAdd(type, orig, replace)
    return ret

def catalogCleanup():
    """Free up all the memory associated with catalogs """
    libxml2mod.xmlCatalogCleanup()

def catalogConvert():
    """Convert all the SGML catalog entries as XML ones """
    ret = libxml2mod.xmlCatalogConvert()
    return ret

def catalogDump(out):
    """Dump all the global catalog content to the given file. """
    if out is not None: out.flush()
    libxml2mod.xmlCatalogDump(out)

def catalogGetPublic(pubID):
    """Try to lookup the catalog reference associated to a public
       ID DEPRECATED, use xmlCatalogResolvePublic() """
    ret = libxml2mod.xmlCatalogGetPublic(pubID)
    return ret

def catalogGetSystem(sysID):
    """Try to lookup the catalog reference associated to a system
       ID DEPRECATED, use xmlCatalogResolveSystem() """
    ret = libxml2mod.xmlCatalogGetSystem(sysID)
    return ret

def catalogRemove(value):
    """Remove an entry from the catalog """
    ret = libxml2mod.xmlCatalogRemove(value)
    return ret

def catalogResolve(pubID, sysID):
    """Do a complete resolution lookup of an External Identifier """
    ret = libxml2mod.xmlCatalogResolve(pubID, sysID)
    return ret

def catalogResolvePublic(pubID):
    """Try to lookup the catalog reference associated to a public
       ID """
    ret = libxml2mod.xmlCatalogResolvePublic(pubID)
    return ret

def catalogResolveSystem(sysID):
    """Try to lookup the catalog resource for a system ID """
    ret = libxml2mod.xmlCatalogResolveSystem(sysID)
    return ret

def catalogResolveURI(URI):
    """Do a complete resolution lookup of an URI """
    ret = libxml2mod.xmlCatalogResolveURI(URI)
    return ret

def catalogSetDebug(level):
    """Used to set the debug level for catalog operation, 0
       disable debugging, 1 enable it """
    ret = libxml2mod.xmlCatalogSetDebug(level)
    return ret

def initializeCatalog():
    """Do the catalog initialization. this function is not thread
      safe, catalog initialization should preferably be done once
       at startup """
    libxml2mod.xmlInitializeCatalog()

def loadACatalog(filename):
    """Load the catalog and build the associated data structures.
      This can be either an XML Catalog or an SGML Catalog It
      will recurse in SGML CATALOG entries. On the other hand XML
       Catalogs are not handled recursively. """
    ret = libxml2mod.xmlLoadACatalog(filename)
    if ret is None:raise treeError('xmlLoadACatalog() failed')
    return catalog(_obj=ret)

def loadCatalog(filename):
    """Load the catalog and makes its definitions effective for
      the default external entity loader. It will recurse in SGML
      CATALOG entries. this function is not thread safe, catalog
       initialization should preferably be done once at startup """
    ret = libxml2mod.xmlLoadCatalog(filename)
    return ret

def loadCatalogs(pathss):
    """Load the catalogs and makes their definitions effective for
      the default external entity loader. this function is not
      thread safe, catalog initialization should preferably be
       done once at startup """
    libxml2mod.xmlLoadCatalogs(pathss)

def loadSGMLSuperCatalog(filename):
    """Load an SGML super catalog. It won't expand CATALOG or
      DELEGATE references. This is only needed for manipulating
      SGML Super Catalogs like adding and removing CATALOG or
       DELEGATE entries. """
    ret = libxml2mod.xmlLoadSGMLSuperCatalog(filename)
    if ret is None:raise treeError('xmlLoadSGMLSuperCatalog() failed')
    return catalog(_obj=ret)

def newCatalog(sgml):
    """create a new Catalog. """
    ret = libxml2mod.xmlNewCatalog(sgml)
    if ret is None:raise treeError('xmlNewCatalog() failed')
    return catalog(_obj=ret)

def parseCatalogFile(filename):
    """parse an XML file and build a tree. It's like
       xmlParseFile() except it bypass all catalog lookups. """
    ret = libxml2mod.xmlParseCatalogFile(filename)
    if ret is None:raise parserError('xmlParseCatalogFile() failed')
    return xmlDoc(_obj=ret)

#
# Functions from module chvalid
#

def isBaseChar(ch):
    """This function is DEPRECATED. Use xmlIsBaseChar_ch or
       xmlIsBaseCharQ instead """
    ret = libxml2mod.xmlIsBaseChar(ch)
    return ret

def isBlank(ch):
    """This function is DEPRECATED. Use xmlIsBlank_ch or
       xmlIsBlankQ instead """
    ret = libxml2mod.xmlIsBlank(ch)
    return ret

def isChar(ch):
    """This function is DEPRECATED. Use xmlIsChar_ch or xmlIsCharQ
       instead """
    ret = libxml2mod.xmlIsChar(ch)
    return ret

def isCombining(ch):
    """This function is DEPRECATED. Use xmlIsCombiningQ instead """
    ret = libxml2mod.xmlIsCombining(ch)
    return ret

def isDigit(ch):
    """This function is DEPRECATED. Use xmlIsDigit_ch or
       xmlIsDigitQ instead """
    ret = libxml2mod.xmlIsDigit(ch)
    return ret

def isExtender(ch):
    """This function is DEPRECATED. Use xmlIsExtender_ch or
       xmlIsExtenderQ instead """
    ret = libxml2mod.xmlIsExtender(ch)
    return ret

def isIdeographic(ch):
    """This function is DEPRECATED. Use xmlIsIdeographicQ instead """
    ret = libxml2mod.xmlIsIdeographic(ch)
    return ret

def isPubidChar(ch):
    """This function is DEPRECATED. Use xmlIsPubidChar_ch or
       xmlIsPubidCharQ instead """
    ret = libxml2mod.xmlIsPubidChar(ch)
    return ret

#
# Functions from module debugXML
#

def boolToText(boolval):
    """Convenient way to turn bool into text """
    ret = libxml2mod.xmlBoolToText(boolval)
    return ret

def debugDumpString(output, str):
    """Dumps informations about the string, shorten it if necessary """
    if output is not None: output.flush()
    libxml2mod.xmlDebugDumpString(output, str)

def shellPrintXPathError(errorType, arg):
    """Print the xpath error to libxml default error channel """
    libxml2mod.xmlShellPrintXPathError(errorType, arg)

#
# Functions from module dict
#

def dictCleanup():
    """Free the dictionary mutex. Do not call unless sure the
       library is not in use anymore ! """
    libxml2mod.xmlDictCleanup()

def initializeDict():
    """Do the dictionary mutex initialization. this function is
       deprecated """
    ret = libxml2mod.xmlInitializeDict()
    return ret

#
# Functions from module encoding
#

def addEncodingAlias(name, alias):
    """Registers an alias @alias for an encoding named @name.
       Existing alias will be overwritten. """
    ret = libxml2mod.xmlAddEncodingAlias(name, alias)
    return ret

def cleanupCharEncodingHandlers():
    """Cleanup the memory allocated for the char encoding support,
       it unregisters all the encoding handlers and the aliases. """
    libxml2mod.xmlCleanupCharEncodingHandlers()

def cleanupEncodingAliases():
    """Unregisters all aliases """
    libxml2mod.xmlCleanupEncodingAliases()

def delEncodingAlias(alias):
    """Unregisters an encoding alias @alias """
    ret = libxml2mod.xmlDelEncodingAlias(alias)
    return ret

def encodingAlias(alias):
    """Lookup an encoding name for the given alias. """
    ret = libxml2mod.xmlGetEncodingAlias(alias)
    return ret

def initCharEncodingHandlers():
    """Initialize the char encoding support, it registers the
      default encoding supported. NOTE: while public, this
      function usually doesn't need to be called in normal
       processing. """
    libxml2mod.xmlInitCharEncodingHandlers()

#
# Functions from module entities
#

def cleanupPredefinedEntities():
    """Cleanup up the predefined entities table. Deprecated call """
    libxml2mod.xmlCleanupPredefinedEntities()

def initializePredefinedEntities():
    """Set up the predefined entities. Deprecated call """
    libxml2mod.xmlInitializePredefinedEntities()

def predefinedEntity(name):
    """Check whether this name is an predefined entity. """
    ret = libxml2mod.xmlGetPredefinedEntity(name)
    if ret is None:raise treeError('xmlGetPredefinedEntity() failed')
    return xmlEntity(_obj=ret)

#
# Functions from module globals
#

def cleanupGlobals():
    """Additional cleanup for multi-threading """
    libxml2mod.xmlCleanupGlobals()

def initGlobals():
    """Additional initialisation for multi-threading """
    libxml2mod.xmlInitGlobals()

def thrDefDefaultBufferSize(v):
    ret = libxml2mod.xmlThrDefDefaultBufferSize(v)
    return ret

def thrDefDoValidityCheckingDefaultValue(v):
    ret = libxml2mod.xmlThrDefDoValidityCheckingDefaultValue(v)
    return ret

def thrDefGetWarningsDefaultValue(v):
    ret = libxml2mod.xmlThrDefGetWarningsDefaultValue(v)
    return ret

def thrDefIndentTreeOutput(v):
    ret = libxml2mod.xmlThrDefIndentTreeOutput(v)
    return ret

def thrDefKeepBlanksDefaultValue(v):
    ret = libxml2mod.xmlThrDefKeepBlanksDefaultValue(v)
    return ret

def thrDefLineNumbersDefaultValue(v):
    ret = libxml2mod.xmlThrDefLineNumbersDefaultValue(v)
    return ret

def thrDefLoadExtDtdDefaultValue(v):
    ret = libxml2mod.xmlThrDefLoadExtDtdDefaultValue(v)
    return ret

def thrDefParserDebugEntities(v):
    ret = libxml2mod.xmlThrDefParserDebugEntities(v)
    return ret

def thrDefPedanticParserDefaultValue(v):
    ret = libxml2mod.xmlThrDefPedanticParserDefaultValue(v)
    return ret

def thrDefSaveNoEmptyTags(v):
    ret = libxml2mod.xmlThrDefSaveNoEmptyTags(v)
    return ret

def thrDefSubstituteEntitiesDefaultValue(v):
    ret = libxml2mod.xmlThrDefSubstituteEntitiesDefaultValue(v)
    return ret

def thrDefTreeIndentString(v):
    ret = libxml2mod.xmlThrDefTreeIndentString(v)
    return ret

#
# Functions from module nanoftp
#

def nanoFTPCleanup():
    """Cleanup the FTP protocol layer. This cleanup proxy
       informations. """
    libxml2mod.xmlNanoFTPCleanup()

def nanoFTPInit():
    """Initialize the FTP protocol layer. Currently it just checks
       for proxy informations, and get the hostname """
    libxml2mod.xmlNanoFTPInit()

def nanoFTPProxy(host, port, user, passwd, type):
    """Setup the FTP proxy informations. This can also be done by
      using ftp_proxy ftp_proxy_user and ftp_proxy_password
       environment variables. """
    libxml2mod.xmlNanoFTPProxy(host, port, user, passwd, type)

def nanoFTPScanProxy(URL):
    """(Re)Initialize the FTP Proxy context by parsing the URL and
      finding the protocol host port it indicates. Should be like
      ftp://myproxy/ or ftp://myproxy:3128/ A None URL cleans up
       proxy informations. """
    libxml2mod.xmlNanoFTPScanProxy(URL)

#
# Functions from module nanohttp
#

def nanoHTTPCleanup():
    """Cleanup the HTTP protocol layer. """
    libxml2mod.xmlNanoHTTPCleanup()

def nanoHTTPInit():
    """Initialize the HTTP protocol layer. Currently it just
       checks for proxy informations """
    libxml2mod.xmlNanoHTTPInit()

def nanoHTTPScanProxy(URL):
    """(Re)Initialize the HTTP Proxy context by parsing the URL
      and finding the protocol host port it indicates. Should be
      like http://myproxy/ or http://myproxy:3128/ A None URL
       cleans up proxy informations. """
    libxml2mod.xmlNanoHTTPScanProxy(URL)

#
# Functions from module parser
#

def createDocParserCtxt(cur):
    """Creates a parser context for an XML in-memory document. """
    ret = libxml2mod.xmlCreateDocParserCtxt(cur)
    if ret is None:raise parserError('xmlCreateDocParserCtxt() failed')
    return parserCtxt(_obj=ret)

def initParser():
    """Initialization function for the XML parser. This is not
      reentrant. Call once before processing in case of use in
       multithreaded programs. """
    libxml2mod.xmlInitParser()

def keepBlanksDefault(val):
    """Set and return the previous value for default blanks text
      nodes support. The 1.x version of the parser used an
      heuristic to try to detect ignorable white spaces. As a
      result the SAX callback was generating
      xmlSAX2IgnorableWhitespace() callbacks instead of
      characters() one, and when using the DOM output text nodes
      containing those blanks were not generated. The 2.x and
      later version will switch to the XML standard way and
      ignorableWhitespace() are only generated when running the
      parser in validating mode and when the current element
      doesn't allow CDATA or mixed content. This function is
      provided as a way to force the standard behavior on 1.X
      libs and to switch back to the old mode for compatibility
      when running 1.X client code on 2.X . Upgrade of 1.X code
      should be done by using xmlIsBlankNode() commodity function
      to detect the "empty" nodes generated. This value also
      affect autogeneration of indentation when saving code if
       blanks sections are kept, indentation is not generated. """
    ret = libxml2mod.xmlKeepBlanksDefault(val)
    return ret

def lineNumbersDefault(val):
    """Set and return the previous value for enabling line numbers
      in elements contents. This may break on old application and
       is turned off by default. """
    ret = libxml2mod.xmlLineNumbersDefault(val)
    return ret

def newParserCtxt():
    """Allocate and initialize a new parser context. """
    ret = libxml2mod.xmlNewParserCtxt()
    if ret is None:raise parserError('xmlNewParserCtxt() failed')
    return parserCtxt(_obj=ret)

def parseDTD(ExternalID, SystemID):
    """Load and parse an external subset. """
    ret = libxml2mod.xmlParseDTD(ExternalID, SystemID)
    if ret is None:raise parserError('xmlParseDTD() failed')
    return xmlDtd(_obj=ret)

def parseDoc(cur):
    """parse an XML in-memory document and build a tree. """
    ret = libxml2mod.xmlParseDoc(cur)
    if ret is None:raise parserError('xmlParseDoc() failed')
    return xmlDoc(_obj=ret)

def parseEntity(filename):
    """parse an XML external entity out of context and build a
      tree.  [78] extParsedEnt ::= TextDecl? content  This
       correspond to a "Well Balanced" chunk """
    ret = libxml2mod.xmlParseEntity(filename)
    if ret is None:raise parserError('xmlParseEntity() failed')
    return xmlDoc(_obj=ret)

def parseFile(filename):
    """parse an XML file and build a tree. Automatic support for
      ZLIB/Compress compressed document is provided by default if
       found at compile-time. """
    ret = libxml2mod.xmlParseFile(filename)
    if ret is None:raise parserError('xmlParseFile() failed')
    return xmlDoc(_obj=ret)

def parseMemory(buffer, size):
    """parse an XML in-memory block and build a tree. """
    ret = libxml2mod.xmlParseMemory(buffer, size)
    if ret is None:raise parserError('xmlParseMemory() failed')
    return xmlDoc(_obj=ret)

def pedanticParserDefault(val):
    """Set and return the previous value for enabling pedantic
       warnings. """
    ret = libxml2mod.xmlPedanticParserDefault(val)
    return ret

def readDoc(cur, URL, encoding, options):
    """parse an XML in-memory document and build a tree. """
    ret = libxml2mod.xmlReadDoc(cur, URL, encoding, options)
    if ret is None:raise treeError('xmlReadDoc() failed')
    return xmlDoc(_obj=ret)

def readFd(fd, URL, encoding, options):
    """parse an XML from a file descriptor and build a tree. NOTE
      that the file descriptor will not be closed when the reader
       is closed or reset. """
    ret = libxml2mod.xmlReadFd(fd, URL, encoding, options)
    if ret is None:raise treeError('xmlReadFd() failed')
    return xmlDoc(_obj=ret)

def readFile(filename, encoding, options):
    """parse an XML file from the filesystem or the network. """
    ret = libxml2mod.xmlReadFile(filename, encoding, options)
    if ret is None:raise treeError('xmlReadFile() failed')
    return xmlDoc(_obj=ret)

def readMemory(buffer, size, URL, encoding, options):
    """parse an XML in-memory document and build a tree. """
    ret = libxml2mod.xmlReadMemory(buffer, size, URL, encoding, options)
    if ret is None:raise treeError('xmlReadMemory() failed')
    return xmlDoc(_obj=ret)

def recoverDoc(cur):
    """parse an XML in-memory document and build a tree. In the
      case the document is not Well Formed, a attempt to build a
       tree is tried anyway """
    ret = libxml2mod.xmlRecoverDoc(cur)
    if ret is None:raise treeError('xmlRecoverDoc() failed')
    return xmlDoc(_obj=ret)

def recoverFile(filename):
    """parse an XML file and build a tree. Automatic support for
      ZLIB/Compress compressed document is provided by default if
      found at compile-time. In the case the document is not Well
       Formed, it attempts to build a tree anyway """
    ret = libxml2mod.xmlRecoverFile(filename)
    if ret is None:raise treeError('xmlRecoverFile() failed')
    return xmlDoc(_obj=ret)

def recoverMemory(buffer, size):
    """parse an XML in-memory block and build a tree. In the case
      the document is not Well Formed, an attempt to build a tree
       is tried anyway """
    ret = libxml2mod.xmlRecoverMemory(buffer, size)
    if ret is None:raise treeError('xmlRecoverMemory() failed')
    return xmlDoc(_obj=ret)

def substituteEntitiesDefault(val):
    """Set and return the previous value for default entity
      support. Initially the parser always keep entity references
      instead of substituting entity values in the output. This
      function has to be used to change the default parser
      behavior SAX::substituteEntities() has to be used for
       changing that on a file by file basis. """
    ret = libxml2mod.xmlSubstituteEntitiesDefault(val)
    return ret

#
# Functions from module parserInternals
#

def checkLanguageID(lang):
    """Checks that the value conforms to the LanguageID
      production:  NOTE: this is somewhat deprecated, those
      productions were removed from the XML Second edition.  [33]
      LanguageID ::= Langcode ('-' Subcode)* [34] Langcode ::=
      ISO639Code |  IanaCode |  UserCode [35] ISO639Code ::=
      ([a-z] | [A-Z]) ([a-z] | [A-Z]) [36] IanaCode ::= ('i' |
      'I') '-' ([a-z] | [A-Z])+ [37] UserCode ::= ('x' | 'X') '-'
      ([a-z] | [A-Z])+ [38] Subcode ::= ([a-z] | [A-Z])+  The
      current REC reference the sucessors of RFC 1766, currently
      5646  http://www.rfc-editor.org/rfc/rfc5646.txt langtag
      = language ["-" script] ["-" region] *("-" variant) *("-"
      extension) ["-" privateuse] language      = 2*3ALPHA
      ; shortest ISO 639 code ["-" extlang]       ; sometimes
      followed by ; extended language subtags / 4ALPHA
      ; or reserved for future use / 5*8ALPHA            ; or
      registered language subtag  extlang       = 3ALPHA
      ; selected ISO 639 codes *2("-" 3ALPHA)      ; permanently
      reserved  script        = 4ALPHA              ; ISO 15924
      code  region        = 2ALPHA              ; ISO 3166-1 code
      / 3DIGIT              ; UN M.49 code  variant       =
      5*8alphanum         ; registered variants / (DIGIT
      3alphanum)  extension     = singleton 1*("-" (2*8alphanum))
      ; Single alphanumerics ; "x" reserved for private use
      singleton     = DIGIT               ; 0 - 9 / %x41-57
      ; A - W / %x59-5A             ; Y - Z / %x61-77
      ; a - w / %x79-7A             ; y - z  it sounds right to
      still allow Irregular i-xxx IANA and user codes too The
      parser below doesn't try to cope with extension or
      privateuse that could be added but that's not interoperable
       anyway """
    ret = libxml2mod.xmlCheckLanguageID(lang)
    return ret

def copyChar(len, out, val):
    """append the char value in the array """
    ret = libxml2mod.xmlCopyChar(len, out, val)
    return ret

def copyCharMultiByte(out, val):
    """append the char value in the array """
    ret = libxml2mod.xmlCopyCharMultiByte(out, val)
    return ret

def createEntityParserCtxt(URL, ID, base):
    """Create a parser context for an external entity Automatic
      support for ZLIB/Compress compressed document is provided
       by default if found at compile-time. """
    ret = libxml2mod.xmlCreateEntityParserCtxt(URL, ID, base)
    if ret is None:raise parserError('xmlCreateEntityParserCtxt() failed')
    return parserCtxt(_obj=ret)

def createFileParserCtxt(filename):
    """Create a parser context for a file content. Automatic
      support for ZLIB/Compress compressed document is provided
       by default if found at compile-time. """
    ret = libxml2mod.xmlCreateFileParserCtxt(filename)
    if ret is None:raise parserError('xmlCreateFileParserCtxt() failed')
    return parserCtxt(_obj=ret)

def createMemoryParserCtxt(buffer, size):
    """Create a parser context for an XML in-memory document. """
    ret = libxml2mod.xmlCreateMemoryParserCtxt(buffer, size)
    if ret is None:raise parserError('xmlCreateMemoryParserCtxt() failed')
    return parserCtxt(_obj=ret)

def createURLParserCtxt(filename, options):
    """Create a parser context for a file or URL content.
      Automatic support for ZLIB/Compress compressed document is
      provided by default if found at compile-time and for file
       accesses """
    ret = libxml2mod.xmlCreateURLParserCtxt(filename, options)
    if ret is None:raise parserError('xmlCreateURLParserCtxt() failed')
    return parserCtxt(_obj=ret)

def htmlCreateFileParserCtxt(filename, encoding):
    """Create a parser context for a file content. Automatic
      support for ZLIB/Compress compressed document is provided
       by default if found at compile-time. """
    ret = libxml2mod.htmlCreateFileParserCtxt(filename, encoding)
    if ret is None:raise parserError('htmlCreateFileParserCtxt() failed')
    return parserCtxt(_obj=ret)

def htmlInitAutoClose():
    """Initialize the htmlStartCloseIndex for fast lookup of
      closing tags names. This is not reentrant. Call
      xmlInitParser() once before processing in case of use in
       multithreaded programs. """
    libxml2mod.htmlInitAutoClose()

def isLetter(c):
    """Check whether the character is allowed by the production
       [84] Letter ::= BaseChar | Ideographic """
    ret = libxml2mod.xmlIsLetter(c)
    return ret

def namePop(ctxt):
    """Pops the top element name from the name stack """
    if ctxt is None: ctxt__o = None
    else: ctxt__o = ctxt._o
    ret = libxml2mod.namePop(ctxt__o)
    return ret

def namePush(ctxt, value):
    """Pushes a new element name on top of the name stack """
    if ctxt is None: ctxt__o = None
    else: ctxt__o = ctxt._o
    ret = libxml2mod.namePush(ctxt__o, value)
    return ret

def nodePop(ctxt):
    """Pops the top element node from the node stack """
    if ctxt is None: ctxt__o = None
    else: ctxt__o = ctxt._o
    ret = libxml2mod.nodePop(ctxt__o)
    if ret is None:raise treeError('nodePop() failed')
    return xmlNode(_obj=ret)

def nodePush(ctxt, value):
    """Pushes a new element node on top of the node stack """
    if ctxt is None: ctxt__o = None
    else: ctxt__o = ctxt._o
    if value is None: value__o = None
    else: value__o = value._o
    ret = libxml2mod.nodePush(ctxt__o, value__o)
    return ret

#
# Functions from module python
#

def SAXParseFile(SAX, URI, recover):
    """Interface to parse an XML file or resource pointed by an
       URI to build an event flow to the SAX object """
    libxml2mod.xmlSAXParseFile(SAX, URI, recover)

def createInputBuffer(file, encoding):
    """Create a libxml2 input buffer from a Python file """
    ret = libxml2mod.xmlCreateInputBuffer(file, encoding)
    if ret is None:raise treeError('xmlCreateInputBuffer() failed')
    return inputBuffer(_obj=ret)

def createOutputBuffer(file, encoding):
    """Create a libxml2 output buffer from a Python file """
    ret = libxml2mod.xmlCreateOutputBuffer(file, encoding)
    if ret is None:raise treeError('xmlCreateOutputBuffer() failed')
    return outputBuffer(_obj=ret)

def createPushParser(SAX, chunk, size, URI):
    """Create a progressive XML parser context to build either an
      event flow if the SAX object is not None, or a DOM tree
       otherwise. """
    ret = libxml2mod.xmlCreatePushParser(SAX, chunk, size, URI)
    if ret is None:raise parserError('xmlCreatePushParser() failed')
    return parserCtxt(_obj=ret)

def debugMemory(activate):
    """Switch on the generation of line number for elements nodes.
      Also returns the number of bytes allocated and not freed by
       libxml2 since memory debugging was switched on. """
    ret = libxml2mod.xmlDebugMemory(activate)
    return ret

def dumpMemory():
    """dump the memory allocated in the file .memdump """
    libxml2mod.xmlDumpMemory()

def htmlCreatePushParser(SAX, chunk, size, URI):
    """Create a progressive HTML parser context to build either an
      event flow if the SAX object is not None, or a DOM tree
       otherwise. """
    ret = libxml2mod.htmlCreatePushParser(SAX, chunk, size, URI)
    if ret is None:raise parserError('htmlCreatePushParser() failed')
    return parserCtxt(_obj=ret)

def htmlSAXParseFile(SAX, URI, encoding):
    """Interface to parse an HTML file or resource pointed by an
       URI to build an event flow to the SAX object """
    libxml2mod.htmlSAXParseFile(SAX, URI, encoding)

def memoryUsed():
    """Returns the total amount of memory allocated by libxml2 """
    ret = libxml2mod.xmlMemoryUsed()
    return ret

def newNode(name):
    """Create a new Node """
    ret = libxml2mod.xmlNewNode(name)
    if ret is None:raise treeError('xmlNewNode() failed')
    return xmlNode(_obj=ret)

def pythonCleanupParser():
    """Cleanup function for the XML library. It tries to reclaim
      all parsing related global memory allocated for the library
      processing. It doesn't deallocate any document related
      memory. Calling this function should not prevent reusing
      the library but one should call xmlCleanupParser() only
      when the process has finished using the library or XML
       document built with it. """
    libxml2mod.xmlPythonCleanupParser()

def setEntityLoader(resolver):
    """Set the entity resolver as a python function """
    ret = libxml2mod.xmlSetEntityLoader(resolver)
    return ret

#
# Functions from module relaxng
#

def relaxNGCleanupTypes():
    """Cleanup the default Schemas type library associated to
       RelaxNG """
    libxml2mod.xmlRelaxNGCleanupTypes()

def relaxNGInitTypes():
    """Initilize the default type libraries. """
    ret = libxml2mod.xmlRelaxNGInitTypes()
    return ret

def relaxNGNewMemParserCtxt(buffer, size):
    """Create an XML RelaxNGs parse context for that memory buffer
       expected to contain an XML RelaxNGs file. """
    ret = libxml2mod.xmlRelaxNGNewMemParserCtxt(buffer, size)
    if ret is None:raise parserError('xmlRelaxNGNewMemParserCtxt() failed')
    return relaxNgParserCtxt(_obj=ret)

def relaxNGNewParserCtxt(URL):
    """Create an XML RelaxNGs parse context for that file/resource
       expected to contain an XML RelaxNGs file. """
    ret = libxml2mod.xmlRelaxNGNewParserCtxt(URL)
    if ret is None:raise parserError('xmlRelaxNGNewParserCtxt() failed')
    return relaxNgParserCtxt(_obj=ret)

#
# Functions from module tree
#

def buildQName(ncname, prefix, memory, len):
    """Builds the QName @prefix:@ncname in @memory if there is
      enough space and prefix is not None nor empty, otherwise
      allocate a new string. If prefix is None or empty it
       returns ncname. """
    ret = libxml2mod.xmlBuildQName(ncname, prefix, memory, len)
    return ret

def compressMode():
    """get the default compression mode used, ZLIB based. """
    ret = libxml2mod.xmlGetCompressMode()
    return ret

def isXHTML(systemID, publicID):
    """Try to find if the document correspond to an XHTML DTD """
    ret = libxml2mod.xmlIsXHTML(systemID, publicID)
    return ret

def newComment(content):
    """Creation of a new node containing a comment. """
    ret = libxml2mod.xmlNewComment(content)
    if ret is None:raise treeError('xmlNewComment() failed')
    return xmlNode(_obj=ret)

def newDoc(version):
    """Creates a new XML document """
    ret = libxml2mod.xmlNewDoc(version)
    if ret is None:raise treeError('xmlNewDoc() failed')
    return xmlDoc(_obj=ret)

def newPI(name, content):
    """Creation of a processing instruction element. Use
       xmlDocNewPI preferably to get string interning """
    ret = libxml2mod.xmlNewPI(name, content)
    if ret is None:raise treeError('xmlNewPI() failed')
    return xmlNode(_obj=ret)

def newText(content):
    """Creation of a new text node. """
    ret = libxml2mod.xmlNewText(content)
    if ret is None:raise treeError('xmlNewText() failed')
    return xmlNode(_obj=ret)

def newTextLen(content, len):
    """Creation of a new text node with an extra parameter for the
       content's length """
    ret = libxml2mod.xmlNewTextLen(content, len)
    if ret is None:raise treeError('xmlNewTextLen() failed')
    return xmlNode(_obj=ret)

def setCompressMode(mode):
    """set the default compression mode used, ZLIB based Correct
       values: 0 (uncompressed) to 9 (max compression) """
    libxml2mod.xmlSetCompressMode(mode)

def validateNCName(value, space):
    """Check that a value conforms to the lexical space of NCName """
    ret = libxml2mod.xmlValidateNCName(value, space)
    return ret

def validateNMToken(value, space):
    """Check that a value conforms to the lexical space of NMToken """
    ret = libxml2mod.xmlValidateNMToken(value, space)
    return ret

def validateName(value, space):
    """Check that a value conforms to the lexical space of Name """
    ret = libxml2mod.xmlValidateName(value, space)
    return ret

def validateQName(value, space):
    """Check that a value conforms to the lexical space of QName """
    ret = libxml2mod.xmlValidateQName(value, space)
    return ret

#
# Functions from module uri
#

def URIEscape(str):
    """Escaping routine, does not do validity checks ! It will try
      to escape the chars needing this, but this is heuristic
       based it's impossible to be sure. """
    ret = libxml2mod.xmlURIEscape(str)
    return ret

def URIEscapeStr(str, list):
    """This routine escapes a string to hex, ignoring reserved
       characters (a-z) and the characters in the exception list. """
    ret = libxml2mod.xmlURIEscapeStr(str, list)
    return ret

def URIUnescapeString(str, len, target):
    """Unescaping routine, but does not check that the string is
      an URI. The output is a direct unsigned char translation of
      %XX values (no encoding) Note that the length of the result
       can only be smaller or same size as the input string. """
    ret = libxml2mod.xmlURIUnescapeString(str, len, target)
    return ret

def buildRelativeURI(URI, base):
    """Expresses the URI of the reference in terms relative to the
      base.  Some examples of this operation include: base =
      "http://site1.com/docs/book1.html" URI input
      URI returned docs/pic1.gif                    pic1.gif
      docs/img/pic1.gif                img/pic1.gif img/pic1.gif
      ../img/pic1.gif http://site1.com/docs/pic1.gif   pic1.gif
      http://site2.com/docs/pic1.gif
      http://site2.com/docs/pic1.gif  base = "docs/book1.html"
      URI input                        URI returned docs/pic1.gif
      pic1.gif docs/img/pic1.gif                img/pic1.gif
      img/pic1.gif                     ../img/pic1.gif
      http://site1.com/docs/pic1.gif
      http://site1.com/docs/pic1.gif   Note: if the URI reference
      is really wierd or complicated, it may be worthwhile to
      first convert it into a "nice" one by calling xmlBuildURI
      (using 'base') before calling this routine, since this
      routine (for reasonable efficiency) assumes URI has already
       been through some validation. """
    ret = libxml2mod.xmlBuildRelativeURI(URI, base)
    return ret

def buildURI(URI, base):
    """Computes he final URI of the reference done by checking
      that the given URI is valid, and building the final URI
      using the base URI. This is processed according to section
      5.2 of the RFC 2396  5.2. Resolving Relative References to
       Absolute Form """
    ret = libxml2mod.xmlBuildURI(URI, base)
    return ret

def canonicPath(path):
    """Constructs a canonic path from the specified path. """
    ret = libxml2mod.xmlCanonicPath(path)
    return ret

def createURI():
    """Simply creates an empty xmlURI """
    ret = libxml2mod.xmlCreateURI()
    if ret is None:raise uriError('xmlCreateURI() failed')
    return URI(_obj=ret)

def normalizeURIPath(path):
    """Applies the 5 normalization steps to a path string--that
      is, RFC 2396 Section 5.2, steps 6.c through 6.g.
      Normalization occurs directly on the string, no new
       allocation is done """
    ret = libxml2mod.xmlNormalizeURIPath(path)
    return ret

def parseURI(str):
    """Parse an URI based on RFC 3986  URI-reference = [
       absoluteURI | relativeURI ] [ "#" fragment ] """
    ret = libxml2mod.xmlParseURI(str)
    if ret is None:raise uriError('xmlParseURI() failed')
    return URI(_obj=ret)

def parseURIRaw(str, raw):
    """Parse an URI but allows to keep intact the original
       fragments.  URI-reference = URI / relative-ref """
    ret = libxml2mod.xmlParseURIRaw(str, raw)
    if ret is None:raise uriError('xmlParseURIRaw() failed')
    return URI(_obj=ret)

def pathToURI(path):
    """Constructs an URI expressing the existing path """
    ret = libxml2mod.xmlPathToURI(path)
    return ret

#
# Functions from module valid
#

def newValidCtxt():
    """Allocate a validation context structure. """
    ret = libxml2mod.xmlNewValidCtxt()
    if ret is None:raise treeError('xmlNewValidCtxt() failed')
    return ValidCtxt(_obj=ret)

def validateNameValue(value):
    """Validate that the given value match Name production """
    ret = libxml2mod.xmlValidateNameValue(value)
    return ret

def validateNamesValue(value):
    """Validate that the given value match Names production """
    ret = libxml2mod.xmlValidateNamesValue(value)
    return ret

def validateNmtokenValue(value):
    """Validate that the given value match Nmtoken production  [
       VC: Name Token ] """
    ret = libxml2mod.xmlValidateNmtokenValue(value)
    return ret

def validateNmtokensValue(value):
    """Validate that the given value match Nmtokens production  [
       VC: Name Token ] """
    ret = libxml2mod.xmlValidateNmtokensValue(value)
    return ret

#
# Functions from module xmlIO
#

def checkFilename(path):
    """function checks to see if @path is a valid source (file,
      socket...) for XML.  if stat is not available on the target
       machine, """
    ret = libxml2mod.xmlCheckFilename(path)
    return ret

def cleanupInputCallbacks():
    """clears the entire input callback table. this includes the
       compiled-in I/O. """
    libxml2mod.xmlCleanupInputCallbacks()

def cleanupOutputCallbacks():
    """clears the entire output callback table. this includes the
       compiled-in I/O callbacks. """
    libxml2mod.xmlCleanupOutputCallbacks()

def fileMatch(filename):
    """input from FILE * """
    ret = libxml2mod.xmlFileMatch(filename)
    return ret

def iOFTPMatch(filename):
    """check if the URI matches an FTP one """
    ret = libxml2mod.xmlIOFTPMatch(filename)
    return ret

def iOHTTPMatch(filename):
    """check if the URI matches an HTTP one """
    ret = libxml2mod.xmlIOHTTPMatch(filename)
    return ret

def normalizeWindowsPath(path):
    """This function is obsolete. Please see xmlURIFromPath in
       uri.c for a better solution. """
    ret = libxml2mod.xmlNormalizeWindowsPath(path)
    return ret

def parserGetDirectory(filename):
    """lookup the directory for that file """
    ret = libxml2mod.xmlParserGetDirectory(filename)
    return ret

def registerDefaultInputCallbacks():
    """Registers the default compiled-in I/O handlers. """
    libxml2mod.xmlRegisterDefaultInputCallbacks()

def registerDefaultOutputCallbacks():
    """Registers the default compiled-in I/O handlers. """
    libxml2mod.xmlRegisterDefaultOutputCallbacks()

def registerHTTPPostCallbacks():
    """By default, libxml submits HTTP output requests using the
      "PUT" method. Calling this method changes the HTTP output
       method to use the "POST" method instead. """
    libxml2mod.xmlRegisterHTTPPostCallbacks()

#
# Functions from module xmlerror
#

def lastError():
    """Get the last global error registered. This is per thread if
       compiled with thread support. """
    ret = libxml2mod.xmlGetLastError()
    if ret is None:raise treeError('xmlGetLastError() failed')
    return Error(_obj=ret)

def resetLastError():
    """Cleanup the last global error registered. For parsing error
       this does not change the well-formedness result. """
    libxml2mod.xmlResetLastError()

#
# Functions from module xmlreader
#

def newTextReaderFilename(URI):
    """Create an xmlTextReader structure fed with the resource at
       @URI """
    ret = libxml2mod.xmlNewTextReaderFilename(URI)
    if ret is None:raise treeError('xmlNewTextReaderFilename() failed')
    return xmlTextReader(_obj=ret)

def readerForDoc(cur, URL, encoding, options):
    """Create an xmltextReader for an XML in-memory document. The
      parsing flags @options are a combination of xmlParserOption. """
    ret = libxml2mod.xmlReaderForDoc(cur, URL, encoding, options)
    if ret is None:raise treeError('xmlReaderForDoc() failed')
    return xmlTextReader(_obj=ret)

def readerForFd(fd, URL, encoding, options):
    """Create an xmltextReader for an XML from a file descriptor.
      The parsing flags @options are a combination of
      xmlParserOption. NOTE that the file descriptor will not be
       closed when the reader is closed or reset. """
    ret = libxml2mod.xmlReaderForFd(fd, URL, encoding, options)
    if ret is None:raise treeError('xmlReaderForFd() failed')
    return xmlTextReader(_obj=ret)

def readerForFile(filename, encoding, options):
    """parse an XML file from the filesystem or the network. The
      parsing flags @options are a combination of xmlParserOption. """
    ret = libxml2mod.xmlReaderForFile(filename, encoding, options)
    if ret is None:raise treeError('xmlReaderForFile() failed')
    return xmlTextReader(_obj=ret)

def readerForMemory(buffer, size, URL, encoding, options):
    """Create an xmltextReader for an XML in-memory document. The
      parsing flags @options are a combination of xmlParserOption. """
    ret = libxml2mod.xmlReaderForMemory(buffer, size, URL, encoding, options)
    if ret is None:raise treeError('xmlReaderForMemory() failed')
    return xmlTextReader(_obj=ret)

#
# Functions from module xmlregexp
#

def regexpCompile(regexp):
    """Parses a regular expression conforming to XML Schemas Part
      2 Datatype Appendix F and builds an automata suitable for
       testing strings against that regular expression """
    ret = libxml2mod.xmlRegexpCompile(regexp)
    if ret is None:raise treeError('xmlRegexpCompile() failed')
    return xmlReg(_obj=ret)

#
# Functions from module xmlschemas
#

def schemaNewMemParserCtxt(buffer, size):
    """Create an XML Schemas parse context for that memory buffer
       expected to contain an XML Schemas file. """
    ret = libxml2mod.xmlSchemaNewMemParserCtxt(buffer, size)
    if ret is None:raise parserError('xmlSchemaNewMemParserCtxt() failed')
    return SchemaParserCtxt(_obj=ret)

def schemaNewParserCtxt(URL):
    """Create an XML Schemas parse context for that file/resource
       expected to contain an XML Schemas file. """
    ret = libxml2mod.xmlSchemaNewParserCtxt(URL)
    if ret is None:raise parserError('xmlSchemaNewParserCtxt() failed')
    return SchemaParserCtxt(_obj=ret)

#
# Functions from module xmlschemastypes
#

def schemaCleanupTypes():
    """Cleanup the default XML Schemas type library """
    libxml2mod.xmlSchemaCleanupTypes()

def schemaCollapseString(value):
    """Removes and normalize white spaces in the string """
    ret = libxml2mod.xmlSchemaCollapseString(value)
    return ret

def schemaInitTypes():
    """Initialize the default XML Schemas type library """
    libxml2mod.xmlSchemaInitTypes()

def schemaWhiteSpaceReplace(value):
    """Replaces 0xd, 0x9 and 0xa with a space. """
    ret = libxml2mod.xmlSchemaWhiteSpaceReplace(value)
    return ret

#
# Functions from module xmlstring
#

def UTF8Charcmp(utf1, utf2):
    """compares the two UCS4 values """
    ret = libxml2mod.xmlUTF8Charcmp(utf1, utf2)
    return ret

def UTF8Size(utf):
    """calculates the internal size of a UTF8 character """
    ret = libxml2mod.xmlUTF8Size(utf)
    return ret

def UTF8Strlen(utf):
    """compute the length of an UTF8 string, it doesn't do a full
       UTF8 checking of the content of the string. """
    ret = libxml2mod.xmlUTF8Strlen(utf)
    return ret

def UTF8Strloc(utf, utfchar):
    """a function to provide the relative location of a UTF8 char """
    ret = libxml2mod.xmlUTF8Strloc(utf, utfchar)
    return ret

def UTF8Strndup(utf, len):
    """a strndup for array of UTF8's """
    ret = libxml2mod.xmlUTF8Strndup(utf, len)
    return ret

def UTF8Strpos(utf, pos):
    """a function to provide the equivalent of fetching a
       character from a string array """
    ret = libxml2mod.xmlUTF8Strpos(utf, pos)
    return ret

def UTF8Strsize(utf, len):
    """storage size of an UTF8 string the behaviour is not
       garanteed if the input string is not UTF-8 """
    ret = libxml2mod.xmlUTF8Strsize(utf, len)
    return ret

def UTF8Strsub(utf, start, len):
    """Create a substring from a given UTF-8 string Note:
       positions are given in units of UTF-8 chars """
    ret = libxml2mod.xmlUTF8Strsub(utf, start, len)
    return ret

def checkUTF8(utf):
    """Checks @utf for being valid UTF-8. @utf is assumed to be
      null-terminated. This function is not super-strict, as it
      will allow longer UTF-8 sequences than necessary. Note that
      Java is capable of producing these sequences if provoked.
      Also note, this routine checks for the 4-byte maximum size,
       but does not check for 0x10ffff maximum value. """
    ret = libxml2mod.xmlCheckUTF8(utf)
    return ret

#
# Functions from module xmlunicode
#

def uCSIsAegeanNumbers(code):
    """Check whether the character is part of AegeanNumbers UCS
       Block """
    ret = libxml2mod.xmlUCSIsAegeanNumbers(code)
    return ret

def uCSIsAlphabeticPresentationForms(code):
    """Check whether the character is part of
       AlphabeticPresentationForms UCS Block """
    ret = libxml2mod.xmlUCSIsAlphabeticPresentationForms(code)
    return ret

def uCSIsArabic(code):
    """Check whether the character is part of Arabic UCS Block """
    ret = libxml2mod.xmlUCSIsArabic(code)
    return ret

def uCSIsArabicPresentationFormsA(code):
    """Check whether the character is part of
       ArabicPresentationForms-A UCS Block """
    ret = libxml2mod.xmlUCSIsArabicPresentationFormsA(code)
    return ret

def uCSIsArabicPresentationFormsB(code):
    """Check whether the character is part of
       ArabicPresentationForms-B UCS Block """
    ret = libxml2mod.xmlUCSIsArabicPresentationFormsB(code)
    return ret

def uCSIsArmenian(code):
    """Check whether the character is part of Armenian UCS Block """
    ret = libxml2mod.xmlUCSIsArmenian(code)
    return ret

def uCSIsArrows(code):
    """Check whether the character is part of Arrows UCS Block """
    ret = libxml2mod.xmlUCSIsArrows(code)
    return ret

def uCSIsBasicLatin(code):
    """Check whether the character is part of BasicLatin UCS Block """
    ret = libxml2mod.xmlUCSIsBasicLatin(code)
    return ret

def uCSIsBengali(code):
    """Check whether the character is part of Bengali UCS Block """
    ret = libxml2mod.xmlUCSIsBengali(code)
    return ret

def uCSIsBlock(code, block):
    """Check whether the character is part of the UCS Block """
    ret = libxml2mod.xmlUCSIsBlock(code, block)
    return ret

def uCSIsBlockElements(code):
    """Check whether the character is part of BlockElements UCS
       Block """
    ret = libxml2mod.xmlUCSIsBlockElements(code)
    return ret

def uCSIsBopomofo(code):
    """Check whether the character is part of Bopomofo UCS Block """
    ret = libxml2mod.xmlUCSIsBopomofo(code)
    return ret

def uCSIsBopomofoExtended(code):
    """Check whether the character is part of BopomofoExtended UCS
       Block """
    ret = libxml2mod.xmlUCSIsBopomofoExtended(code)
    return ret

def uCSIsBoxDrawing(code):
    """Check whether the character is part of BoxDrawing UCS Block """
    ret = libxml2mod.xmlUCSIsBoxDrawing(code)
    return ret

def uCSIsBraillePatterns(code):
    """Check whether the character is part of BraillePatterns UCS
       Block """
    ret = libxml2mod.xmlUCSIsBraillePatterns(code)
    return ret

def uCSIsBuhid(code):
    """Check whether the character is part of Buhid UCS Block """
    ret = libxml2mod.xmlUCSIsBuhid(code)
    return ret

def uCSIsByzantineMusicalSymbols(code):
    """Check whether the character is part of
       ByzantineMusicalSymbols UCS Block """
    ret = libxml2mod.xmlUCSIsByzantineMusicalSymbols(code)
    return ret

def uCSIsCJKCompatibility(code):
    """Check whether the character is part of CJKCompatibility UCS
       Block """
    ret = libxml2mod.xmlUCSIsCJKCompatibility(code)
    return ret

def uCSIsCJKCompatibilityForms(code):
    """Check whether the character is part of
       CJKCompatibilityForms UCS Block """
    ret = libxml2mod.xmlUCSIsCJKCompatibilityForms(code)
    return ret

def uCSIsCJKCompatibilityIdeographs(code):
    """Check whether the character is part of
       CJKCompatibilityIdeographs UCS Block """
    ret = libxml2mod.xmlUCSIsCJKCompatibilityIdeographs(code)
    return ret

def uCSIsCJKCompatibilityIdeographsSupplement(code):
    """Check whether the character is part of
       CJKCompatibilityIdeographsSupplement UCS Block """
    ret = libxml2mod.xmlUCSIsCJKCompatibilityIdeographsSupplement(code)
    return ret

def uCSIsCJKRadicalsSupplement(code):
    """Check whether the character is part of
       CJKRadicalsSupplement UCS Block """
    ret = libxml2mod.xmlUCSIsCJKRadicalsSupplement(code)
    return ret

def uCSIsCJKSymbolsandPunctuation(code):
    """Check whether the character is part of
       CJKSymbolsandPunctuation UCS Block """
    ret = libxml2mod.xmlUCSIsCJKSymbolsandPunctuation(code)
    return ret

def uCSIsCJKUnifiedIdeographs(code):
    """Check whether the character is part of CJKUnifiedIdeographs
       UCS Block """
    ret = libxml2mod.xmlUCSIsCJKUnifiedIdeographs(code)
    return ret

def uCSIsCJKUnifiedIdeographsExtensionA(code):
    """Check whether the character is part of
       CJKUnifiedIdeographsExtensionA UCS Block """
    ret = libxml2mod.xmlUCSIsCJKUnifiedIdeographsExtensionA(code)
    return ret

def uCSIsCJKUnifiedIdeographsExtensionB(code):
    """Check whether the character is part of
       CJKUnifiedIdeographsExtensionB UCS Block """
    ret = libxml2mod.xmlUCSIsCJKUnifiedIdeographsExtensionB(code)
    return ret

def uCSIsCat(code, cat):
    """Check whether the character is part of the UCS Category """
    ret = libxml2mod.xmlUCSIsCat(code, cat)
    return ret

def uCSIsCatC(code):
    """Check whether the character is part of C UCS Category """
    ret = libxml2mod.xmlUCSIsCatC(code)
    return ret

def uCSIsCatCc(code):
    """Check whether the character is part of Cc UCS Category """
    ret = libxml2mod.xmlUCSIsCatCc(code)
    return ret

def uCSIsCatCf(code):
    """Check whether the character is part of Cf UCS Category """
    ret = libxml2mod.xmlUCSIsCatCf(code)
    return ret

def uCSIsCatCo(code):
    """Check whether the character is part of Co UCS Category """
    ret = libxml2mod.xmlUCSIsCatCo(code)
    return ret

def uCSIsCatCs(code):
    """Check whether the character is part of Cs UCS Category """
    ret = libxml2mod.xmlUCSIsCatCs(code)
    return ret

def uCSIsCatL(code):
    """Check whether the character is part of L UCS Category """
    ret = libxml2mod.xmlUCSIsCatL(code)
    return ret

def uCSIsCatLl(code):
    """Check whether the character is part of Ll UCS Category """
    ret = libxml2mod.xmlUCSIsCatLl(code)
    return ret

def uCSIsCatLm(code):
    """Check whether the character is part of Lm UCS Category """
    ret = libxml2mod.xmlUCSIsCatLm(code)
    return ret

def uCSIsCatLo(code):
    """Check whether the character is part of Lo UCS Category """
    ret = libxml2mod.xmlUCSIsCatLo(code)
    return ret

def uCSIsCatLt(code):
    """Check whether the character is part of Lt UCS Category """
    ret = libxml2mod.xmlUCSIsCatLt(code)
    return ret

def uCSIsCatLu(code):
    """Check whether the character is part of Lu UCS Category """
    ret = libxml2mod.xmlUCSIsCatLu(code)
    return ret

def uCSIsCatM(code):
    """Check whether the character is part of M UCS Category """
    ret = libxml2mod.xmlUCSIsCatM(code)
    return ret

def uCSIsCatMc(code):
    """Check whether the character is part of Mc UCS Category """
    ret = libxml2mod.xmlUCSIsCatMc(code)
    return ret

def uCSIsCatMe(code):
    """Check whether the character is part of Me UCS Category """
    ret = libxml2mod.xmlUCSIsCatMe(code)
    return ret

def uCSIsCatMn(code):
    """Check whether the character is part of Mn UCS Category """
    ret = libxml2mod.xmlUCSIsCatMn(code)
    return ret

def uCSIsCatN(code):
    """Check whether the character is part of N UCS Category """
    ret = libxml2mod.xmlUCSIsCatN(code)
    return ret

def uCSIsCatNd(code):
    """Check whether the character is part of Nd UCS Category """
    ret = libxml2mod.xmlUCSIsCatNd(code)
    return ret

def uCSIsCatNl(code):
    """Check whether the character is part of Nl UCS Category """
    ret = libxml2mod.xmlUCSIsCatNl(code)
    return ret

def uCSIsCatNo(code):
    """Check whether the character is part of No UCS Category """
    ret = libxml2mod.xmlUCSIsCatNo(code)
    return ret

def uCSIsCatP(code):
    """Check whether the character is part of P UCS Category """
    ret = libxml2mod.xmlUCSIsCatP(code)
    return ret

def uCSIsCatPc(code):
    """Check whether the character is part of Pc UCS Category """
    ret = libxml2mod.xmlUCSIsCatPc(code)
    return ret

def uCSIsCatPd(code):
    """Check whether the character is part of Pd UCS Category """
    ret = libxml2mod.xmlUCSIsCatPd(code)
    return ret

def uCSIsCatPe(code):
    """Check whether the character is part of Pe UCS Category """
    ret = libxml2mod.xmlUCSIsCatPe(code)
    return ret

def uCSIsCatPf(code):
    """Check whether the character is part of Pf UCS Category """
    ret = libxml2mod.xmlUCSIsCatPf(code)
    return ret

def uCSIsCatPi(code):
    """Check whether the character is part of Pi UCS Category """
    ret = libxml2mod.xmlUCSIsCatPi(code)
    return ret

def uCSIsCatPo(code):
    """Check whether the character is part of Po UCS Category """
    ret = libxml2mod.xmlUCSIsCatPo(code)
    return ret

def uCSIsCatPs(code):
    """Check whether the character is part of Ps UCS Category """
    ret = libxml2mod.xmlUCSIsCatPs(code)
    return ret

def uCSIsCatS(code):
    """Check whether the character is part of S UCS Category """
    ret = libxml2mod.xmlUCSIsCatS(code)
    return ret

def uCSIsCatSc(code):
    """Check whether the character is part of Sc UCS Category """
    ret = libxml2mod.xmlUCSIsCatSc(code)
    return ret

def uCSIsCatSk(code):
    """Check whether the character is part of Sk UCS Category """
    ret = libxml2mod.xmlUCSIsCatSk(code)
    return ret

def uCSIsCatSm(code):
    """Check whether the character is part of Sm UCS Category """
    ret = libxml2mod.xmlUCSIsCatSm(code)
    return ret

def uCSIsCatSo(code):
    """Check whether the character is part of So UCS Category """
    ret = libxml2mod.xmlUCSIsCatSo(code)
    return ret

def uCSIsCatZ(code):
    """Check whether the character is part of Z UCS Category """
    ret = libxml2mod.xmlUCSIsCatZ(code)
    return ret

def uCSIsCatZl(code):
    """Check whether the character is part of Zl UCS Category """
    ret = libxml2mod.xmlUCSIsCatZl(code)
    return ret

def uCSIsCatZp(code):
    """Check whether the character is part of Zp UCS Category """
    ret = libxml2mod.xmlUCSIsCatZp(code)
    return ret

def uCSIsCatZs(code):
    """Check whether the character is part of Zs UCS Category """
    ret = libxml2mod.xmlUCSIsCatZs(code)
    return ret

def uCSIsCherokee(code):
    """Check whether the character is part of Cherokee UCS Block """
    ret = libxml2mod.xmlUCSIsCherokee(code)
    return ret

def uCSIsCombiningDiacriticalMarks(code):
    """Check whether the character is part of
       CombiningDiacriticalMarks UCS Block """
    ret = libxml2mod.xmlUCSIsCombiningDiacriticalMarks(code)
    return ret

def uCSIsCombiningDiacriticalMarksforSymbols(code):
    """Check whether the character is part of
       CombiningDiacriticalMarksforSymbols UCS Block """
    ret = libxml2mod.xmlUCSIsCombiningDiacriticalMarksforSymbols(code)
    return ret

def uCSIsCombiningHalfMarks(code):
    """Check whether the character is part of CombiningHalfMarks
       UCS Block """
    ret = libxml2mod.xmlUCSIsCombiningHalfMarks(code)
    return ret

def uCSIsCombiningMarksforSymbols(code):
    """Check whether the character is part of
       CombiningMarksforSymbols UCS Block """
    ret = libxml2mod.xmlUCSIsCombiningMarksforSymbols(code)
    return ret

def uCSIsControlPictures(code):
    """Check whether the character is part of ControlPictures UCS
       Block """
    ret = libxml2mod.xmlUCSIsControlPictures(code)
    return ret

def uCSIsCurrencySymbols(code):
    """Check whether the character is part of CurrencySymbols UCS
       Block """
    ret = libxml2mod.xmlUCSIsCurrencySymbols(code)
    return ret

def uCSIsCypriotSyllabary(code):
    """Check whether the character is part of CypriotSyllabary UCS
       Block """
    ret = libxml2mod.xmlUCSIsCypriotSyllabary(code)
    return ret

def uCSIsCyrillic(code):
    """Check whether the character is part of Cyrillic UCS Block """
    ret = libxml2mod.xmlUCSIsCyrillic(code)
    return ret

def uCSIsCyrillicSupplement(code):
    """Check whether the character is part of CyrillicSupplement
       UCS Block """
    ret = libxml2mod.xmlUCSIsCyrillicSupplement(code)
    return ret

def uCSIsDeseret(code):
    """Check whether the character is part of Deseret UCS Block """
    ret = libxml2mod.xmlUCSIsDeseret(code)
    return ret

def uCSIsDevanagari(code):
    """Check whether the character is part of Devanagari UCS Block """
    ret = libxml2mod.xmlUCSIsDevanagari(code)
    return ret

def uCSIsDingbats(code):
    """Check whether the character is part of Dingbats UCS Block """
    ret = libxml2mod.xmlUCSIsDingbats(code)
    return ret

def uCSIsEnclosedAlphanumerics(code):
    """Check whether the character is part of
       EnclosedAlphanumerics UCS Block """
    ret = libxml2mod.xmlUCSIsEnclosedAlphanumerics(code)
    return ret

def uCSIsEnclosedCJKLettersandMonths(code):
    """Check whether the character is part of
       EnclosedCJKLettersandMonths UCS Block """
    ret = libxml2mod.xmlUCSIsEnclosedCJKLettersandMonths(code)
    return ret

def uCSIsEthiopic(code):
    """Check whether the character is part of Ethiopic UCS Block """
    ret = libxml2mod.xmlUCSIsEthiopic(code)
    return ret

def uCSIsGeneralPunctuation(code):
    """Check whether the character is part of GeneralPunctuation
       UCS Block """
    ret = libxml2mod.xmlUCSIsGeneralPunctuation(code)
    return ret

def uCSIsGeometricShapes(code):
    """Check whether the character is part of GeometricShapes UCS
       Block """
    ret = libxml2mod.xmlUCSIsGeometricShapes(code)
    return ret

def uCSIsGeorgian(code):
    """Check whether the character is part of Georgian UCS Block """
    ret = libxml2mod.xmlUCSIsGeorgian(code)
    return ret

def uCSIsGothic(code):
    """Check whether the character is part of Gothic UCS Block """
    ret = libxml2mod.xmlUCSIsGothic(code)
    return ret

def uCSIsGreek(code):
    """Check whether the character is part of Greek UCS Block """
    ret = libxml2mod.xmlUCSIsGreek(code)
    return ret

def uCSIsGreekExtended(code):
    """Check whether the character is part of GreekExtended UCS
       Block """
    ret = libxml2mod.xmlUCSIsGreekExtended(code)
    return ret

def uCSIsGreekandCoptic(code):
    """Check whether the character is part of GreekandCoptic UCS
       Block """
    ret = libxml2mod.xmlUCSIsGreekandCoptic(code)
    return ret

def uCSIsGujarati(code):
    """Check whether the character is part of Gujarati UCS Block """
    ret = libxml2mod.xmlUCSIsGujarati(code)
    return ret

def uCSIsGurmukhi(code):
    """Check whether the character is part of Gurmukhi UCS Block """
    ret = libxml2mod.xmlUCSIsGurmukhi(code)
    return ret

def uCSIsHalfwidthandFullwidthForms(code):
    """Check whether the character is part of
       HalfwidthandFullwidthForms UCS Block """
    ret = libxml2mod.xmlUCSIsHalfwidthandFullwidthForms(code)
    return ret

def uCSIsHangulCompatibilityJamo(code):
    """Check whether the character is part of
       HangulCompatibilityJamo UCS Block """
    ret = libxml2mod.xmlUCSIsHangulCompatibilityJamo(code)
    return ret

def uCSIsHangulJamo(code):
    """Check whether the character is part of HangulJamo UCS Block """
    ret = libxml2mod.xmlUCSIsHangulJamo(code)
    return ret

def uCSIsHangulSyllables(code):
    """Check whether the character is part of HangulSyllables UCS
       Block """
    ret = libxml2mod.xmlUCSIsHangulSyllables(code)
    return ret

def uCSIsHanunoo(code):
    """Check whether the character is part of Hanunoo UCS Block """
    ret = libxml2mod.xmlUCSIsHanunoo(code)
    return ret

def uCSIsHebrew(code):
    """Check whether the character is part of Hebrew UCS Block """
    ret = libxml2mod.xmlUCSIsHebrew(code)
    return ret

def uCSIsHighPrivateUseSurrogates(code):
    """Check whether the character is part of
       HighPrivateUseSurrogates UCS Block """
    ret = libxml2mod.xmlUCSIsHighPrivateUseSurrogates(code)
    return ret

def uCSIsHighSurrogates(code):
    """Check whether the character is part of HighSurrogates UCS
       Block """
    ret = libxml2mod.xmlUCSIsHighSurrogates(code)
    return ret

def uCSIsHiragana(code):
    """Check whether the character is part of Hiragana UCS Block """
    ret = libxml2mod.xmlUCSIsHiragana(code)
    return ret

def uCSIsIPAExtensions(code):
    """Check whether the character is part of IPAExtensions UCS
       Block """
    ret = libxml2mod.xmlUCSIsIPAExtensions(code)
    return ret

def uCSIsIdeographicDescriptionCharacters(code):
    """Check whether the character is part of
       IdeographicDescriptionCharacters UCS Block """
    ret = libxml2mod.xmlUCSIsIdeographicDescriptionCharacters(code)
    return ret

def uCSIsKanbun(code):
    """Check whether the character is part of Kanbun UCS Block """
    ret = libxml2mod.xmlUCSIsKanbun(code)
    return ret

def uCSIsKangxiRadicals(code):
    """Check whether the character is part of KangxiRadicals UCS
       Block """
    ret = libxml2mod.xmlUCSIsKangxiRadicals(code)
    return ret

def uCSIsKannada(code):
    """Check whether the character is part of Kannada UCS Block """
    ret = libxml2mod.xmlUCSIsKannada(code)
    return ret

def uCSIsKatakana(code):
    """Check whether the character is part of Katakana UCS Block """
    ret = libxml2mod.xmlUCSIsKatakana(code)
    return ret

def uCSIsKatakanaPhoneticExtensions(code):
    """Check whether the character is part of
       KatakanaPhoneticExtensions UCS Block """
    ret = libxml2mod.xmlUCSIsKatakanaPhoneticExtensions(code)
    return ret

def uCSIsKhmer(code):
    """Check whether the character is part of Khmer UCS Block """
    ret = libxml2mod.xmlUCSIsKhmer(code)
    return ret

def uCSIsKhmerSymbols(code):
    """Check whether the character is part of KhmerSymbols UCS
       Block """
    ret = libxml2mod.xmlUCSIsKhmerSymbols(code)
    return ret

def uCSIsLao(code):
    """Check whether the character is part of Lao UCS Block """
    ret = libxml2mod.xmlUCSIsLao(code)
    return ret

def uCSIsLatin1Supplement(code):
    """Check whether the character is part of Latin-1Supplement
       UCS Block """
    ret = libxml2mod.xmlUCSIsLatin1Supplement(code)
    return ret

def uCSIsLatinExtendedA(code):
    """Check whether the character is part of LatinExtended-A UCS
       Block """
    ret = libxml2mod.xmlUCSIsLatinExtendedA(code)
    return ret

def uCSIsLatinExtendedAdditional(code):
    """Check whether the character is part of
       LatinExtendedAdditional UCS Block """
    ret = libxml2mod.xmlUCSIsLatinExtendedAdditional(code)
    return ret

def uCSIsLatinExtendedB(code):
    """Check whether the character is part of LatinExtended-B UCS
       Block """
    ret = libxml2mod.xmlUCSIsLatinExtendedB(code)
    return ret

def uCSIsLetterlikeSymbols(code):
    """Check whether the character is part of LetterlikeSymbols
       UCS Block """
    ret = libxml2mod.xmlUCSIsLetterlikeSymbols(code)
    return ret

def uCSIsLimbu(code):
    """Check whether the character is part of Limbu UCS Block """
    ret = libxml2mod.xmlUCSIsLimbu(code)
    return ret

def uCSIsLinearBIdeograms(code):
    """Check whether the character is part of LinearBIdeograms UCS
       Block """
    ret = libxml2mod.xmlUCSIsLinearBIdeograms(code)
    return ret

def uCSIsLinearBSyllabary(code):
    """Check whether the character is part of LinearBSyllabary UCS
       Block """
    ret = libxml2mod.xmlUCSIsLinearBSyllabary(code)
    return ret

def uCSIsLowSurrogates(code):
    """Check whether the character is part of LowSurrogates UCS
       Block """
    ret = libxml2mod.xmlUCSIsLowSurrogates(code)
    return ret

def uCSIsMalayalam(code):
    """Check whether the character is part of Malayalam UCS Block """
    ret = libxml2mod.xmlUCSIsMalayalam(code)
    return ret

def uCSIsMathematicalAlphanumericSymbols(code):
    """Check whether the character is part of
       MathematicalAlphanumericSymbols UCS Block """
    ret = libxml2mod.xmlUCSIsMathematicalAlphanumericSymbols(code)
    return ret

def uCSIsMathematicalOperators(code):
    """Check whether the character is part of
       MathematicalOperators UCS Block """
    ret = libxml2mod.xmlUCSIsMathematicalOperators(code)
    return ret

def uCSIsMiscellaneousMathematicalSymbolsA(code):
    """Check whether the character is part of
       MiscellaneousMathematicalSymbols-A UCS Block """
    ret = libxml2mod.xmlUCSIsMiscellaneousMathematicalSymbolsA(code)
    return ret

def uCSIsMiscellaneousMathematicalSymbolsB(code):
    """Check whether the character is part of
       MiscellaneousMathematicalSymbols-B UCS Block """
    ret = libxml2mod.xmlUCSIsMiscellaneousMathematicalSymbolsB(code)
    return ret

def uCSIsMiscellaneousSymbols(code):
    """Check whether the character is part of MiscellaneousSymbols
       UCS Block """
    ret = libxml2mod.xmlUCSIsMiscellaneousSymbols(code)
    return ret

def uCSIsMiscellaneousSymbolsandArrows(code):
    """Check whether the character is part of
       MiscellaneousSymbolsandArrows UCS Block """
    ret = libxml2mod.xmlUCSIsMiscellaneousSymbolsandArrows(code)
    return ret

def uCSIsMiscellaneousTechnical(code):
    """Check whether the character is part of
       MiscellaneousTechnical UCS Block """
    ret = libxml2mod.xmlUCSIsMiscellaneousTechnical(code)
    return ret

def uCSIsMongolian(code):
    """Check whether the character is part of Mongolian UCS Block """
    ret = libxml2mod.xmlUCSIsMongolian(code)
    return ret

def uCSIsMusicalSymbols(code):
    """Check whether the character is part of MusicalSymbols UCS
       Block """
    ret = libxml2mod.xmlUCSIsMusicalSymbols(code)
    return ret

def uCSIsMyanmar(code):
    """Check whether the character is part of Myanmar UCS Block """
    ret = libxml2mod.xmlUCSIsMyanmar(code)
    return ret

def uCSIsNumberForms(code):
    """Check whether the character is part of NumberForms UCS Block """
    ret = libxml2mod.xmlUCSIsNumberForms(code)
    return ret

def uCSIsOgham(code):
    """Check whether the character is part of Ogham UCS Block """
    ret = libxml2mod.xmlUCSIsOgham(code)
    return ret

def uCSIsOldItalic(code):
    """Check whether the character is part of OldItalic UCS Block """
    ret = libxml2mod.xmlUCSIsOldItalic(code)
    return ret

def uCSIsOpticalCharacterRecognition(code):
    """Check whether the character is part of
       OpticalCharacterRecognition UCS Block """
    ret = libxml2mod.xmlUCSIsOpticalCharacterRecognition(code)
    return ret

def uCSIsOriya(code):
    """Check whether the character is part of Oriya UCS Block """
    ret = libxml2mod.xmlUCSIsOriya(code)
    return ret

def uCSIsOsmanya(code):
    """Check whether the character is part of Osmanya UCS Block """
    ret = libxml2mod.xmlUCSIsOsmanya(code)
    return ret

def uCSIsPhoneticExtensions(code):
    """Check whether the character is part of PhoneticExtensions
       UCS Block """
    ret = libxml2mod.xmlUCSIsPhoneticExtensions(code)
    return ret

def uCSIsPrivateUse(code):
    """Check whether the character is part of PrivateUse UCS Block """
    ret = libxml2mod.xmlUCSIsPrivateUse(code)
    return ret

def uCSIsPrivateUseArea(code):
    """Check whether the character is part of PrivateUseArea UCS
       Block """
    ret = libxml2mod.xmlUCSIsPrivateUseArea(code)
    return ret

def uCSIsRunic(code):
    """Check whether the character is part of Runic UCS Block """
    ret = libxml2mod.xmlUCSIsRunic(code)
    return ret

def uCSIsShavian(code):
    """Check whether the character is part of Shavian UCS Block """
    ret = libxml2mod.xmlUCSIsShavian(code)
    return ret

def uCSIsSinhala(code):
    """Check whether the character is part of Sinhala UCS Block """
    ret = libxml2mod.xmlUCSIsSinhala(code)
    return ret

def uCSIsSmallFormVariants(code):
    """Check whether the character is part of SmallFormVariants
       UCS Block """
    ret = libxml2mod.xmlUCSIsSmallFormVariants(code)
    return ret

def uCSIsSpacingModifierLetters(code):
    """Check whether the character is part of
       SpacingModifierLetters UCS Block """
    ret = libxml2mod.xmlUCSIsSpacingModifierLetters(code)
    return ret

def uCSIsSpecials(code):
    """Check whether the character is part of Specials UCS Block """
    ret = libxml2mod.xmlUCSIsSpecials(code)
    return ret

def uCSIsSuperscriptsandSubscripts(code):
    """Check whether the character is part of
       SuperscriptsandSubscripts UCS Block """
    ret = libxml2mod.xmlUCSIsSuperscriptsandSubscripts(code)
    return ret

def uCSIsSupplementalArrowsA(code):
    """Check whether the character is part of SupplementalArrows-A
       UCS Block """
    ret = libxml2mod.xmlUCSIsSupplementalArrowsA(code)
    return ret

def uCSIsSupplementalArrowsB(code):
    """Check whether the character is part of SupplementalArrows-B
       UCS Block """
    ret = libxml2mod.xmlUCSIsSupplementalArrowsB(code)
    return ret

def uCSIsSupplementalMathematicalOperators(code):
    """Check whether the character is part of
       SupplementalMathematicalOperators UCS Block """
    ret = libxml2mod.xmlUCSIsSupplementalMathematicalOperators(code)
    return ret

def uCSIsSupplementaryPrivateUseAreaA(code):
    """Check whether the character is part of
       SupplementaryPrivateUseArea-A UCS Block """
    ret = libxml2mod.xmlUCSIsSupplementaryPrivateUseAreaA(code)
    return ret

def uCSIsSupplementaryPrivateUseAreaB(code):
    """Check whether the character is part of
       SupplementaryPrivateUseArea-B UCS Block """
    ret = libxml2mod.xmlUCSIsSupplementaryPrivateUseAreaB(code)
    return ret

def uCSIsSyriac(code):
    """Check whether the character is part of Syriac UCS Block """
    ret = libxml2mod.xmlUCSIsSyriac(code)
    return ret

def uCSIsTagalog(code):
    """Check whether the character is part of Tagalog UCS Block """
    ret = libxml2mod.xmlUCSIsTagalog(code)
    return ret

def uCSIsTagbanwa(code):
    """Check whether the character is part of Tagbanwa UCS Block """
    ret = libxml2mod.xmlUCSIsTagbanwa(code)
    return ret

def uCSIsTags(code):
    """Check whether the character is part of Tags UCS Block """
    ret = libxml2mod.xmlUCSIsTags(code)
    return ret

def uCSIsTaiLe(code):
    """Check whether the character is part of TaiLe UCS Block """
    ret = libxml2mod.xmlUCSIsTaiLe(code)
    return ret

def uCSIsTaiXuanJingSymbols(code):
    """Check whether the character is part of TaiXuanJingSymbols
       UCS Block """
    ret = libxml2mod.xmlUCSIsTaiXuanJingSymbols(code)
    return ret

def uCSIsTamil(code):
    """Check whether the character is part of Tamil UCS Block """
    ret = libxml2mod.xmlUCSIsTamil(code)
    return ret

def uCSIsTelugu(code):
    """Check whether the character is part of Telugu UCS Block """
    ret = libxml2mod.xmlUCSIsTelugu(code)
    return ret

def uCSIsThaana(code):
    """Check whether the character is part of Thaana UCS Block """
    ret = libxml2mod.xmlUCSIsThaana(code)
    return ret

def uCSIsThai(code):
    """Check whether the character is part of Thai UCS Block """
    ret = libxml2mod.xmlUCSIsThai(code)
    return ret

def uCSIsTibetan(code):
    """Check whether the character is part of Tibetan UCS Block """
    ret = libxml2mod.xmlUCSIsTibetan(code)
    return ret

def uCSIsUgaritic(code):
    """Check whether the character is part of Ugaritic UCS Block """
    ret = libxml2mod.xmlUCSIsUgaritic(code)
    return ret

def uCSIsUnifiedCanadianAboriginalSyllabics(code):
    """Check whether the character is part of
       UnifiedCanadianAboriginalSyllabics UCS Block """
    ret = libxml2mod.xmlUCSIsUnifiedCanadianAboriginalSyllabics(code)
    return ret

def uCSIsVariationSelectors(code):
    """Check whether the character is part of VariationSelectors
       UCS Block """
    ret = libxml2mod.xmlUCSIsVariationSelectors(code)
    return ret

def uCSIsVariationSelectorsSupplement(code):
    """Check whether the character is part of
       VariationSelectorsSupplement UCS Block """
    ret = libxml2mod.xmlUCSIsVariationSelectorsSupplement(code)
    return ret

def uCSIsYiRadicals(code):
    """Check whether the character is part of YiRadicals UCS Block """
    ret = libxml2mod.xmlUCSIsYiRadicals(code)
    return ret

def uCSIsYiSyllables(code):
    """Check whether the character is part of YiSyllables UCS Block """
    ret = libxml2mod.xmlUCSIsYiSyllables(code)
    return ret

def uCSIsYijingHexagramSymbols(code):
    """Check whether the character is part of
       YijingHexagramSymbols UCS Block """
    ret = libxml2mod.xmlUCSIsYijingHexagramSymbols(code)
    return ret

#
# Functions from module xmlversion
#

def checkVersion(version):
    """check the compiled lib version against the include one.
       This can warn or immediately kill the application """
    libxml2mod.xmlCheckVersion(version)

#
# Functions from module xpathInternals
#

def valuePop(ctxt):
    """Pops the top XPath object from the value stack """
    if ctxt is None: ctxt__o = None
    else: ctxt__o = ctxt._o
    ret = libxml2mod.valuePop(ctxt__o)
    return ret

class xmlNode(xmlCore):
    def __init__(self, _obj=None):
        if checkWrapper(_obj) != 0:            raise TypeError('xmlNode got a wrong wrapper object type')
        self._o = _obj
        xmlCore.__init__(self, _obj=_obj)

    def __repr__(self):
        return "<xmlNode (%s) object at 0x%x>" % (self.name, int(pos_id (self)))

    # accessors for xmlNode
    def ns(self):
        """Get the namespace of a node """
        ret = libxml2mod.xmlNodeGetNs(self._o)
        if ret is None:return None
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def nsDefs(self):
        """Get the namespace of a node """
        ret = libxml2mod.xmlNodeGetNsDefs(self._o)
        if ret is None:return None
        __tmp = xmlNs(_obj=ret)
        return __tmp

    #
    # xmlNode functions from module debugXML
    #

    def debugDumpNode(self, output, depth):
        """Dumps debug information for the element node, it is
           recursive """
        libxml2mod.xmlDebugDumpNode(output, self._o, depth)

    def debugDumpNodeList(self, output, depth):
        """Dumps debug information for the list of element node, it is
           recursive """
        libxml2mod.xmlDebugDumpNodeList(output, self._o, depth)

    def debugDumpOneNode(self, output, depth):
        """Dumps debug information for the element node, it is not
           recursive """
        libxml2mod.xmlDebugDumpOneNode(output, self._o, depth)

    def lsCountNode(self):
        """Count the children of @node. """
        ret = libxml2mod.xmlLsCountNode(self._o)
        return ret

    def lsOneNode(self, output):
        """Dump to @output the type and name of @node. """
        libxml2mod.xmlLsOneNode(output, self._o)

    def shellPrintNode(self):
        """Print node to the output FILE """
        libxml2mod.xmlShellPrintNode(self._o)

    #
    # xmlNode functions from module tree
    #

    def addChild(self, cur):
        """Add a new node to @parent, at the end of the child (or
          property) list merging adjacent TEXT nodes (in which case
          @cur is freed) If the new node is ATTRIBUTE, it is added
          into properties instead of children. If there is an
           attribute with equal name, it is first destroyed. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlAddChild(self._o, cur__o)
        if ret is None:raise treeError('xmlAddChild() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def addChildList(self, cur):
        """Add a list of node at the end of the child list of the
           parent merging adjacent TEXT nodes (@cur may be freed) """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlAddChildList(self._o, cur__o)
        if ret is None:raise treeError('xmlAddChildList() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def addContent(self, content):
        """Append the extra substring to the node content. NOTE: In
          contrast to xmlNodeSetContent(), @content is supposed to be
          raw text, so unescaped XML special chars are allowed,
           entity references are not supported. """
        libxml2mod.xmlNodeAddContent(self._o, content)

    def addContentLen(self, content, len):
        """Append the extra substring to the node content. NOTE: In
          contrast to xmlNodeSetContentLen(), @content is supposed to
          be raw text, so unescaped XML special chars are allowed,
           entity references are not supported. """
        libxml2mod.xmlNodeAddContentLen(self._o, content, len)

    def addNextSibling(self, elem):
        """Add a new node @elem as the next sibling of @cur If the new
          node was already inserted in a document it is first
          unlinked from its existing context. As a result of text
          merging @elem may be freed. If the new node is ATTRIBUTE,
          it is added into properties instead of children. If there
           is an attribute with equal name, it is first destroyed. """
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlAddNextSibling(self._o, elem__o)
        if ret is None:raise treeError('xmlAddNextSibling() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def addPrevSibling(self, elem):
        """Add a new node @elem as the previous sibling of @cur
          merging adjacent TEXT nodes (@elem may be freed) If the new
          node was already inserted in a document it is first
          unlinked from its existing context. If the new node is
          ATTRIBUTE, it is added into properties instead of children.
          If there is an attribute with equal name, it is first
           destroyed. """
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlAddPrevSibling(self._o, elem__o)
        if ret is None:raise treeError('xmlAddPrevSibling() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def addSibling(self, elem):
        """Add a new element @elem to the list of siblings of @cur
          merging adjacent TEXT nodes (@elem may be freed) If the new
          element was already inserted in a document it is first
           unlinked from its existing context. """
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlAddSibling(self._o, elem__o)
        if ret is None:raise treeError('xmlAddSibling() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def copyNode(self, extended):
        """Do a copy of the node. """
        ret = libxml2mod.xmlCopyNode(self._o, extended)
        if ret is None:raise treeError('xmlCopyNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def copyNodeList(self):
        """Do a recursive copy of the node list. Use
          xmlDocCopyNodeList() if possible to ensure string interning. """
        ret = libxml2mod.xmlCopyNodeList(self._o)
        if ret is None:raise treeError('xmlCopyNodeList() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def copyProp(self, cur):
        """Do a copy of the attribute. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlCopyProp(self._o, cur__o)
        if ret is None:raise treeError('xmlCopyProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def copyPropList(self, cur):
        """Do a copy of an attribute list. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlCopyPropList(self._o, cur__o)
        if ret is None:raise treeError('xmlCopyPropList() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def docCopyNode(self, doc, extended):
        """Do a copy of the node to a given document. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlDocCopyNode(self._o, doc__o, extended)
        if ret is None:raise treeError('xmlDocCopyNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def docCopyNodeList(self, doc):
        """Do a recursive copy of the node list. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlDocCopyNodeList(doc__o, self._o)
        if ret is None:raise treeError('xmlDocCopyNodeList() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def docSetRootElement(self, doc):
        """Set the root element of the document (doc->children is a
           list containing possibly comments, PIs, etc ...). """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlDocSetRootElement(doc__o, self._o)
        if ret is None:return None
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def firstElementChild(self):
        """Finds the first child node of that element which is a
          Element node Note the handling of entities references is
          different than in the W3C DOM element traversal spec since
          we don't have back reference from entities content to
           entities references. """
        ret = libxml2mod.xmlFirstElementChild(self._o)
        if ret is None:return None
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def freeNode(self):
        """Free a node, this is a recursive behaviour, all the
          children are freed too. This doesn't unlink the child from
           the list, use xmlUnlinkNode() first. """
        libxml2mod.xmlFreeNode(self._o)

    def freeNodeList(self):
        """Free a node and all its siblings, this is a recursive
           behaviour, all the children are freed too. """
        libxml2mod.xmlFreeNodeList(self._o)

    def getBase(self, doc):
        """Searches for the BASE URL. The code should work on both XML
          and HTML document even if base mechanisms are completely
          different. It returns the base as defined in RFC 2396
          sections 5.1.1. Base URI within Document Content and 5.1.2.
          Base URI from the Encapsulating Entity However it does not
           return the document base (5.1.3), use doc->URL in this case """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlNodeGetBase(doc__o, self._o)
        return ret

    def getContent(self):
        """Read the value of a node, this can be either the text
          carried directly by this node if it's a TEXT node or the
          aggregate string of the values carried by this node child's
           (TEXT and ENTITY_REF). Entity references are substituted. """
        ret = libxml2mod.xmlNodeGetContent(self._o)
        return ret

    def getLang(self):
        """Searches the language of a node, i.e. the values of the
          xml:lang attribute or the one carried by the nearest
           ancestor. """
        ret = libxml2mod.xmlNodeGetLang(self._o)
        return ret

    def getSpacePreserve(self):
        """Searches the space preserving behaviour of a node, i.e. the
          values of the xml:space attribute or the one carried by the
           nearest ancestor. """
        ret = libxml2mod.xmlNodeGetSpacePreserve(self._o)
        return ret

    def hasNsProp(self, name, nameSpace):
        """Search for an attribute associated to a node This attribute
          has to be anchored in the namespace specified. This does
          the entity substitution. This function looks in DTD
          attribute declaration for #FIXED or default declaration
          values unless DTD use has been turned off. Note that a
           namespace of None indicates to use the default namespace. """
        ret = libxml2mod.xmlHasNsProp(self._o, name, nameSpace)
        if ret is None:return None
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def hasProp(self, name):
        """Search an attribute associated to a node This function also
          looks in DTD attribute declaration for #FIXED or default
           declaration values unless DTD use has been turned off. """
        ret = libxml2mod.xmlHasProp(self._o, name)
        if ret is None:return None
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def isBlankNode(self):
        """Checks whether this node is an empty or whitespace only
           (and possibly ignorable) text-node. """
        ret = libxml2mod.xmlIsBlankNode(self._o)
        return ret

    def isText(self):
        """Is this node a Text node ? """
        ret = libxml2mod.xmlNodeIsText(self._o)
        return ret

    def lastChild(self):
        """Search the last child of a node. """
        ret = libxml2mod.xmlGetLastChild(self._o)
        if ret is None:raise treeError('xmlGetLastChild() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def lastElementChild(self):
        """Finds the last child node of that element which is a
          Element node Note the handling of entities references is
          different than in the W3C DOM element traversal spec since
          we don't have back reference from entities content to
           entities references. """
        ret = libxml2mod.xmlLastElementChild(self._o)
        if ret is None:return None
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def lineNo(self):
        """Get line number of @node. Try to override the limitation of
          lines being store in 16 bits ints if XML_PARSE_BIG_LINES
           parser option was used """
        ret = libxml2mod.xmlGetLineNo(self._o)
        return ret

    def listGetRawString(self, doc, inLine):
        """Builds the string equivalent to the text contained in the
          Node list made of TEXTs and ENTITY_REFs, contrary to
          xmlNodeListGetString() this function doesn't do any
           character encoding handling. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlNodeListGetRawString(doc__o, self._o, inLine)
        return ret

    def listGetString(self, doc, inLine):
        """Build the string equivalent to the text contained in the
           Node list made of TEXTs and ENTITY_REFs """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlNodeListGetString(doc__o, self._o, inLine)
        return ret

    def newChild(self, ns, name, content):
        """Creation of a new child element, added at the end of
          @parent children list. @ns and @content parameters are
          optional (None). If @ns is None, the newly created element
          inherits the namespace of @parent. If @content is non None,
          a child list containing the TEXTs and ENTITY_REFs node will
          be created. NOTE: @content is supposed to be a piece of XML
          CDATA, so it allows entity references. XML special chars
          must be escaped first by using
          xmlEncodeEntitiesReentrant(), or xmlNewTextChild() should
           be used. """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlNewChild(self._o, ns__o, name, content)
        if ret is None:raise treeError('xmlNewChild() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newNs(self, href, prefix):
        """Creation of a new Namespace. This function will refuse to
          create a namespace with a similar prefix than an existing
          one present on this node. We use href==None in the case of
           an element creation where the namespace was not defined. """
        ret = libxml2mod.xmlNewNs(self._o, href, prefix)
        if ret is None:raise treeError('xmlNewNs() failed')
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def newNsProp(self, ns, name, value):
        """Create a new property tagged with a namespace and carried
           by a node. """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlNewNsProp(self._o, ns__o, name, value)
        if ret is None:raise treeError('xmlNewNsProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def newNsPropEatName(self, ns, name, value):
        """Create a new property tagged with a namespace and carried
           by a node. """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlNewNsPropEatName(self._o, ns__o, name, value)
        if ret is None:raise treeError('xmlNewNsPropEatName() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def newProp(self, name, value):
        """Create a new property carried by a node. """
        ret = libxml2mod.xmlNewProp(self._o, name, value)
        if ret is None:raise treeError('xmlNewProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def newTextChild(self, ns, name, content):
        """Creation of a new child element, added at the end of
          @parent children list. @ns and @content parameters are
          optional (None). If @ns is None, the newly created element
          inherits the namespace of @parent. If @content is non None,
          a child TEXT node will be created containing the string
          @content. NOTE: Use xmlNewChild() if @content will contain
          entities that need to be preserved. Use this function,
          xmlNewTextChild(), if you need to ensure that reserved XML
          chars that might appear in @content, such as the ampersand,
          greater-than or less-than signs, are automatically replaced
           by their XML escaped entity representations. """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlNewTextChild(self._o, ns__o, name, content)
        if ret is None:raise treeError('xmlNewTextChild() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def nextElementSibling(self):
        """Finds the first closest next sibling of the node which is
          an element node. Note the handling of entities references
          is different than in the W3C DOM element traversal spec
          since we don't have back reference from entities content to
           entities references. """
        ret = libxml2mod.xmlNextElementSibling(self._o)
        if ret is None:return None
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def noNsProp(self, name):
        """Search and get the value of an attribute associated to a
          node This does the entity substitution. This function looks
          in DTD attribute declaration for #FIXED or default
          declaration values unless DTD use has been turned off. This
          function is similar to xmlGetProp except it will accept
           only an attribute in no namespace. """
        ret = libxml2mod.xmlGetNoNsProp(self._o, name)
        return ret

    def nodePath(self):
        """Build a structure based Path for the given node """
        ret = libxml2mod.xmlGetNodePath(self._o)
        return ret

    def nsProp(self, name, nameSpace):
        """Search and get the value of an attribute associated to a
          node This attribute has to be anchored in the namespace
          specified. This does the entity substitution. This function
          looks in DTD attribute declaration for #FIXED or default
           declaration values unless DTD use has been turned off. """
        ret = libxml2mod.xmlGetNsProp(self._o, name, nameSpace)
        return ret

    def previousElementSibling(self):
        """Finds the first closest previous sibling of the node which
          is an element node. Note the handling of entities
          references is different than in the W3C DOM element
          traversal spec since we don't have back reference from
           entities content to entities references. """
        ret = libxml2mod.xmlPreviousElementSibling(self._o)
        if ret is None:return None
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def prop(self, name):
        """Search and get the value of an attribute associated to a
          node This does the entity substitution. This function looks
          in DTD attribute declaration for #FIXED or default
          declaration values unless DTD use has been turned off.
          NOTE: this function acts independently of namespaces
          associated to the attribute. Use xmlGetNsProp() or
           xmlGetNoNsProp() for namespace aware processing. """
        ret = libxml2mod.xmlGetProp(self._o, name)
        return ret

    def reconciliateNs(self, doc):
        """This function checks that all the namespaces declared
          within the given tree are properly declared. This is needed
          for example after Copy or Cut and then paste operations.
          The subtree may still hold pointers to namespace
          declarations outside the subtree or invalid/masked. As much
          as possible the function try to reuse the existing
          namespaces found in the new environment. If not possible
          the new namespaces are redeclared on @tree at the top of
           the given subtree. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlReconciliateNs(doc__o, self._o)
        return ret

    def replaceNode(self, cur):
        """Unlink the old node from its current context, prune the new
          one at the same place. If @cur was already inserted in a
           document it is first unlinked from its existing context. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlReplaceNode(self._o, cur__o)
        if ret is None:raise treeError('xmlReplaceNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def searchNs(self, doc, nameSpace):
        """Search a Ns registered under a given name space for a
          document. recurse on the parents until it finds the defined
          namespace or return None otherwise. @nameSpace can be None,
          this is a search for the default namespace. We don't allow
          to cross entities boundaries. If you don't declare the
          namespace within those you will be in troubles !!! A
           warning is generated to cover this case. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlSearchNs(doc__o, self._o, nameSpace)
        if ret is None:raise treeError('xmlSearchNs() failed')
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def searchNsByHref(self, doc, href):
        """Search a Ns aliasing a given URI. Recurse on the parents
          until it finds the defined namespace or return None
           otherwise. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlSearchNsByHref(doc__o, self._o, href)
        if ret is None:raise treeError('xmlSearchNsByHref() failed')
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def setBase(self, uri):
        """Set (or reset) the base URI of a node, i.e. the value of
           the xml:base attribute. """
        libxml2mod.xmlNodeSetBase(self._o, uri)

    def setContent(self, content):
        """Replace the content of a node. NOTE: @content is supposed
          to be a piece of XML CDATA, so it allows entity references,
          but XML special chars need to be escaped first by using
           xmlEncodeEntitiesReentrant() resp. xmlEncodeSpecialChars(). """
        libxml2mod.xmlNodeSetContent(self._o, content)

    def setContentLen(self, content, len):
        """Replace the content of a node. NOTE: @content is supposed
          to be a piece of XML CDATA, so it allows entity references,
          but XML special chars need to be escaped first by using
           xmlEncodeEntitiesReentrant() resp. xmlEncodeSpecialChars(). """
        libxml2mod.xmlNodeSetContentLen(self._o, content, len)

    def setLang(self, lang):
        """Set the language of a node, i.e. the values of the xml:lang
           attribute. """
        libxml2mod.xmlNodeSetLang(self._o, lang)

    def setListDoc(self, doc):
        """update all nodes in the list to point to the right document """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        libxml2mod.xmlSetListDoc(self._o, doc__o)

    def setName(self, name):
        """Set (or reset) the name of a node. """
        libxml2mod.xmlNodeSetName(self._o, name)

    def setNs(self, ns):
        """Associate a namespace to a node, a posteriori. """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        libxml2mod.xmlSetNs(self._o, ns__o)

    def setNsProp(self, ns, name, value):
        """Set (or reset) an attribute carried by a node. The ns
           structure must be in scope, this is not checked """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlSetNsProp(self._o, ns__o, name, value)
        if ret is None:raise treeError('xmlSetNsProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def setProp(self, name, value):
        """Set (or reset) an attribute carried by a node. If @name has
          a prefix, then the corresponding namespace-binding will be
          used, if in scope; it is an error it there's no such
           ns-binding for the prefix in scope. """
        ret = libxml2mod.xmlSetProp(self._o, name, value)
        if ret is None:raise treeError('xmlSetProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def setSpacePreserve(self, val):
        """Set (or reset) the space preserving behaviour of a node,
           i.e. the value of the xml:space attribute. """
        libxml2mod.xmlNodeSetSpacePreserve(self._o, val)

    def setTreeDoc(self, doc):
        """update all nodes under the tree to point to the right
           document """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        libxml2mod.xmlSetTreeDoc(self._o, doc__o)

    def textConcat(self, content, len):
        """Concat the given string at the end of the existing node
           content """
        ret = libxml2mod.xmlTextConcat(self._o, content, len)
        return ret

    def textMerge(self, second):
        """Merge two text nodes into one """
        if second is None: second__o = None
        else: second__o = second._o
        ret = libxml2mod.xmlTextMerge(self._o, second__o)
        if ret is None:raise treeError('xmlTextMerge() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def unlinkNode(self):
        """Unlink a node from it's current context, the node is not
          freed If one need to free the node, use xmlFreeNode()
          routine after the unlink to discard it. Note that namespace
          nodes can't be unlinked as they do not have pointer to
           their parent. """
        libxml2mod.xmlUnlinkNode(self._o)

    def unsetNsProp(self, ns, name):
        """Remove an attribute carried by a node. """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlUnsetNsProp(self._o, ns__o, name)
        return ret

    def unsetProp(self, name):
        """Remove an attribute carried by a node. This handles only
           attributes in no namespace. """
        ret = libxml2mod.xmlUnsetProp(self._o, name)
        return ret

    #
    # xmlNode functions from module valid
    #

    def isID(self, doc, attr):
        """Determine whether an attribute is of type ID. In case we
          have DTD(s) then this is done if DTD loading has been
          requested. In the case of HTML documents parsed with the
           HTML parser, then ID detection is done systematically. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if attr is None: attr__o = None
        else: attr__o = attr._o
        ret = libxml2mod.xmlIsID(doc__o, self._o, attr__o)
        return ret

    def isRef(self, doc, attr):
        """Determine whether an attribute is of type Ref. In case we
          have DTD(s) then this is simple, otherwise we use an
           heuristic: name Ref (upper or lowercase). """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if attr is None: attr__o = None
        else: attr__o = attr._o
        ret = libxml2mod.xmlIsRef(doc__o, self._o, attr__o)
        return ret

    def validNormalizeAttributeValue(self, doc, name, value):
        """Does the validation related extra step of the normalization
          of attribute values:  If the declared value is not CDATA,
          then the XML processor must further process the normalized
          attribute value by discarding any leading and trailing
          space (#x20) characters, and by replacing sequences of
           space (#x20) characters by single space (#x20) character. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlValidNormalizeAttributeValue(doc__o, self._o, name, value)
        return ret

    #
    # xmlNode functions from module xinclude
    #

    def xincludeProcessTree(self):
        """Implement the XInclude substitution for the given subtree """
        ret = libxml2mod.xmlXIncludeProcessTree(self._o)
        return ret

    def xincludeProcessTreeFlags(self, flags):
        """Implement the XInclude substitution for the given subtree """
        ret = libxml2mod.xmlXIncludeProcessTreeFlags(self._o, flags)
        return ret

    #
    # xmlNode functions from module xmlschemas
    #

    def schemaValidateOneElement(self, ctxt):
        """Validate a branch of a tree, starting with the given @elem. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlSchemaValidateOneElement(ctxt__o, self._o)
        return ret

    #
    # xmlNode functions from module xpath
    #

    def xpathCastNodeToNumber(self):
        """Converts a node to its number value """
        ret = libxml2mod.xmlXPathCastNodeToNumber(self._o)
        return ret

    def xpathCastNodeToString(self):
        """Converts a node to its string value. """
        ret = libxml2mod.xmlXPathCastNodeToString(self._o)
        return ret

    def xpathCmpNodes(self, node2):
        """Compare two nodes w.r.t document order """
        if node2 is None: node2__o = None
        else: node2__o = node2._o
        ret = libxml2mod.xmlXPathCmpNodes(self._o, node2__o)
        return ret

    def xpathNodeEval(self, str, ctx):
        """Evaluate the XPath Location Path in the given context. The
          node 'node' is set as the context node. The context node is
           not restored. """
        if ctx is None: ctx__o = None
        else: ctx__o = ctx._o
        ret = libxml2mod.xmlXPathNodeEval(self._o, str, ctx__o)
        if ret is None:raise xpathError('xmlXPathNodeEval() failed')
        return xpathObjectRet(ret)

    #
    # xmlNode functions from module xpathInternals
    #

    def xpathNewNodeSet(self):
        """Create a new xmlXPathObjectPtr of type NodeSet and
           initialize it with the single Node @val """
        ret = libxml2mod.xmlXPathNewNodeSet(self._o)
        if ret is None:raise xpathError('xmlXPathNewNodeSet() failed')
        return xpathObjectRet(ret)

    def xpathNewValueTree(self):
        """Create a new xmlXPathObjectPtr of type Value Tree (XSLT)
           and initialize it with the tree root @val """
        ret = libxml2mod.xmlXPathNewValueTree(self._o)
        if ret is None:raise xpathError('xmlXPathNewValueTree() failed')
        return xpathObjectRet(ret)

    def xpathNextAncestor(self, ctxt):
        """Traversal function for the "ancestor" direction the
          ancestor axis contains the ancestors of the context node;
          the ancestors of the context node consist of the parent of
          context node and the parent's parent and so on; the nodes
          are ordered in reverse document order; thus the parent is
          the first node on the axis, and the parent's parent is the
           second node on the axis """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextAncestor(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextAncestor() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextAncestorOrSelf(self, ctxt):
        """Traversal function for the "ancestor-or-self" direction he
          ancestor-or-self axis contains the context node and
          ancestors of the context node in reverse document order;
          thus the context node is the first node on the axis, and
          the context node's parent the second; parent here is
           defined the same as with the parent axis. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextAncestorOrSelf(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextAncestorOrSelf() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextAttribute(self, ctxt):
        """Traversal function for the "attribute" direction TODO:
           support DTD inherited default attributes """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextAttribute(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextAttribute() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextChild(self, ctxt):
        """Traversal function for the "child" direction The child axis
          contains the children of the context node in document order. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextChild(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextChild() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextDescendant(self, ctxt):
        """Traversal function for the "descendant" direction the
          descendant axis contains the descendants of the context
          node in document order; a descendant is a child or a child
           of a child and so on. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextDescendant(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextDescendant() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextDescendantOrSelf(self, ctxt):
        """Traversal function for the "descendant-or-self" direction
          the descendant-or-self axis contains the context node and
          the descendants of the context node in document order; thus
          the context node is the first node on the axis, and the
          first child of the context node is the second node on the
           axis """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextDescendantOrSelf(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextDescendantOrSelf() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextFollowing(self, ctxt):
        """Traversal function for the "following" direction The
          following axis contains all nodes in the same document as
          the context node that are after the context node in
          document order, excluding any descendants and excluding
          attribute nodes and namespace nodes; the nodes are ordered
           in document order """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextFollowing(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextFollowing() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextFollowingSibling(self, ctxt):
        """Traversal function for the "following-sibling" direction
          The following-sibling axis contains the following siblings
           of the context node in document order. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextFollowingSibling(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextFollowingSibling() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextNamespace(self, ctxt):
        """Traversal function for the "namespace" direction the
          namespace axis contains the namespace nodes of the context
          node; the order of nodes on this axis is
          implementation-defined; the axis will be empty unless the
          context node is an element  We keep the XML namespace node
           at the end of the list. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextNamespace(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextNamespace() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextParent(self, ctxt):
        """Traversal function for the "parent" direction The parent
          axis contains the parent of the context node, if there is
           one. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextParent(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextParent() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextPreceding(self, ctxt):
        """Traversal function for the "preceding" direction the
          preceding axis contains all nodes in the same document as
          the context node that are before the context node in
          document order, excluding any ancestors and excluding
          attribute nodes and namespace nodes; the nodes are ordered
           in reverse document order """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextPreceding(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextPreceding() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextPrecedingSibling(self, ctxt):
        """Traversal function for the "preceding-sibling" direction
          The preceding-sibling axis contains the preceding siblings
          of the context node in reverse document order; the first
          preceding sibling is first on the axis; the sibling
           preceding that node is the second on the axis and so on. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextPrecedingSibling(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextPrecedingSibling() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextSelf(self, ctxt):
        """Traversal function for the "self" direction The self axis
           contains just the context node itself """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlXPathNextSelf(ctxt__o, self._o)
        if ret is None:raise xpathError('xmlXPathNextSelf() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    #
    # xmlNode functions from module xpointer
    #

    def xpointerNewCollapsedRange(self):
        """Create a new xmlXPathObjectPtr of type range using a single
           nodes """
        ret = libxml2mod.xmlXPtrNewCollapsedRange(self._o)
        if ret is None:raise treeError('xmlXPtrNewCollapsedRange() failed')
        return xpathObjectRet(ret)

    def xpointerNewContext(self, doc, origin):
        """Create a new XPointer context """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if origin is None: origin__o = None
        else: origin__o = origin._o
        ret = libxml2mod.xmlXPtrNewContext(doc__o, self._o, origin__o)
        if ret is None:raise treeError('xmlXPtrNewContext() failed')
        __tmp = xpathContext(_obj=ret)
        return __tmp

    def xpointerNewLocationSetNodes(self, end):
        """Create a new xmlXPathObjectPtr of type LocationSet and
          initialize it with the single range made of the two nodes
           @start and @end """
        if end is None: end__o = None
        else: end__o = end._o
        ret = libxml2mod.xmlXPtrNewLocationSetNodes(self._o, end__o)
        if ret is None:raise treeError('xmlXPtrNewLocationSetNodes() failed')
        return xpathObjectRet(ret)

    def xpointerNewRange(self, startindex, end, endindex):
        """Create a new xmlXPathObjectPtr of type range """
        if end is None: end__o = None
        else: end__o = end._o
        ret = libxml2mod.xmlXPtrNewRange(self._o, startindex, end__o, endindex)
        if ret is None:raise treeError('xmlXPtrNewRange() failed')
        return xpathObjectRet(ret)

    def xpointerNewRangeNodes(self, end):
        """Create a new xmlXPathObjectPtr of type range using 2 nodes """
        if end is None: end__o = None
        else: end__o = end._o
        ret = libxml2mod.xmlXPtrNewRangeNodes(self._o, end__o)
        if ret is None:raise treeError('xmlXPtrNewRangeNodes() failed')
        return xpathObjectRet(ret)

class xmlDoc(xmlNode):
    def __init__(self, _obj=None):
        if checkWrapper(_obj) != 0:            raise TypeError('xmlDoc got a wrong wrapper object type')
        self._o = _obj
        xmlNode.__init__(self, _obj=_obj)

    def __repr__(self):
        return "<xmlDoc (%s) object at 0x%x>" % (self.name, int(pos_id (self)))

    #
    # xmlDoc functions from module HTMLparser
    #

    def htmlAutoCloseTag(self, name, elem):
        """The HTML DTD allows a tag to implicitly close other tags.
          The list is kept in htmlStartClose array. This function
          checks if the element or one of it's children would
           autoclose the given tag. """
        ret = libxml2mod.htmlAutoCloseTag(self._o, name, elem)
        return ret

    def htmlIsAutoClosed(self, elem):
        """The HTML DTD allows a tag to implicitly close other tags.
          The list is kept in htmlStartClose array. This function
           checks if a tag is autoclosed by one of it's child """
        ret = libxml2mod.htmlIsAutoClosed(self._o, elem)
        return ret

    #
    # xmlDoc functions from module HTMLtree
    #

    def htmlDocContentDumpFormatOutput(self, buf, encoding, format):
        """Dump an HTML document. """
        if buf is None: buf__o = None
        else: buf__o = buf._o
        libxml2mod.htmlDocContentDumpFormatOutput(buf__o, self._o, encoding, format)

    def htmlDocContentDumpOutput(self, buf, encoding):
        """Dump an HTML document. Formating return/spaces are added. """
        if buf is None: buf__o = None
        else: buf__o = buf._o
        libxml2mod.htmlDocContentDumpOutput(buf__o, self._o, encoding)

    def htmlDocDump(self, f):
        """Dump an HTML document to an open FILE. """
        ret = libxml2mod.htmlDocDump(f, self._o)
        return ret

    def htmlGetMetaEncoding(self):
        """Encoding definition lookup in the Meta tags """
        ret = libxml2mod.htmlGetMetaEncoding(self._o)
        return ret

    def htmlNodeDumpFile(self, out, cur):
        """Dump an HTML node, recursive behaviour,children are printed
           too, and formatting returns are added. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.htmlNodeDumpFile(out, self._o, cur__o)

    def htmlNodeDumpFileFormat(self, out, cur, encoding, format):
        """Dump an HTML node, recursive behaviour,children are printed
          too.  TODO: if encoding == None try to save in the doc
           encoding """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.htmlNodeDumpFileFormat(out, self._o, cur__o, encoding, format)
        return ret

    def htmlNodeDumpFormatOutput(self, buf, cur, encoding, format):
        """Dump an HTML node, recursive behaviour,children are printed
           too. """
        if buf is None: buf__o = None
        else: buf__o = buf._o
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.htmlNodeDumpFormatOutput(buf__o, self._o, cur__o, encoding, format)

    def htmlNodeDumpOutput(self, buf, cur, encoding):
        """Dump an HTML node, recursive behaviour,children are printed
           too, and formatting returns/spaces are added. """
        if buf is None: buf__o = None
        else: buf__o = buf._o
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.htmlNodeDumpOutput(buf__o, self._o, cur__o, encoding)

    def htmlSaveFile(self, filename):
        """Dump an HTML document to a file. If @filename is "-" the
           stdout file is used. """
        ret = libxml2mod.htmlSaveFile(filename, self._o)
        return ret

    def htmlSaveFileEnc(self, filename, encoding):
        """Dump an HTML document to a file using a given encoding and
           formatting returns/spaces are added. """
        ret = libxml2mod.htmlSaveFileEnc(filename, self._o, encoding)
        return ret

    def htmlSaveFileFormat(self, filename, encoding, format):
        """Dump an HTML document to a file using a given encoding. """
        ret = libxml2mod.htmlSaveFileFormat(filename, self._o, encoding, format)
        return ret

    def htmlSetMetaEncoding(self, encoding):
        """Sets the current encoding in the Meta tags NOTE: this will
          not change the document content encoding, just the META
           flag associated. """
        ret = libxml2mod.htmlSetMetaEncoding(self._o, encoding)
        return ret

    #
    # xmlDoc functions from module debugXML
    #

    def debugCheckDocument(self, output):
        """Check the document for potential content problems, and
           output the errors to @output """
        ret = libxml2mod.xmlDebugCheckDocument(output, self._o)
        return ret

    def debugDumpDocument(self, output):
        """Dumps debug information for the document, it's recursive """
        libxml2mod.xmlDebugDumpDocument(output, self._o)

    def debugDumpDocumentHead(self, output):
        """Dumps debug information cncerning the document, not
           recursive """
        libxml2mod.xmlDebugDumpDocumentHead(output, self._o)

    def debugDumpEntities(self, output):
        """Dumps debug information for all the entities in use by the
           document """
        libxml2mod.xmlDebugDumpEntities(output, self._o)

    #
    # xmlDoc functions from module entities
    #

    def addDocEntity(self, name, type, ExternalID, SystemID, content):
        """Register a new entity for this document. """
        ret = libxml2mod.xmlAddDocEntity(self._o, name, type, ExternalID, SystemID, content)
        if ret is None:raise treeError('xmlAddDocEntity() failed')
        __tmp = xmlEntity(_obj=ret)
        return __tmp

    def addDtdEntity(self, name, type, ExternalID, SystemID, content):
        """Register a new entity for this document DTD external subset. """
        ret = libxml2mod.xmlAddDtdEntity(self._o, name, type, ExternalID, SystemID, content)
        if ret is None:raise treeError('xmlAddDtdEntity() failed')
        __tmp = xmlEntity(_obj=ret)
        return __tmp

    def docEntity(self, name):
        """Do an entity lookup in the document entity hash table and """
        ret = libxml2mod.xmlGetDocEntity(self._o, name)
        if ret is None:raise treeError('xmlGetDocEntity() failed')
        __tmp = xmlEntity(_obj=ret)
        return __tmp

    def dtdEntity(self, name):
        """Do an entity lookup in the DTD entity hash table and """
        ret = libxml2mod.xmlGetDtdEntity(self._o, name)
        if ret is None:raise treeError('xmlGetDtdEntity() failed')
        __tmp = xmlEntity(_obj=ret)
        return __tmp

    def encodeEntities(self, input):
        """TODO: remove xmlEncodeEntities, once we are not afraid of
          breaking binary compatibility  People must migrate their
          code to xmlEncodeEntitiesReentrant ! This routine will
           issue a warning when encountered. """
        ret = libxml2mod.xmlEncodeEntities(self._o, input)
        return ret

    def encodeEntitiesReentrant(self, input):
        """Do a global encoding of a string, replacing the predefined
          entities and non ASCII values with their entities and
          CharRef counterparts. Contrary to xmlEncodeEntities, this
           routine is reentrant, and result must be deallocated. """
        ret = libxml2mod.xmlEncodeEntitiesReentrant(self._o, input)
        return ret

    def encodeSpecialChars(self, input):
        """Do a global encoding of a string, replacing the predefined
          entities this routine is reentrant, and result must be
           deallocated. """
        ret = libxml2mod.xmlEncodeSpecialChars(self._o, input)
        return ret

    def newEntity(self, name, type, ExternalID, SystemID, content):
        """Create a new entity, this differs from xmlAddDocEntity()
          that if the document is None or has no internal subset
          defined, then an unlinked entity structure will be
          returned, it is then the responsability of the caller to
          link it to the document later or free it when not needed
           anymore. """
        ret = libxml2mod.xmlNewEntity(self._o, name, type, ExternalID, SystemID, content)
        if ret is None:raise treeError('xmlNewEntity() failed')
        __tmp = xmlEntity(_obj=ret)
        return __tmp

    def parameterEntity(self, name):
        """Do an entity lookup in the internal and external subsets and """
        ret = libxml2mod.xmlGetParameterEntity(self._o, name)
        if ret is None:raise treeError('xmlGetParameterEntity() failed')
        __tmp = xmlEntity(_obj=ret)
        return __tmp

    #
    # xmlDoc functions from module relaxng
    #

    def relaxNGNewDocParserCtxt(self):
        """Create an XML RelaxNGs parser context for that document.
          Note: since the process of compiling a RelaxNG schemas
          modifies the document, the @doc parameter is duplicated
           internally. """
        ret = libxml2mod.xmlRelaxNGNewDocParserCtxt(self._o)
        if ret is None:raise parserError('xmlRelaxNGNewDocParserCtxt() failed')
        __tmp = relaxNgParserCtxt(_obj=ret)
        return __tmp

    def relaxNGValidateDoc(self, ctxt):
        """Validate a document tree in memory. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlRelaxNGValidateDoc(ctxt__o, self._o)
        return ret

    def relaxNGValidateFullElement(self, ctxt, elem):
        """Validate a full subtree when
          xmlRelaxNGValidatePushElement() returned 0 and the content
           of the node has been expanded. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlRelaxNGValidateFullElement(ctxt__o, self._o, elem__o)
        return ret

    def relaxNGValidatePopElement(self, ctxt, elem):
        """Pop the element end from the RelaxNG validation stack. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlRelaxNGValidatePopElement(ctxt__o, self._o, elem__o)
        return ret

    def relaxNGValidatePushElement(self, ctxt, elem):
        """Push a new element start on the RelaxNG validation stack. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlRelaxNGValidatePushElement(ctxt__o, self._o, elem__o)
        return ret

    #
    # xmlDoc functions from module tree
    #

    def copyDoc(self, recursive):
        """Do a copy of the document info. If recursive, the content
          tree will be copied too as well as DTD, namespaces and
           entities. """
        ret = libxml2mod.xmlCopyDoc(self._o, recursive)
        if ret is None:raise treeError('xmlCopyDoc() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def copyNode(self, node, extended):
        """Do a copy of the node to a given document. """
        if node is None: node__o = None
        else: node__o = node._o
        ret = libxml2mod.xmlDocCopyNode(node__o, self._o, extended)
        if ret is None:raise treeError('xmlDocCopyNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def copyNodeList(self, node):
        """Do a recursive copy of the node list. """
        if node is None: node__o = None
        else: node__o = node._o
        ret = libxml2mod.xmlDocCopyNodeList(self._o, node__o)
        if ret is None:raise treeError('xmlDocCopyNodeList() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def createIntSubset(self, name, ExternalID, SystemID):
        """Create the internal subset of a document """
        ret = libxml2mod.xmlCreateIntSubset(self._o, name, ExternalID, SystemID)
        if ret is None:raise treeError('xmlCreateIntSubset() failed')
        __tmp = xmlDtd(_obj=ret)
        return __tmp

    def docCompressMode(self):
        """get the compression ratio for a document, ZLIB based """
        ret = libxml2mod.xmlGetDocCompressMode(self._o)
        return ret

    def dump(self, f):
        """Dump an XML document to an open FILE. """
        ret = libxml2mod.xmlDocDump(f, self._o)
        return ret

    def elemDump(self, f, cur):
        """Dump an XML/HTML node, recursive behaviour, children are
           printed too. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.xmlElemDump(f, self._o, cur__o)

    def formatDump(self, f, format):
        """Dump an XML document to an open FILE. """
        ret = libxml2mod.xmlDocFormatDump(f, self._o, format)
        return ret

    def freeDoc(self):
        """Free up all the structures used by a document, tree
           included. """
        libxml2mod.xmlFreeDoc(self._o)

    def getRootElement(self):
        """Get the root element of the document (doc->children is a
           list containing possibly comments, PIs, etc ...). """
        ret = libxml2mod.xmlDocGetRootElement(self._o)
        if ret is None:raise treeError('xmlDocGetRootElement() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def intSubset(self):
        """Get the internal subset of a document """
        ret = libxml2mod.xmlGetIntSubset(self._o)
        if ret is None:raise treeError('xmlGetIntSubset() failed')
        __tmp = xmlDtd(_obj=ret)
        return __tmp

    def newCDataBlock(self, content, len):
        """Creation of a new node containing a CDATA block. """
        ret = libxml2mod.xmlNewCDataBlock(self._o, content, len)
        if ret is None:raise treeError('xmlNewCDataBlock() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newCharRef(self, name):
        """Creation of a new character reference node. """
        ret = libxml2mod.xmlNewCharRef(self._o, name)
        if ret is None:raise treeError('xmlNewCharRef() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocComment(self, content):
        """Creation of a new node containing a comment within a
           document. """
        ret = libxml2mod.xmlNewDocComment(self._o, content)
        if ret is None:raise treeError('xmlNewDocComment() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocFragment(self):
        """Creation of a new Fragment node. """
        ret = libxml2mod.xmlNewDocFragment(self._o)
        if ret is None:raise treeError('xmlNewDocFragment() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocNode(self, ns, name, content):
        """Creation of a new node element within a document. @ns and
          @content are optional (None). NOTE: @content is supposed to
          be a piece of XML CDATA, so it allow entities references,
          but XML special chars need to be escaped first by using
          xmlEncodeEntitiesReentrant(). Use xmlNewDocRawNode() if you
           don't need entities support. """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlNewDocNode(self._o, ns__o, name, content)
        if ret is None:raise treeError('xmlNewDocNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocNodeEatName(self, ns, name, content):
        """Creation of a new node element within a document. @ns and
          @content are optional (None). NOTE: @content is supposed to
          be a piece of XML CDATA, so it allow entities references,
          but XML special chars need to be escaped first by using
          xmlEncodeEntitiesReentrant(). Use xmlNewDocRawNode() if you
           don't need entities support. """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlNewDocNodeEatName(self._o, ns__o, name, content)
        if ret is None:raise treeError('xmlNewDocNodeEatName() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocPI(self, name, content):
        """Creation of a processing instruction element. """
        ret = libxml2mod.xmlNewDocPI(self._o, name, content)
        if ret is None:raise treeError('xmlNewDocPI() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocProp(self, name, value):
        """Create a new property carried by a document. """
        ret = libxml2mod.xmlNewDocProp(self._o, name, value)
        if ret is None:raise treeError('xmlNewDocProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def newDocRawNode(self, ns, name, content):
        """Creation of a new node element within a document. @ns and
           @content are optional (None). """
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlNewDocRawNode(self._o, ns__o, name, content)
        if ret is None:raise treeError('xmlNewDocRawNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocText(self, content):
        """Creation of a new text node within a document. """
        ret = libxml2mod.xmlNewDocText(self._o, content)
        if ret is None:raise treeError('xmlNewDocText() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocTextLen(self, content, len):
        """Creation of a new text node with an extra content length
           parameter. The text node pertain to a given document. """
        ret = libxml2mod.xmlNewDocTextLen(self._o, content, len)
        if ret is None:raise treeError('xmlNewDocTextLen() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDtd(self, name, ExternalID, SystemID):
        """Creation of a new DTD for the external subset. To create an
           internal subset, use xmlCreateIntSubset(). """
        ret = libxml2mod.xmlNewDtd(self._o, name, ExternalID, SystemID)
        if ret is None:raise treeError('xmlNewDtd() failed')
        __tmp = xmlDtd(_obj=ret)
        return __tmp

    def newGlobalNs(self, href, prefix):
        """Creation of a Namespace, the old way using PI and without
           scoping DEPRECATED !!! """
        ret = libxml2mod.xmlNewGlobalNs(self._o, href, prefix)
        if ret is None:raise treeError('xmlNewGlobalNs() failed')
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def newReference(self, name):
        """Creation of a new reference node. """
        ret = libxml2mod.xmlNewReference(self._o, name)
        if ret is None:raise treeError('xmlNewReference() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def nodeDumpOutput(self, buf, cur, level, format, encoding):
        """Dump an XML node, recursive behaviour, children are printed
          too. Note that @format = 1 provide node indenting only if
          xmlIndentTreeOutput = 1 or xmlKeepBlanksDefault(0) was
           called """
        if buf is None: buf__o = None
        else: buf__o = buf._o
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.xmlNodeDumpOutput(buf__o, self._o, cur__o, level, format, encoding)

    def nodeGetBase(self, cur):
        """Searches for the BASE URL. The code should work on both XML
          and HTML document even if base mechanisms are completely
          different. It returns the base as defined in RFC 2396
          sections 5.1.1. Base URI within Document Content and 5.1.2.
          Base URI from the Encapsulating Entity However it does not
           return the document base (5.1.3), use doc->URL in this case """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlNodeGetBase(self._o, cur__o)
        return ret

    def nodeListGetRawString(self, list, inLine):
        """Builds the string equivalent to the text contained in the
          Node list made of TEXTs and ENTITY_REFs, contrary to
          xmlNodeListGetString() this function doesn't do any
           character encoding handling. """
        if list is None: list__o = None
        else: list__o = list._o
        ret = libxml2mod.xmlNodeListGetRawString(self._o, list__o, inLine)
        return ret

    def nodeListGetString(self, list, inLine):
        """Build the string equivalent to the text contained in the
           Node list made of TEXTs and ENTITY_REFs """
        if list is None: list__o = None
        else: list__o = list._o
        ret = libxml2mod.xmlNodeListGetString(self._o, list__o, inLine)
        return ret

    def reconciliateNs(self, tree):
        """This function checks that all the namespaces declared
          within the given tree are properly declared. This is needed
          for example after Copy or Cut and then paste operations.
          The subtree may still hold pointers to namespace
          declarations outside the subtree or invalid/masked. As much
          as possible the function try to reuse the existing
          namespaces found in the new environment. If not possible
          the new namespaces are redeclared on @tree at the top of
           the given subtree. """
        if tree is None: tree__o = None
        else: tree__o = tree._o
        ret = libxml2mod.xmlReconciliateNs(self._o, tree__o)
        return ret

    def saveFile(self, filename):
        """Dump an XML document to a file. Will use compression if
          compiled in and enabled. If @filename is "-" the stdout
           file is used. """
        ret = libxml2mod.xmlSaveFile(filename, self._o)
        return ret

    def saveFileEnc(self, filename, encoding):
        """Dump an XML document, converting it to the given encoding """
        ret = libxml2mod.xmlSaveFileEnc(filename, self._o, encoding)
        return ret

    def saveFileTo(self, buf, encoding):
        """Dump an XML document to an I/O buffer. Warning ! This call
          xmlOutputBufferClose() on buf which is not available after
           this call. """
        if buf is None: buf__o = None
        else: buf__o = buf._o
        ret = libxml2mod.xmlSaveFileTo(buf__o, self._o, encoding)
        return ret

    def saveFormatFile(self, filename, format):
        """Dump an XML document to a file. Will use compression if
          compiled in and enabled. If @filename is "-" the stdout
          file is used. If @format is set then the document will be
          indented on output. Note that @format = 1 provide node
          indenting only if xmlIndentTreeOutput = 1 or
           xmlKeepBlanksDefault(0) was called """
        ret = libxml2mod.xmlSaveFormatFile(filename, self._o, format)
        return ret

    def saveFormatFileEnc(self, filename, encoding, format):
        """Dump an XML document to a file or an URL. """
        ret = libxml2mod.xmlSaveFormatFileEnc(filename, self._o, encoding, format)
        return ret

    def saveFormatFileTo(self, buf, encoding, format):
        """Dump an XML document to an I/O buffer. Warning ! This call
          xmlOutputBufferClose() on buf which is not available after
           this call. """
        if buf is None: buf__o = None
        else: buf__o = buf._o
        ret = libxml2mod.xmlSaveFormatFileTo(buf__o, self._o, encoding, format)
        return ret

    def searchNs(self, node, nameSpace):
        """Search a Ns registered under a given name space for a
          document. recurse on the parents until it finds the defined
          namespace or return None otherwise. @nameSpace can be None,
          this is a search for the default namespace. We don't allow
          to cross entities boundaries. If you don't declare the
          namespace within those you will be in troubles !!! A
           warning is generated to cover this case. """
        if node is None: node__o = None
        else: node__o = node._o
        ret = libxml2mod.xmlSearchNs(self._o, node__o, nameSpace)
        if ret is None:raise treeError('xmlSearchNs() failed')
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def searchNsByHref(self, node, href):
        """Search a Ns aliasing a given URI. Recurse on the parents
          until it finds the defined namespace or return None
           otherwise. """
        if node is None: node__o = None
        else: node__o = node._o
        ret = libxml2mod.xmlSearchNsByHref(self._o, node__o, href)
        if ret is None:raise treeError('xmlSearchNsByHref() failed')
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def setDocCompressMode(self, mode):
        """set the compression ratio for a document, ZLIB based
           Correct values: 0 (uncompressed) to 9 (max compression) """
        libxml2mod.xmlSetDocCompressMode(self._o, mode)

    def setListDoc(self, list):
        """update all nodes in the list to point to the right document """
        if list is None: list__o = None
        else: list__o = list._o
        libxml2mod.xmlSetListDoc(list__o, self._o)

    def setRootElement(self, root):
        """Set the root element of the document (doc->children is a
           list containing possibly comments, PIs, etc ...). """
        if root is None: root__o = None
        else: root__o = root._o
        ret = libxml2mod.xmlDocSetRootElement(self._o, root__o)
        if ret is None:return None
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def setTreeDoc(self, tree):
        """update all nodes under the tree to point to the right
           document """
        if tree is None: tree__o = None
        else: tree__o = tree._o
        libxml2mod.xmlSetTreeDoc(tree__o, self._o)

    def stringGetNodeList(self, value):
        """Parse the value string and build the node list associated.
           Should produce a flat tree with only TEXTs and ENTITY_REFs. """
        ret = libxml2mod.xmlStringGetNodeList(self._o, value)
        if ret is None:raise treeError('xmlStringGetNodeList() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def stringLenGetNodeList(self, value, len):
        """Parse the value string and build the node list associated.
           Should produce a flat tree with only TEXTs and ENTITY_REFs. """
        ret = libxml2mod.xmlStringLenGetNodeList(self._o, value, len)
        if ret is None:raise treeError('xmlStringLenGetNodeList() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    #
    # xmlDoc functions from module valid
    #

    def ID(self, ID):
        """Search the attribute declaring the given ID """
        ret = libxml2mod.xmlGetID(self._o, ID)
        if ret is None:raise treeError('xmlGetID() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def isID(self, elem, attr):
        """Determine whether an attribute is of type ID. In case we
          have DTD(s) then this is done if DTD loading has been
          requested. In the case of HTML documents parsed with the
           HTML parser, then ID detection is done systematically. """
        if elem is None: elem__o = None
        else: elem__o = elem._o
        if attr is None: attr__o = None
        else: attr__o = attr._o
        ret = libxml2mod.xmlIsID(self._o, elem__o, attr__o)
        return ret

    def isMixedElement(self, name):
        """Search in the DtDs whether an element accept Mixed content
           (or ANY) basically if it is supposed to accept text childs """
        ret = libxml2mod.xmlIsMixedElement(self._o, name)
        return ret

    def isRef(self, elem, attr):
        """Determine whether an attribute is of type Ref. In case we
          have DTD(s) then this is simple, otherwise we use an
           heuristic: name Ref (upper or lowercase). """
        if elem is None: elem__o = None
        else: elem__o = elem._o
        if attr is None: attr__o = None
        else: attr__o = attr._o
        ret = libxml2mod.xmlIsRef(self._o, elem__o, attr__o)
        return ret

    def removeID(self, attr):
        """Remove the given attribute from the ID table maintained
           internally. """
        if attr is None: attr__o = None
        else: attr__o = attr._o
        ret = libxml2mod.xmlRemoveID(self._o, attr__o)
        return ret

    def removeRef(self, attr):
        """Remove the given attribute from the Ref table maintained
           internally. """
        if attr is None: attr__o = None
        else: attr__o = attr._o
        ret = libxml2mod.xmlRemoveRef(self._o, attr__o)
        return ret

    def validCtxtNormalizeAttributeValue(self, ctxt, elem, name, value):
        """Does the validation related extra step of the normalization
          of attribute values:  If the declared value is not CDATA,
          then the XML processor must further process the normalized
          attribute value by discarding any leading and trailing
          space (#x20) characters, and by replacing sequences of
          space (#x20) characters by single space (#x20) character.
          Also  check VC: Standalone Document Declaration in P32, and
           update ctxt->valid accordingly """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidCtxtNormalizeAttributeValue(ctxt__o, self._o, elem__o, name, value)
        return ret

    def validNormalizeAttributeValue(self, elem, name, value):
        """Does the validation related extra step of the normalization
          of attribute values:  If the declared value is not CDATA,
          then the XML processor must further process the normalized
          attribute value by discarding any leading and trailing
          space (#x20) characters, and by replacing sequences of
           space (#x20) characters by single space (#x20) character. """
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidNormalizeAttributeValue(self._o, elem__o, name, value)
        return ret

    def validateDocument(self, ctxt):
        """Try to validate the document instance  basically it does
          the all the checks described by the XML Rec i.e. validates
          the internal and external subset (if present) and validate
           the document tree. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlValidateDocument(ctxt__o, self._o)
        return ret

    def validateDocumentFinal(self, ctxt):
        """Does the final step for the document validation once all
          the incremental validation steps have been completed
          basically it does the following checks described by the XML
          Rec  Check all the IDREF/IDREFS attributes definition for
           validity """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlValidateDocumentFinal(ctxt__o, self._o)
        return ret

    def validateDtd(self, ctxt, dtd):
        """Try to validate the document against the dtd instance
          Basically it does check all the definitions in the DtD.
          Note the the internal subset (if present) is de-coupled
          (i.e. not used), which could give problems if ID or IDREF
           is present. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if dtd is None: dtd__o = None
        else: dtd__o = dtd._o
        ret = libxml2mod.xmlValidateDtd(ctxt__o, self._o, dtd__o)
        return ret

    def validateDtdFinal(self, ctxt):
        """Does the final step for the dtds validation once all the
          subsets have been parsed  basically it does the following
          checks described by the XML Rec - check that ENTITY and
          ENTITIES type attributes default or possible values matches
          one of the defined entities. - check that NOTATION type
          attributes default or possible values matches one of the
           defined notations. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlValidateDtdFinal(ctxt__o, self._o)
        return ret

    def validateElement(self, ctxt, elem):
        """Try to validate the subtree under an element """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidateElement(ctxt__o, self._o, elem__o)
        return ret

    def validateNotationUse(self, ctxt, notationName):
        """Validate that the given name match a notation declaration.
           - [ VC: Notation Declared ] """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlValidateNotationUse(ctxt__o, self._o, notationName)
        return ret

    def validateOneAttribute(self, ctxt, elem, attr, value):
        """Try to validate a single attribute for an element basically
          it does the following checks as described by the XML-1.0
          recommendation: - [ VC: Attribute Value Type ] - [ VC:
          Fixed Attribute Default ] - [ VC: Entity Name ] - [ VC:
          Name Token ] - [ VC: ID ] - [ VC: IDREF ] - [ VC: Entity
          Name ] - [ VC: Notation Attributes ]  The ID/IDREF
           uniqueness and matching are done separately """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        if attr is None: attr__o = None
        else: attr__o = attr._o
        ret = libxml2mod.xmlValidateOneAttribute(ctxt__o, self._o, elem__o, attr__o, value)
        return ret

    def validateOneElement(self, ctxt, elem):
        """Try to validate a single element and it's attributes,
          basically it does the following checks as described by the
          XML-1.0 recommendation: - [ VC: Element Valid ] - [ VC:
          Required Attribute ] Then call xmlValidateOneAttribute()
          for each attribute present.  The ID/IDREF checkings are
           done separately """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidateOneElement(ctxt__o, self._o, elem__o)
        return ret

    def validateOneNamespace(self, ctxt, elem, prefix, ns, value):
        """Try to validate a single namespace declaration for an
          element basically it does the following checks as described
          by the XML-1.0 recommendation: - [ VC: Attribute Value Type
          ] - [ VC: Fixed Attribute Default ] - [ VC: Entity Name ] -
          [ VC: Name Token ] - [ VC: ID ] - [ VC: IDREF ] - [ VC:
          Entity Name ] - [ VC: Notation Attributes ]  The ID/IDREF
           uniqueness and matching are done separately """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlValidateOneNamespace(ctxt__o, self._o, elem__o, prefix, ns__o, value)
        return ret

    def validatePopElement(self, ctxt, elem, qname):
        """Pop the element end from the validation stack. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidatePopElement(ctxt__o, self._o, elem__o, qname)
        return ret

    def validatePushElement(self, ctxt, elem, qname):
        """Push a new element start on the validation stack. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidatePushElement(ctxt__o, self._o, elem__o, qname)
        return ret

    def validateRoot(self, ctxt):
        """Try to validate a the root element basically it does the
          following check as described by the XML-1.0 recommendation:
          - [ VC: Root Element Type ] it doesn't try to recurse or
           apply other check to the element """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlValidateRoot(ctxt__o, self._o)
        return ret

    #
    # xmlDoc functions from module xinclude
    #

    def xincludeProcess(self):
        """Implement the XInclude substitution on the XML document @doc """
        ret = libxml2mod.xmlXIncludeProcess(self._o)
        return ret

    def xincludeProcessFlags(self, flags):
        """Implement the XInclude substitution on the XML document @doc """
        ret = libxml2mod.xmlXIncludeProcessFlags(self._o, flags)
        return ret

    #
    # xmlDoc functions from module xmlreader
    #

    def NewWalker(self, reader):
        """Setup an xmltextReader to parse a preparsed XML document.
           This reuses the existing @reader xmlTextReader. """
        if reader is None: reader__o = None
        else: reader__o = reader._o
        ret = libxml2mod.xmlReaderNewWalker(reader__o, self._o)
        return ret

    def readerWalker(self):
        """Create an xmltextReader for a preparsed document. """
        ret = libxml2mod.xmlReaderWalker(self._o)
        if ret is None:raise treeError('xmlReaderWalker() failed')
        __tmp = xmlTextReader(_obj=ret)
        return __tmp

    #
    # xmlDoc functions from module xmlschemas
    #

    def schemaNewDocParserCtxt(self):
        """Create an XML Schemas parse context for that document. NB.
           The document may be modified during the parsing process. """
        ret = libxml2mod.xmlSchemaNewDocParserCtxt(self._o)
        if ret is None:raise parserError('xmlSchemaNewDocParserCtxt() failed')
        __tmp = SchemaParserCtxt(_obj=ret)
        return __tmp

    def schemaValidateDoc(self, ctxt):
        """Validate a document tree in memory. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlSchemaValidateDoc(ctxt__o, self._o)
        return ret

    #
    # xmlDoc functions from module xpath
    #

    def xpathNewContext(self):
        """Create a new xmlXPathContext """
        ret = libxml2mod.xmlXPathNewContext(self._o)
        if ret is None:raise xpathError('xmlXPathNewContext() failed')
        __tmp = xpathContext(_obj=ret)
        return __tmp

    def xpathOrderDocElems(self):
        """Call this routine to speed up XPath computation on static
          documents. This stamps all the element nodes with the
          document order Like for line information, the order is kept
          in the element->content field, the value stored is actually
          - the node number (starting at -1) to be able to
           differentiate from line numbers. """
        ret = libxml2mod.xmlXPathOrderDocElems(self._o)
        return ret

    #
    # xmlDoc functions from module xpointer
    #

    def xpointerNewContext(self, here, origin):
        """Create a new XPointer context """
        if here is None: here__o = None
        else: here__o = here._o
        if origin is None: origin__o = None
        else: origin__o = origin._o
        ret = libxml2mod.xmlXPtrNewContext(self._o, here__o, origin__o)
        if ret is None:raise treeError('xmlXPtrNewContext() failed')
        __tmp = xpathContext(_obj=ret)
        return __tmp

class parserCtxt(parserCtxtCore):
    def __init__(self, _obj=None):
        self._o = _obj
        parserCtxtCore.__init__(self, _obj=_obj)

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeParserCtxt(self._o)
        self._o = None

    # accessors for parserCtxt
    def doc(self):
        """Get the document tree from a parser context. """
        ret = libxml2mod.xmlParserGetDoc(self._o)
        if ret is None:raise parserError('xmlParserGetDoc() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def isValid(self):
        """Get the validity information from a parser context. """
        ret = libxml2mod.xmlParserGetIsValid(self._o)
        return ret

    def lineNumbers(self, linenumbers):
        """Switch on the generation of line number for elements nodes. """
        libxml2mod.xmlParserSetLineNumbers(self._o, linenumbers)

    def loadSubset(self, loadsubset):
        """Switch the parser to load the DTD without validating. """
        libxml2mod.xmlParserSetLoadSubset(self._o, loadsubset)

    def pedantic(self, pedantic):
        """Switch the parser to be pedantic. """
        libxml2mod.xmlParserSetPedantic(self._o, pedantic)

    def replaceEntities(self, replaceEntities):
        """Switch the parser to replace entities. """
        libxml2mod.xmlParserSetReplaceEntities(self._o, replaceEntities)

    def validate(self, validate):
        """Switch the parser to validation mode. """
        libxml2mod.xmlParserSetValidate(self._o, validate)

    def wellFormed(self):
        """Get the well formed information from a parser context. """
        ret = libxml2mod.xmlParserGetWellFormed(self._o)
        return ret

    #
    # parserCtxt functions from module HTMLparser
    #

    def htmlCtxtReadDoc(self, cur, URL, encoding, options):
        """parse an XML in-memory document and build a tree. This
           reuses the existing @ctxt parser context """
        ret = libxml2mod.htmlCtxtReadDoc(self._o, cur, URL, encoding, options)
        if ret is None:raise treeError('htmlCtxtReadDoc() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def htmlCtxtReadFd(self, fd, URL, encoding, options):
        """parse an XML from a file descriptor and build a tree. This
           reuses the existing @ctxt parser context """
        ret = libxml2mod.htmlCtxtReadFd(self._o, fd, URL, encoding, options)
        if ret is None:raise treeError('htmlCtxtReadFd() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def htmlCtxtReadFile(self, filename, encoding, options):
        """parse an XML file from the filesystem or the network. This
           reuses the existing @ctxt parser context """
        ret = libxml2mod.htmlCtxtReadFile(self._o, filename, encoding, options)
        if ret is None:raise treeError('htmlCtxtReadFile() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def htmlCtxtReadMemory(self, buffer, size, URL, encoding, options):
        """parse an XML in-memory document and build a tree. This
           reuses the existing @ctxt parser context """
        ret = libxml2mod.htmlCtxtReadMemory(self._o, buffer, size, URL, encoding, options)
        if ret is None:raise treeError('htmlCtxtReadMemory() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def htmlCtxtReset(self):
        """Reset a parser context """
        libxml2mod.htmlCtxtReset(self._o)

    def htmlCtxtUseOptions(self, options):
        """Applies the options to the parser context """
        ret = libxml2mod.htmlCtxtUseOptions(self._o, options)
        return ret

    def htmlFreeParserCtxt(self):
        """Free all the memory used by a parser context. However the
           parsed document in ctxt->myDoc is not freed. """
        libxml2mod.htmlFreeParserCtxt(self._o)

    def htmlParseCharRef(self):
        """parse Reference declarations  [66] CharRef ::= '&#' [0-9]+
           ';' | '&#x' [0-9a-fA-F]+ ';' """
        ret = libxml2mod.htmlParseCharRef(self._o)
        return ret

    def htmlParseChunk(self, chunk, size, terminate):
        """Parse a Chunk of memory """
        ret = libxml2mod.htmlParseChunk(self._o, chunk, size, terminate)
        return ret

    def htmlParseDocument(self):
        """parse an HTML document (and build a tree if using the
           standard SAX interface). """
        ret = libxml2mod.htmlParseDocument(self._o)
        return ret

    def htmlParseElement(self):
        """parse an HTML element, this is highly recursive this is
          kept for compatibility with previous code versions  [39]
          element ::= EmptyElemTag | STag content ETag  [41]
           Attribute ::= Name Eq AttValue """
        libxml2mod.htmlParseElement(self._o)

    #
    # parserCtxt functions from module parser
    #

    def byteConsumed(self):
        """This function provides the current index of the parser
          relative to the start of the current entity. This function
          is computed in bytes from the beginning starting at zero
          and finishing at the size in byte of the file if parsing a
          file. The function is of constant cost if the input is
           UTF-8 but can be costly if run on non-UTF-8 input. """
        ret = libxml2mod.xmlByteConsumed(self._o)
        return ret

    def clearParserCtxt(self):
        """Clear (release owned resources) and reinitialize a parser
           context """
        libxml2mod.xmlClearParserCtxt(self._o)

    def ctxtReadDoc(self, cur, URL, encoding, options):
        """parse an XML in-memory document and build a tree. This
           reuses the existing @ctxt parser context """
        ret = libxml2mod.xmlCtxtReadDoc(self._o, cur, URL, encoding, options)
        if ret is None:raise treeError('xmlCtxtReadDoc() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def ctxtReadFd(self, fd, URL, encoding, options):
        """parse an XML from a file descriptor and build a tree. This
          reuses the existing @ctxt parser context NOTE that the file
          descriptor will not be closed when the reader is closed or
           reset. """
        ret = libxml2mod.xmlCtxtReadFd(self._o, fd, URL, encoding, options)
        if ret is None:raise treeError('xmlCtxtReadFd() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def ctxtReadFile(self, filename, encoding, options):
        """parse an XML file from the filesystem or the network. This
           reuses the existing @ctxt parser context """
        ret = libxml2mod.xmlCtxtReadFile(self._o, filename, encoding, options)
        if ret is None:raise treeError('xmlCtxtReadFile() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def ctxtReadMemory(self, buffer, size, URL, encoding, options):
        """parse an XML in-memory document and build a tree. This
           reuses the existing @ctxt parser context """
        ret = libxml2mod.xmlCtxtReadMemory(self._o, buffer, size, URL, encoding, options)
        if ret is None:raise treeError('xmlCtxtReadMemory() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def ctxtReset(self):
        """Reset a parser context """
        libxml2mod.xmlCtxtReset(self._o)

    def ctxtResetPush(self, chunk, size, filename, encoding):
        """Reset a push parser context """
        ret = libxml2mod.xmlCtxtResetPush(self._o, chunk, size, filename, encoding)
        return ret

    def ctxtUseOptions(self, options):
        """Applies the options to the parser context """
        ret = libxml2mod.xmlCtxtUseOptions(self._o, options)
        return ret

    def initParserCtxt(self):
        """Initialize a parser context """
        ret = libxml2mod.xmlInitParserCtxt(self._o)
        return ret

    def parseChunk(self, chunk, size, terminate):
        """Parse a Chunk of memory """
        ret = libxml2mod.xmlParseChunk(self._o, chunk, size, terminate)
        return ret

    def parseDocument(self):
        """parse an XML document (and build a tree if using the
          standard SAX interface).  [1] document ::= prolog element
           Misc*  [22] prolog ::= XMLDecl? Misc* (doctypedecl Misc*)? """
        ret = libxml2mod.xmlParseDocument(self._o)
        return ret

    def parseExtParsedEnt(self):
        """parse a general parsed entity An external general parsed
          entity is well-formed if it matches the production labeled
           extParsedEnt.  [78] extParsedEnt ::= TextDecl? content """
        ret = libxml2mod.xmlParseExtParsedEnt(self._o)
        return ret

    def setupParserForBuffer(self, buffer, filename):
        """Setup the parser context to parse a new buffer; Clears any
          prior contents from the parser context. The buffer
          parameter must not be None, but the filename parameter can
           be """
        libxml2mod.xmlSetupParserForBuffer(self._o, buffer, filename)

    def stopParser(self):
        """Blocks further parser processing """
        libxml2mod.xmlStopParser(self._o)

    #
    # parserCtxt functions from module parserInternals
    #

    def decodeEntities(self, len, what, end, end2, end3):
        """This function is deprecated, we now always process entities
          content through xmlStringDecodeEntities  TODO: remove it in
          next major release.  [67] Reference ::= EntityRef | CharRef
            [69] PEReference ::= '%' Name ';' """
        ret = libxml2mod.xmlDecodeEntities(self._o, len, what, end, end2, end3)
        return ret

    def handleEntity(self, entity):
        """Default handling of defined entities, when should we define
          a new input stream ? When do we just handle that as a set
           of chars ?  OBSOLETE: to be removed at some point. """
        if entity is None: entity__o = None
        else: entity__o = entity._o
        libxml2mod.xmlHandleEntity(self._o, entity__o)

    def namespaceParseNCName(self):
        """parse an XML namespace name.  TODO: this seems not in use
          anymore, the namespace handling is done on top of the SAX
          interfaces, i.e. not on raw input.  [NS 3] NCName ::=
          (Letter | '_') (NCNameChar)*  [NS 4] NCNameChar ::= Letter
           | Digit | '.' | '-' | '_' | CombiningChar | Extender """
        ret = libxml2mod.xmlNamespaceParseNCName(self._o)
        return ret

    def namespaceParseNSDef(self):
        """parse a namespace prefix declaration  TODO: this seems not
          in use anymore, the namespace handling is done on top of
          the SAX interfaces, i.e. not on raw input.  [NS 1] NSDef
          ::= PrefixDef Eq SystemLiteral  [NS 2] PrefixDef ::=
           'xmlns' (':' NCName)? """
        ret = libxml2mod.xmlNamespaceParseNSDef(self._o)
        return ret

    def nextChar(self):
        """Skip to the next char input char. """
        libxml2mod.xmlNextChar(self._o)

    def parseAttValue(self):
        """parse a value for an attribute Note: the parser won't do
          substitution of entities here, this will be handled later
          in xmlStringGetNodeList  [10] AttValue ::= '"' ([^<&"] |
          Reference)* '"' | "'" ([^<&'] | Reference)* "'"  3.3.3
          Attribute-Value Normalization: Before the value of an
          attribute is passed to the application or checked for
          validity, the XML processor must normalize it as follows: -
          a character reference is processed by appending the
          referenced character to the attribute value - an entity
          reference is processed by recursively processing the
          replacement text of the entity - a whitespace character
          (#x20, #xD, #xA, #x9) is processed by appending #x20 to the
          normalized value, except that only a single #x20 is
          appended for a "#xD#xA" sequence that is part of an
          external parsed entity or the literal entity value of an
          internal parsed entity - other characters are processed by
          appending them to the normalized value If the declared
          value is not CDATA, then the XML processor must further
          process the normalized attribute value by discarding any
          leading and trailing space (#x20) characters, and by
          replacing sequences of space (#x20) characters by a single
          space (#x20) character. All attributes for which no
          declaration has been read should be treated by a
           non-validating parser as if declared CDATA. """
        ret = libxml2mod.xmlParseAttValue(self._o)
        return ret

    def parseAttributeListDecl(self):
        """: parse the Attribute list def for an element  [52]
          AttlistDecl ::= '<!ATTLIST' S Name AttDef* S? '>'  [53]
           AttDef ::= S Name S AttType S DefaultDecl """
        libxml2mod.xmlParseAttributeListDecl(self._o)

    def parseCDSect(self):
        """Parse escaped pure raw content.  [18] CDSect ::= CDStart
          CData CDEnd  [19] CDStart ::= '<![CDATA['  [20] Data ::=
           (Char* - (Char* ']]>' Char*))  [21] CDEnd ::= ']]>' """
        libxml2mod.xmlParseCDSect(self._o)

    def parseCharData(self, cdata):
        """parse a CharData section. if we are within a CDATA section
          ']]>' marks an end of section.  The right angle bracket (>)
          may be represented using the string "&gt;", and must, for
          compatibility, be escaped using "&gt;" or a character
          reference when it appears in the string "]]>" in content,
          when that string is not marking the end of a CDATA section.
            [14] CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*) """
        libxml2mod.xmlParseCharData(self._o, cdata)

    def parseCharRef(self):
        """parse Reference declarations  [66] CharRef ::= '&#' [0-9]+
          ';' | '&#x' [0-9a-fA-F]+ ';'  [ WFC: Legal Character ]
          Characters referred to using character references must
           match the production for Char. """
        ret = libxml2mod.xmlParseCharRef(self._o)
        return ret

    def parseComment(self):
        """Skip an XML (SGML) comment <!-- .... --> The spec says that
          "For compatibility, the string "--" (double-hyphen) must
          not occur within comments. "  [15] Comment ::= '<!--'
           ((Char - '-') | ('-' (Char - '-')))* '-->' """
        libxml2mod.xmlParseComment(self._o)

    def parseContent(self):
        """Parse a content:  [43] content ::= (element | CharData |
           Reference | CDSect | PI | Comment)* """
        libxml2mod.xmlParseContent(self._o)

    def parseDocTypeDecl(self):
        """parse a DOCTYPE declaration  [28] doctypedecl ::=
          '<!DOCTYPE' S Name (S ExternalID)? S? ('[' (markupdecl |
          PEReference | S)* ']' S?)? '>'  [ VC: Root Element Type ]
          The Name in the document type declaration must match the
           element type of the root element. """
        libxml2mod.xmlParseDocTypeDecl(self._o)

    def parseElement(self):
        """parse an XML element, this is highly recursive  [39]
          element ::= EmptyElemTag | STag content ETag  [ WFC:
          Element Type Match ] The Name in an element's end-tag must
           match the element type in the start-tag. """
        libxml2mod.xmlParseElement(self._o)

    def parseElementDecl(self):
        """parse an Element declaration.  [45] elementdecl ::=
          '<!ELEMENT' S Name S contentspec S? '>'  [ VC: Unique
          Element Type Declaration ] No element type may be declared
           more than once """
        ret = libxml2mod.xmlParseElementDecl(self._o)
        return ret

    def parseEncName(self):
        """parse the XML encoding name  [81] EncName ::= [A-Za-z]
           ([A-Za-z0-9._] | '-')* """
        ret = libxml2mod.xmlParseEncName(self._o)
        return ret

    def parseEncodingDecl(self):
        """parse the XML encoding declaration  [80] EncodingDecl ::= S
          'encoding' Eq ('"' EncName '"' |  "'" EncName "'")  this
           setups the conversion filters. """
        ret = libxml2mod.xmlParseEncodingDecl(self._o)
        return ret

    def parseEndTag(self):
        """parse an end of tag  [42] ETag ::= '</' Name S? '>'  With
           namespace  [NS 9] ETag ::= '</' QName S? '>' """
        libxml2mod.xmlParseEndTag(self._o)

    def parseEntityDecl(self):
        """parse <!ENTITY declarations  [70] EntityDecl ::= GEDecl |
          PEDecl  [71] GEDecl ::= '<!ENTITY' S Name S EntityDef S?
          '>'  [72] PEDecl ::= '<!ENTITY' S '%' S Name S PEDef S? '>'
          [73] EntityDef ::= EntityValue | (ExternalID NDataDecl?)
          [74] PEDef ::= EntityValue | ExternalID  [76] NDataDecl ::=
          S 'NDATA' S Name  [ VC: Notation Declared ] The Name must
           match the declared name of a notation. """
        libxml2mod.xmlParseEntityDecl(self._o)

    def parseEntityRef(self):
        """parse ENTITY references declarations  [68] EntityRef ::=
          '&' Name ';'  [ WFC: Entity Declared ] In a document
          without any DTD, a document with only an internal DTD
          subset which contains no parameter entity references, or a
          document with "standalone='yes'", the Name given in the
          entity reference must match that in an entity declaration,
          except that well-formed documents need not declare any of
          the following entities: amp, lt, gt, apos, quot.  The
          declaration of a parameter entity must precede any
          reference to it.  Similarly, the declaration of a general
          entity must precede any reference to it which appears in a
          default value in an attribute-list declaration. Note that
          if entities are declared in the external subset or in
          external parameter entities, a non-validating processor is
          not obligated to read and process their declarations; for
          such documents, the rule that an entity must be declared is
          a well-formedness constraint only if standalone='yes'.  [
          WFC: Parsed Entity ] An entity reference must not contain
           the name of an unparsed entity """
        ret = libxml2mod.xmlParseEntityRef(self._o)
        if ret is None:raise parserError('xmlParseEntityRef() failed')
        __tmp = xmlEntity(_obj=ret)
        return __tmp

    def parseExternalSubset(self, ExternalID, SystemID):
        """parse Markup declarations from an external subset  [30]
          extSubset ::= textDecl? extSubsetDecl  [31] extSubsetDecl
           ::= (markupdecl | conditionalSect | PEReference | S) * """
        libxml2mod.xmlParseExternalSubset(self._o, ExternalID, SystemID)

    def parseMarkupDecl(self):
        """parse Markup declarations  [29] markupdecl ::= elementdecl
          | AttlistDecl | EntityDecl | NotationDecl | PI | Comment  [
          VC: Proper Declaration/PE Nesting ] Parameter-entity
          replacement text must be properly nested with markup
          declarations. That is to say, if either the first character
          or the last character of a markup declaration (markupdecl
          above) is contained in the replacement text for a
          parameter-entity reference, both must be contained in the
          same replacement text.  [ WFC: PEs in Internal Subset ] In
          the internal DTD subset, parameter-entity references can
          occur only where markup declarations can occur, not within
          markup declarations. (This does not apply to references
          that occur in external parameter entities or to the
           external subset.) """
        libxml2mod.xmlParseMarkupDecl(self._o)

    def parseMisc(self):
        """parse an XML Misc* optional field.  [27] Misc ::= Comment |
           PI |  S """
        libxml2mod.xmlParseMisc(self._o)

    def parseName(self):
        """parse an XML name.  [4] NameChar ::= Letter | Digit | '.' |
          '-' | '_' | ':' | CombiningChar | Extender  [5] Name ::=
          (Letter | '_' | ':') (NameChar)*  [6] Names ::= Name (#x20
           Name)* """
        ret = libxml2mod.xmlParseName(self._o)
        return ret

    def parseNamespace(self):
        """xmlParseNamespace: parse specific PI '<?namespace ...'
          constructs.  This is what the older xml-name Working Draft
          specified, a bunch of other stuff may still rely on it, so
          support is still here as if it was declared on the root of
          the Tree:-(  TODO: remove from library  To be removed at
           next drop of binary compatibility """
        libxml2mod.xmlParseNamespace(self._o)

    def parseNmtoken(self):
        """parse an XML Nmtoken.  [7] Nmtoken ::= (NameChar)+  [8]
           Nmtokens ::= Nmtoken (#x20 Nmtoken)* """
        ret = libxml2mod.xmlParseNmtoken(self._o)
        return ret

    def parseNotationDecl(self):
        """parse a notation declaration  [82] NotationDecl ::=
          '<!NOTATION' S Name S (ExternalID |  PublicID) S? '>'
          Hence there is actually 3 choices: 'PUBLIC' S PubidLiteral
          'PUBLIC' S PubidLiteral S SystemLiteral and 'SYSTEM' S
           SystemLiteral  See the NOTE on xmlParseExternalID(). """
        libxml2mod.xmlParseNotationDecl(self._o)

    def parsePEReference(self):
        """parse PEReference declarations The entity content is
          handled directly by pushing it's content as a new input
          stream.  [69] PEReference ::= '%' Name ';'  [ WFC: No
          Recursion ] A parsed entity must not contain a recursive
          reference to itself, either directly or indirectly.  [ WFC:
          Entity Declared ] In a document without any DTD, a document
          with only an internal DTD subset which contains no
          parameter entity references, or a document with
          "standalone='yes'", ...  ... The declaration of a parameter
          entity must precede any reference to it...  [ VC: Entity
          Declared ] In a document with an external subset or
          external parameter entities with "standalone='no'", ...
          ... The declaration of a parameter entity must precede any
          reference to it...  [ WFC: In DTD ] Parameter-entity
          references may only appear in the DTD. NOTE: misleading but
           this is handled. """
        libxml2mod.xmlParsePEReference(self._o)

    def parsePI(self):
        """parse an XML Processing Instruction.  [16] PI ::= '<?'
          PITarget (S (Char* - (Char* '?>' Char*)))? '?>'  The
           processing is transfered to SAX once parsed. """
        libxml2mod.xmlParsePI(self._o)

    def parsePITarget(self):
        """parse the name of a PI  [17] PITarget ::= Name - (('X' |
           'x') ('M' | 'm') ('L' | 'l')) """
        ret = libxml2mod.xmlParsePITarget(self._o)
        return ret

    def parsePubidLiteral(self):
        """parse an XML public literal  [12] PubidLiteral ::= '"'
           PubidChar* '"' | "'" (PubidChar - "'")* "'" """
        ret = libxml2mod.xmlParsePubidLiteral(self._o)
        return ret

    def parseQuotedString(self):
        """Parse and return a string between quotes or doublequotes
          TODO: Deprecated, to  be removed at next drop of binary
           compatibility """
        ret = libxml2mod.xmlParseQuotedString(self._o)
        return ret

    def parseReference(self):
        """parse and handle entity references in content, depending on
          the SAX interface, this may end-up in a call to character()
          if this is a CharRef, a predefined entity, if there is no
          reference() callback. or if the parser was asked to switch
           to that mode.  [67] Reference ::= EntityRef | CharRef """
        libxml2mod.xmlParseReference(self._o)

    def parseSDDecl(self):
        """parse the XML standalone declaration  [32] SDDecl ::= S
          'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' |
          'no')'"'))  [ VC: Standalone Document Declaration ] TODO
          The standalone document declaration must have the value
          "no" if any external markup declarations contain
          declarations of: - attributes with default values, if
          elements to which these attributes apply appear in the
          document without specifications of values for these
          attributes, or - entities (other than amp, lt, gt, apos,
          quot), if references to those entities appear in the
          document, or - attributes with values subject to
          normalization, where the attribute appears in the document
          with a value which will change as a result of
          normalization, or - element types with element content, if
          white space occurs directly within any instance of those
           types. """
        ret = libxml2mod.xmlParseSDDecl(self._o)
        return ret

    def parseStartTag(self):
        """parse a start of tag either for rule element or
          EmptyElement. In both case we don't parse the tag closing
          chars.  [40] STag ::= '<' Name (S Attribute)* S? '>'  [
          WFC: Unique Att Spec ] No attribute name may appear more
          than once in the same start-tag or empty-element tag.  [44]
          EmptyElemTag ::= '<' Name (S Attribute)* S? '/>'  [ WFC:
          Unique Att Spec ] No attribute name may appear more than
          once in the same start-tag or empty-element tag.  With
          namespace:  [NS 8] STag ::= '<' QName (S Attribute)* S? '>'
            [NS 10] EmptyElement ::= '<' QName (S Attribute)* S? '/>' """
        ret = libxml2mod.xmlParseStartTag(self._o)
        return ret

    def parseSystemLiteral(self):
        """parse an XML Literal  [11] SystemLiteral ::= ('"' [^"]*
           '"') | ("'" [^']* "'") """
        ret = libxml2mod.xmlParseSystemLiteral(self._o)
        return ret

    def parseTextDecl(self):
        """parse an XML declaration header for external entities  [77]
           TextDecl ::= '<?xml' VersionInfo? EncodingDecl S? '?>' """
        libxml2mod.xmlParseTextDecl(self._o)

    def parseVersionInfo(self):
        """parse the XML version.  [24] VersionInfo ::= S 'version' Eq
           (' VersionNum ' | " VersionNum ")  [25] Eq ::= S? '=' S? """
        ret = libxml2mod.xmlParseVersionInfo(self._o)
        return ret

    def parseVersionNum(self):
        """parse the XML version value.  [26] VersionNum ::= '1.'
           [0-9]+  In practice allow [0-9].[0-9]+ at that level """
        ret = libxml2mod.xmlParseVersionNum(self._o)
        return ret

    def parseXMLDecl(self):
        """parse an XML declaration header  [23] XMLDecl ::= '<?xml'
           VersionInfo EncodingDecl? SDDecl? S? '?>' """
        libxml2mod.xmlParseXMLDecl(self._o)

    def parserHandlePEReference(self):
        """[69] PEReference ::= '%' Name ';'  [ WFC: No Recursion ] A
          parsed entity must not contain a recursive reference to
          itself, either directly or indirectly.  [ WFC: Entity
          Declared ] In a document without any DTD, a document with
          only an internal DTD subset which contains no parameter
          entity references, or a document with "standalone='yes'",
          ...  ... The declaration of a parameter entity must precede
          any reference to it...  [ VC: Entity Declared ] In a
          document with an external subset or external parameter
          entities with "standalone='no'", ...  ... The declaration
          of a parameter entity must precede any reference to it...
          [ WFC: In DTD ] Parameter-entity references may only appear
          in the DTD. NOTE: misleading but this is handled.  A
          PEReference may have been detected in the current input
          stream the handling is done accordingly to
          http://www.w3.org/TR/REC-xml#entproc i.e. - Included in
          literal in entity values - Included as Parameter Entity
           reference within DTDs """
        libxml2mod.xmlParserHandlePEReference(self._o)

    def parserHandleReference(self):
        """TODO: Remove, now deprecated ... the test is done directly
          in the content parsing routines.  [67] Reference ::=
          EntityRef | CharRef  [68] EntityRef ::= '&' Name ';'  [
          WFC: Entity Declared ] the Name given in the entity
          reference must match that in an entity declaration, except
          that well-formed documents need not declare any of the
          following entities: amp, lt, gt, apos, quot.  [ WFC: Parsed
          Entity ] An entity reference must not contain the name of
          an unparsed entity  [66] CharRef ::= '&#' [0-9]+ ';' |
          '&#x' [0-9a-fA-F]+ ';'  A PEReference may have been
          detected in the current input stream the handling is done
           accordingly to http://www.w3.org/TR/REC-xml#entproc """
        libxml2mod.xmlParserHandleReference(self._o)

    def popInput(self):
        """xmlPopInput: the current input pointed by ctxt->input came
           to an end pop it and return the next char. """
        ret = libxml2mod.xmlPopInput(self._o)
        return ret

    def scanName(self):
        """Trickery: parse an XML name but without consuming the input
          flow Needed for rollback cases. Used only when parsing
          entities references.  TODO: seems deprecated now, only used
          in the default part of xmlParserHandleReference  [4]
          NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' |
          CombiningChar | Extender  [5] Name ::= (Letter | '_' | ':')
           (NameChar)*  [6] Names ::= Name (S Name)* """
        ret = libxml2mod.xmlScanName(self._o)
        return ret

    def skipBlankChars(self):
        """skip all blanks character found at that point in the input
          streams. It pops up finished entities in the process if
           allowable at that point. """
        ret = libxml2mod.xmlSkipBlankChars(self._o)
        return ret

    def stringDecodeEntities(self, str, what, end, end2, end3):
        """Takes a entity string content and process to do the
          adequate substitutions.  [67] Reference ::= EntityRef |
           CharRef  [69] PEReference ::= '%' Name ';' """
        ret = libxml2mod.xmlStringDecodeEntities(self._o, str, what, end, end2, end3)
        return ret

    def stringLenDecodeEntities(self, str, len, what, end, end2, end3):
        """Takes a entity string content and process to do the
          adequate substitutions.  [67] Reference ::= EntityRef |
           CharRef  [69] PEReference ::= '%' Name ';' """
        ret = libxml2mod.xmlStringLenDecodeEntities(self._o, str, len, what, end, end2, end3)
        return ret

class xmlAttr(xmlNode):
    def __init__(self, _obj=None):
        if checkWrapper(_obj) != 0:            raise TypeError('xmlAttr got a wrong wrapper object type')
        self._o = _obj
        xmlNode.__init__(self, _obj=_obj)

    def __repr__(self):
        return "<xmlAttr (%s) object at 0x%x>" % (self.name, int(pos_id (self)))

    #
    # xmlAttr functions from module debugXML
    #

    def debugDumpAttr(self, output, depth):
        """Dumps debug information for the attribute """
        libxml2mod.xmlDebugDumpAttr(output, self._o, depth)

    def debugDumpAttrList(self, output, depth):
        """Dumps debug information for the attribute list """
        libxml2mod.xmlDebugDumpAttrList(output, self._o, depth)

    #
    # xmlAttr functions from module tree
    #

    def copyProp(self, target):
        """Do a copy of the attribute. """
        if target is None: target__o = None
        else: target__o = target._o
        ret = libxml2mod.xmlCopyProp(target__o, self._o)
        if ret is None:raise treeError('xmlCopyProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def copyPropList(self, target):
        """Do a copy of an attribute list. """
        if target is None: target__o = None
        else: target__o = target._o
        ret = libxml2mod.xmlCopyPropList(target__o, self._o)
        if ret is None:raise treeError('xmlCopyPropList() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def freeProp(self):
        """Free one attribute, all the content is freed too """
        libxml2mod.xmlFreeProp(self._o)

    def freePropList(self):
        """Free a property and all its siblings, all the children are
           freed too. """
        libxml2mod.xmlFreePropList(self._o)

    def removeProp(self):
        """Unlink and free one attribute, all the content is freed too
           Note this doesn't work for namespace definition attributes """
        ret = libxml2mod.xmlRemoveProp(self._o)
        return ret

    #
    # xmlAttr functions from module valid
    #

    def removeID(self, doc):
        """Remove the given attribute from the ID table maintained
           internally. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlRemoveID(doc__o, self._o)
        return ret

    def removeRef(self, doc):
        """Remove the given attribute from the Ref table maintained
           internally. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlRemoveRef(doc__o, self._o)
        return ret

class xmlAttribute(xmlNode):
    def __init__(self, _obj=None):
        if checkWrapper(_obj) != 0:            raise TypeError('xmlAttribute got a wrong wrapper object type')
        self._o = _obj
        xmlNode.__init__(self, _obj=_obj)

    def __repr__(self):
        return "<xmlAttribute (%s) object at 0x%x>" % (self.name, int(pos_id (self)))

class catalog:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeCatalog(self._o)
        self._o = None

    #
    # catalog functions from module catalog
    #

    def add(self, type, orig, replace):
        """Add an entry in the catalog, it may overwrite existing but
           different entries. """
        ret = libxml2mod.xmlACatalogAdd(self._o, type, orig, replace)
        return ret

    def catalogIsEmpty(self):
        """Check is a catalog is empty """
        ret = libxml2mod.xmlCatalogIsEmpty(self._o)
        return ret

    def convertSGMLCatalog(self):
        """Convert all the SGML catalog entries as XML ones """
        ret = libxml2mod.xmlConvertSGMLCatalog(self._o)
        return ret

    def dump(self, out):
        """Dump the given catalog to the given file. """
        libxml2mod.xmlACatalogDump(self._o, out)

    def remove(self, value):
        """Remove an entry from the catalog """
        ret = libxml2mod.xmlACatalogRemove(self._o, value)
        return ret

    def resolve(self, pubID, sysID):
        """Do a complete resolution lookup of an External Identifier """
        ret = libxml2mod.xmlACatalogResolve(self._o, pubID, sysID)
        return ret

    def resolvePublic(self, pubID):
        """Try to lookup the catalog local reference associated to a
           public ID in that catalog """
        ret = libxml2mod.xmlACatalogResolvePublic(self._o, pubID)
        return ret

    def resolveSystem(self, sysID):
        """Try to lookup the catalog resource for a system ID """
        ret = libxml2mod.xmlACatalogResolveSystem(self._o, sysID)
        return ret

    def resolveURI(self, URI):
        """Do a complete resolution lookup of an URI """
        ret = libxml2mod.xmlACatalogResolveURI(self._o, URI)
        return ret

class xmlDtd(xmlNode):
    def __init__(self, _obj=None):
        if checkWrapper(_obj) != 0:            raise TypeError('xmlDtd got a wrong wrapper object type')
        self._o = _obj
        xmlNode.__init__(self, _obj=_obj)

    def __repr__(self):
        return "<xmlDtd (%s) object at 0x%x>" % (self.name, int(pos_id (self)))

    #
    # xmlDtd functions from module debugXML
    #

    def debugDumpDTD(self, output):
        """Dumps debug information for the DTD """
        libxml2mod.xmlDebugDumpDTD(output, self._o)

    #
    # xmlDtd functions from module tree
    #

    def copyDtd(self):
        """Do a copy of the dtd. """
        ret = libxml2mod.xmlCopyDtd(self._o)
        if ret is None:raise treeError('xmlCopyDtd() failed')
        __tmp = xmlDtd(_obj=ret)
        return __tmp

    def freeDtd(self):
        """Free a DTD structure. """
        libxml2mod.xmlFreeDtd(self._o)

    #
    # xmlDtd functions from module valid
    #

    def dtdAttrDesc(self, elem, name):
        """Search the DTD for the description of this attribute on
           this element. """
        ret = libxml2mod.xmlGetDtdAttrDesc(self._o, elem, name)
        if ret is None:raise treeError('xmlGetDtdAttrDesc() failed')
        __tmp = xmlAttribute(_obj=ret)
        return __tmp

    def dtdElementDesc(self, name):
        """Search the DTD for the description of this element """
        ret = libxml2mod.xmlGetDtdElementDesc(self._o, name)
        if ret is None:raise treeError('xmlGetDtdElementDesc() failed')
        __tmp = xmlElement(_obj=ret)
        return __tmp

    def dtdQAttrDesc(self, elem, name, prefix):
        """Search the DTD for the description of this qualified
           attribute on this element. """
        ret = libxml2mod.xmlGetDtdQAttrDesc(self._o, elem, name, prefix)
        if ret is None:raise treeError('xmlGetDtdQAttrDesc() failed')
        __tmp = xmlAttribute(_obj=ret)
        return __tmp

    def dtdQElementDesc(self, name, prefix):
        """Search the DTD for the description of this element """
        ret = libxml2mod.xmlGetDtdQElementDesc(self._o, name, prefix)
        if ret is None:raise treeError('xmlGetDtdQElementDesc() failed')
        __tmp = xmlElement(_obj=ret)
        return __tmp

class xmlElement(xmlNode):
    def __init__(self, _obj=None):
        if checkWrapper(_obj) != 0:            raise TypeError('xmlElement got a wrong wrapper object type')
        self._o = _obj
        xmlNode.__init__(self, _obj=_obj)

    def __repr__(self):
        return "<xmlElement (%s) object at 0x%x>" % (self.name, int(pos_id (self)))

class xmlEntity(xmlNode):
    def __init__(self, _obj=None):
        if checkWrapper(_obj) != 0:            raise TypeError('xmlEntity got a wrong wrapper object type')
        self._o = _obj
        xmlNode.__init__(self, _obj=_obj)

    def __repr__(self):
        return "<xmlEntity (%s) object at 0x%x>" % (self.name, int(pos_id (self)))

    #
    # xmlEntity functions from module parserInternals
    #

    def handleEntity(self, ctxt):
        """Default handling of defined entities, when should we define
          a new input stream ? When do we just handle that as a set
           of chars ?  OBSOLETE: to be removed at some point. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        libxml2mod.xmlHandleEntity(ctxt__o, self._o)

class Error:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    # accessors for Error
    def code(self):
        """The error code, e.g. an xmlParserError """
        ret = libxml2mod.xmlErrorGetCode(self._o)
        return ret

    def domain(self):
        """What part of the library raised this error """
        ret = libxml2mod.xmlErrorGetDomain(self._o)
        return ret

    def file(self):
        """the filename """
        ret = libxml2mod.xmlErrorGetFile(self._o)
        return ret

    def level(self):
        """how consequent is the error """
        ret = libxml2mod.xmlErrorGetLevel(self._o)
        return ret

    def line(self):
        """the line number if available """
        ret = libxml2mod.xmlErrorGetLine(self._o)
        return ret

    def message(self):
        """human-readable informative error message """
        ret = libxml2mod.xmlErrorGetMessage(self._o)
        return ret

    #
    # Error functions from module xmlerror
    #

    def copyError(self, to):
        """Save the original error to the new place. """
        if to is None: to__o = None
        else: to__o = to._o
        ret = libxml2mod.xmlCopyError(self._o, to__o)
        return ret

    def resetError(self):
        """Cleanup the error. """
        libxml2mod.xmlResetError(self._o)

class xmlNs(xmlNode):
    def __init__(self, _obj=None):
        if checkWrapper(_obj) != 0:            raise TypeError('xmlNs got a wrong wrapper object type')
        self._o = _obj
        xmlNode.__init__(self, _obj=_obj)

    def __repr__(self):
        return "<xmlNs (%s) object at 0x%x>" % (self.name, int(pos_id (self)))

    #
    # xmlNs functions from module tree
    #

    def copyNamespace(self):
        """Do a copy of the namespace. """
        ret = libxml2mod.xmlCopyNamespace(self._o)
        if ret is None:raise treeError('xmlCopyNamespace() failed')
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def copyNamespaceList(self):
        """Do a copy of an namespace list. """
        ret = libxml2mod.xmlCopyNamespaceList(self._o)
        if ret is None:raise treeError('xmlCopyNamespaceList() failed')
        __tmp = xmlNs(_obj=ret)
        return __tmp

    def freeNs(self):
        """Free up the structures associated to a namespace """
        libxml2mod.xmlFreeNs(self._o)

    def freeNsList(self):
        """Free up all the structures associated to the chained
           namespaces. """
        libxml2mod.xmlFreeNsList(self._o)

    def newChild(self, parent, name, content):
        """Creation of a new child element, added at the end of
          @parent children list. @ns and @content parameters are
          optional (None). If @ns is None, the newly created element
          inherits the namespace of @parent. If @content is non None,
          a child list containing the TEXTs and ENTITY_REFs node will
          be created. NOTE: @content is supposed to be a piece of XML
          CDATA, so it allows entity references. XML special chars
          must be escaped first by using
          xmlEncodeEntitiesReentrant(), or xmlNewTextChild() should
           be used. """
        if parent is None: parent__o = None
        else: parent__o = parent._o
        ret = libxml2mod.xmlNewChild(parent__o, self._o, name, content)
        if ret is None:raise treeError('xmlNewChild() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocNode(self, doc, name, content):
        """Creation of a new node element within a document. @ns and
          @content are optional (None). NOTE: @content is supposed to
          be a piece of XML CDATA, so it allow entities references,
          but XML special chars need to be escaped first by using
          xmlEncodeEntitiesReentrant(). Use xmlNewDocRawNode() if you
           don't need entities support. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlNewDocNode(doc__o, self._o, name, content)
        if ret is None:raise treeError('xmlNewDocNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocNodeEatName(self, doc, name, content):
        """Creation of a new node element within a document. @ns and
          @content are optional (None). NOTE: @content is supposed to
          be a piece of XML CDATA, so it allow entities references,
          but XML special chars need to be escaped first by using
          xmlEncodeEntitiesReentrant(). Use xmlNewDocRawNode() if you
           don't need entities support. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlNewDocNodeEatName(doc__o, self._o, name, content)
        if ret is None:raise treeError('xmlNewDocNodeEatName() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newDocRawNode(self, doc, name, content):
        """Creation of a new node element within a document. @ns and
           @content are optional (None). """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlNewDocRawNode(doc__o, self._o, name, content)
        if ret is None:raise treeError('xmlNewDocRawNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newNodeEatName(self, name):
        """Creation of a new node element. @ns is optional (None). """
        ret = libxml2mod.xmlNewNodeEatName(self._o, name)
        if ret is None:raise treeError('xmlNewNodeEatName() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def newNsProp(self, node, name, value):
        """Create a new property tagged with a namespace and carried
           by a node. """
        if node is None: node__o = None
        else: node__o = node._o
        ret = libxml2mod.xmlNewNsProp(node__o, self._o, name, value)
        if ret is None:raise treeError('xmlNewNsProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def newNsPropEatName(self, node, name, value):
        """Create a new property tagged with a namespace and carried
           by a node. """
        if node is None: node__o = None
        else: node__o = node._o
        ret = libxml2mod.xmlNewNsPropEatName(node__o, self._o, name, value)
        if ret is None:raise treeError('xmlNewNsPropEatName() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def newTextChild(self, parent, name, content):
        """Creation of a new child element, added at the end of
          @parent children list. @ns and @content parameters are
          optional (None). If @ns is None, the newly created element
          inherits the namespace of @parent. If @content is non None,
          a child TEXT node will be created containing the string
          @content. NOTE: Use xmlNewChild() if @content will contain
          entities that need to be preserved. Use this function,
          xmlNewTextChild(), if you need to ensure that reserved XML
          chars that might appear in @content, such as the ampersand,
          greater-than or less-than signs, are automatically replaced
           by their XML escaped entity representations. """
        if parent is None: parent__o = None
        else: parent__o = parent._o
        ret = libxml2mod.xmlNewTextChild(parent__o, self._o, name, content)
        if ret is None:raise treeError('xmlNewTextChild() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def setNs(self, node):
        """Associate a namespace to a node, a posteriori. """
        if node is None: node__o = None
        else: node__o = node._o
        libxml2mod.xmlSetNs(node__o, self._o)

    def setNsProp(self, node, name, value):
        """Set (or reset) an attribute carried by a node. The ns
           structure must be in scope, this is not checked """
        if node is None: node__o = None
        else: node__o = node._o
        ret = libxml2mod.xmlSetNsProp(node__o, self._o, name, value)
        if ret is None:raise treeError('xmlSetNsProp() failed')
        __tmp = xmlAttr(_obj=ret)
        return __tmp

    def unsetNsProp(self, node, name):
        """Remove an attribute carried by a node. """
        if node is None: node__o = None
        else: node__o = node._o
        ret = libxml2mod.xmlUnsetNsProp(node__o, self._o, name)
        return ret

    #
    # xmlNs functions from module xpathInternals
    #

    def xpathNodeSetFreeNs(self):
        """Namespace nodes in libxml don't match the XPath semantic.
          In a node set the namespace nodes are duplicated and the
          next pointer is set to the parent node in the XPath
           semantic. Check if such a node needs to be freed """
        libxml2mod.xmlXPathNodeSetFreeNs(self._o)

class outputBuffer(ioWriteWrapper):
    def __init__(self, _obj=None):
        self._o = _obj
        ioWriteWrapper.__init__(self, _obj=_obj)

    #
    # outputBuffer functions from module HTMLtree
    #

    def htmlDocContentDumpFormatOutput(self, cur, encoding, format):
        """Dump an HTML document. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.htmlDocContentDumpFormatOutput(self._o, cur__o, encoding, format)

    def htmlDocContentDumpOutput(self, cur, encoding):
        """Dump an HTML document. Formating return/spaces are added. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.htmlDocContentDumpOutput(self._o, cur__o, encoding)

    def htmlNodeDumpFormatOutput(self, doc, cur, encoding, format):
        """Dump an HTML node, recursive behaviour,children are printed
           too. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.htmlNodeDumpFormatOutput(self._o, doc__o, cur__o, encoding, format)

    def htmlNodeDumpOutput(self, doc, cur, encoding):
        """Dump an HTML node, recursive behaviour,children are printed
           too, and formatting returns/spaces are added. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.htmlNodeDumpOutput(self._o, doc__o, cur__o, encoding)

    #
    # outputBuffer functions from module tree
    #

    def nodeDumpOutput(self, doc, cur, level, format, encoding):
        """Dump an XML node, recursive behaviour, children are printed
          too. Note that @format = 1 provide node indenting only if
          xmlIndentTreeOutput = 1 or xmlKeepBlanksDefault(0) was
           called """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if cur is None: cur__o = None
        else: cur__o = cur._o
        libxml2mod.xmlNodeDumpOutput(self._o, doc__o, cur__o, level, format, encoding)

    def saveFileTo(self, cur, encoding):
        """Dump an XML document to an I/O buffer. Warning ! This call
          xmlOutputBufferClose() on buf which is not available after
           this call. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlSaveFileTo(self._o, cur__o, encoding)
        return ret

    def saveFormatFileTo(self, cur, encoding, format):
        """Dump an XML document to an I/O buffer. Warning ! This call
          xmlOutputBufferClose() on buf which is not available after
           this call. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlSaveFormatFileTo(self._o, cur__o, encoding, format)
        return ret

    #
    # outputBuffer functions from module xmlIO
    #

    def getContent(self):
        """Gives a pointer to the data currently held in the output
           buffer """
        ret = libxml2mod.xmlOutputBufferGetContent(self._o)
        return ret

    def write(self, len, buf):
        """Write the content of the array in the output I/O buffer
          This routine handle the I18N transcoding from internal
          UTF-8 The buffer is lossless, i.e. will store in case of
           partial or delayed writes. """
        ret = libxml2mod.xmlOutputBufferWrite(self._o, len, buf)
        return ret

    def writeString(self, str):
        """Write the content of the string in the output I/O buffer
          This routine handle the I18N transcoding from internal
          UTF-8 The buffer is lossless, i.e. will store in case of
           partial or delayed writes. """
        ret = libxml2mod.xmlOutputBufferWriteString(self._o, str)
        return ret

class inputBuffer(ioReadWrapper):
    def __init__(self, _obj=None):
        self._o = _obj
        ioReadWrapper.__init__(self, _obj=_obj)

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeParserInputBuffer(self._o)
        self._o = None

    #
    # inputBuffer functions from module xmlIO
    #

    def grow(self, len):
        """Grow up the content of the input buffer, the old data are
          preserved This routine handle the I18N transcoding to
          internal UTF-8 This routine is used when operating the
          parser in normal (pull) mode  TODO: one should be able to
          remove one extra copy by copying directly onto in->buffer
           or in->raw """
        ret = libxml2mod.xmlParserInputBufferGrow(self._o, len)
        return ret

    def push(self, len, buf):
        """Push the content of the arry in the input buffer This
          routine handle the I18N transcoding to internal UTF-8 This
          is used when operating the parser in progressive (push)
           mode. """
        ret = libxml2mod.xmlParserInputBufferPush(self._o, len, buf)
        return ret

    def read(self, len):
        """Refresh the content of the input buffer, the old data are
          considered consumed This routine handle the I18N
           transcoding to internal UTF-8 """
        ret = libxml2mod.xmlParserInputBufferRead(self._o, len)
        return ret

    #
    # inputBuffer functions from module xmlreader
    #

    def Setup(self, reader, URL, encoding, options):
        """Setup an XML reader with new options """
        if reader is None: reader__o = None
        else: reader__o = reader._o
        ret = libxml2mod.xmlTextReaderSetup(reader__o, self._o, URL, encoding, options)
        return ret

    def newTextReader(self, URI):
        """Create an xmlTextReader structure fed with @input """
        ret = libxml2mod.xmlNewTextReader(self._o, URI)
        if ret is None:raise treeError('xmlNewTextReader() failed')
        __tmp = xmlTextReader(_obj=ret)
        __tmp.input = self
        return __tmp

class xmlReg:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlRegFreeRegexp(self._o)
        self._o = None

    #
    # xmlReg functions from module xmlregexp
    #

    def regexpExec(self, content):
        """Check if the regular expression generates the value """
        ret = libxml2mod.xmlRegexpExec(self._o, content)
        return ret

    def regexpIsDeterminist(self):
        """Check if the regular expression is determinist """
        ret = libxml2mod.xmlRegexpIsDeterminist(self._o)
        return ret

    def regexpPrint(self, output):
        """Print the content of the compiled regular expression """
        libxml2mod.xmlRegexpPrint(output, self._o)

class relaxNgParserCtxt:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlRelaxNGFreeParserCtxt(self._o)
        self._o = None

    #
    # relaxNgParserCtxt functions from module relaxng
    #

    def relaxNGParse(self):
        """parse a schema definition resource and build an internal
           XML Shema struture which can be used to validate instances. """
        ret = libxml2mod.xmlRelaxNGParse(self._o)
        if ret is None:raise parserError('xmlRelaxNGParse() failed')
        __tmp = relaxNgSchema(_obj=ret)
        return __tmp

    def relaxParserSetFlag(self, flags):
        """Semi private function used to pass informations to a parser
           context which are a combination of xmlRelaxNGParserFlag . """
        ret = libxml2mod.xmlRelaxParserSetFlag(self._o, flags)
        return ret

class relaxNgSchema:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlRelaxNGFree(self._o)
        self._o = None

    #
    # relaxNgSchema functions from module relaxng
    #

    def relaxNGDump(self, output):
        """Dump a RelaxNG structure back """
        libxml2mod.xmlRelaxNGDump(output, self._o)

    def relaxNGDumpTree(self, output):
        """Dump the transformed RelaxNG tree. """
        libxml2mod.xmlRelaxNGDumpTree(output, self._o)

    def relaxNGNewValidCtxt(self):
        """Create an XML RelaxNGs validation context based on the
           given schema """
        ret = libxml2mod.xmlRelaxNGNewValidCtxt(self._o)
        if ret is None:raise treeError('xmlRelaxNGNewValidCtxt() failed')
        __tmp = relaxNgValidCtxt(_obj=ret)
        __tmp.schema = self
        return __tmp

    #
    # relaxNgSchema functions from module xmlreader
    #

    def RelaxNGSetSchema(self, reader):
        """Use RelaxNG to validate the document as it is processed.
          Activation is only possible before the first Read(). if
          @schema is None, then RelaxNG validation is desactivated. @
          The @schema should not be freed until the reader is
           deallocated or its use has been deactivated. """
        if reader is None: reader__o = None
        else: reader__o = reader._o
        ret = libxml2mod.xmlTextReaderRelaxNGSetSchema(reader__o, self._o)
        return ret

class relaxNgValidCtxt(relaxNgValidCtxtCore):
    def __init__(self, _obj=None):
        self.schema = None
        self._o = _obj
        relaxNgValidCtxtCore.__init__(self, _obj=_obj)

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlRelaxNGFreeValidCtxt(self._o)
        self._o = None

    #
    # relaxNgValidCtxt functions from module relaxng
    #

    def relaxNGValidateDoc(self, doc):
        """Validate a document tree in memory. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlRelaxNGValidateDoc(self._o, doc__o)
        return ret

    def relaxNGValidateFullElement(self, doc, elem):
        """Validate a full subtree when
          xmlRelaxNGValidatePushElement() returned 0 and the content
           of the node has been expanded. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlRelaxNGValidateFullElement(self._o, doc__o, elem__o)
        return ret

    def relaxNGValidatePopElement(self, doc, elem):
        """Pop the element end from the RelaxNG validation stack. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlRelaxNGValidatePopElement(self._o, doc__o, elem__o)
        return ret

    def relaxNGValidatePushCData(self, data, len):
        """check the CData parsed for validation in the current stack """
        ret = libxml2mod.xmlRelaxNGValidatePushCData(self._o, data, len)
        return ret

    def relaxNGValidatePushElement(self, doc, elem):
        """Push a new element start on the RelaxNG validation stack. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlRelaxNGValidatePushElement(self._o, doc__o, elem__o)
        return ret

    #
    # relaxNgValidCtxt functions from module xmlreader
    #

    def RelaxNGValidateCtxt(self, reader, options):
        """Use RelaxNG schema context to validate the document as it
          is processed. Activation is only possible before the first
          Read(). If @ctxt is None, then RelaxNG schema validation is
           deactivated. """
        if reader is None: reader__o = None
        else: reader__o = reader._o
        ret = libxml2mod.xmlTextReaderRelaxNGValidateCtxt(reader__o, self._o, options)
        return ret

class SchemaParserCtxt:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlSchemaFreeParserCtxt(self._o)
        self._o = None

    #
    # SchemaParserCtxt functions from module xmlschemas
    #

    def schemaParse(self):
        """parse a schema definition resource and build an internal
           XML Shema struture which can be used to validate instances. """
        ret = libxml2mod.xmlSchemaParse(self._o)
        if ret is None:raise parserError('xmlSchemaParse() failed')
        __tmp = Schema(_obj=ret)
        return __tmp

class Schema:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlSchemaFree(self._o)
        self._o = None

    #
    # Schema functions from module xmlreader
    #

    def SetSchema(self, reader):
        """Use XSD Schema to validate the document as it is processed.
          Activation is only possible before the first Read(). if
          @schema is None, then Schema validation is desactivated. @
          The @schema should not be freed until the reader is
           deallocated or its use has been deactivated. """
        if reader is None: reader__o = None
        else: reader__o = reader._o
        ret = libxml2mod.xmlTextReaderSetSchema(reader__o, self._o)
        return ret

    #
    # Schema functions from module xmlschemas
    #

    def schemaDump(self, output):
        """Dump a Schema structure. """
        libxml2mod.xmlSchemaDump(output, self._o)

    def schemaNewValidCtxt(self):
        """Create an XML Schemas validation context based on the given
           schema. """
        ret = libxml2mod.xmlSchemaNewValidCtxt(self._o)
        if ret is None:raise treeError('xmlSchemaNewValidCtxt() failed')
        __tmp = SchemaValidCtxt(_obj=ret)
        __tmp.schema = self
        return __tmp

class SchemaValidCtxt(SchemaValidCtxtCore):
    def __init__(self, _obj=None):
        self.schema = None
        self._o = _obj
        SchemaValidCtxtCore.__init__(self, _obj=_obj)

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlSchemaFreeValidCtxt(self._o)
        self._o = None

    #
    # SchemaValidCtxt functions from module xmlreader
    #

    def SchemaValidateCtxt(self, reader, options):
        """Use W3C XSD schema context to validate the document as it
          is processed. Activation is only possible before the first
          Read(). If @ctxt is None, then XML Schema validation is
           deactivated. """
        if reader is None: reader__o = None
        else: reader__o = reader._o
        ret = libxml2mod.xmlTextReaderSchemaValidateCtxt(reader__o, self._o, options)
        return ret

    #
    # SchemaValidCtxt functions from module xmlschemas
    #

    def schemaIsValid(self):
        """Check if any error was detected during validation. """
        ret = libxml2mod.xmlSchemaIsValid(self._o)
        return ret

    def schemaSetValidOptions(self, options):
        """Sets the options to be used during the validation. """
        ret = libxml2mod.xmlSchemaSetValidOptions(self._o, options)
        return ret

    def schemaValidCtxtGetOptions(self):
        """Get the validation context options. """
        ret = libxml2mod.xmlSchemaValidCtxtGetOptions(self._o)
        return ret

    def schemaValidCtxtGetParserCtxt(self):
        """allow access to the parser context of the schema validation
           context """
        ret = libxml2mod.xmlSchemaValidCtxtGetParserCtxt(self._o)
        if ret is None:raise parserError('xmlSchemaValidCtxtGetParserCtxt() failed')
        __tmp = parserCtxt(_obj=ret)
        return __tmp

    def schemaValidateDoc(self, doc):
        """Validate a document tree in memory. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlSchemaValidateDoc(self._o, doc__o)
        return ret

    def schemaValidateFile(self, filename, options):
        """Do a schemas validation of the given resource, it will use
           the SAX streamable validation internally. """
        ret = libxml2mod.xmlSchemaValidateFile(self._o, filename, options)
        return ret

    def schemaValidateOneElement(self, elem):
        """Validate a branch of a tree, starting with the given @elem. """
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlSchemaValidateOneElement(self._o, elem__o)
        return ret

    def schemaValidateSetFilename(self, filename):
        """Workaround to provide file error reporting information when
           this is not provided by current APIs """
        libxml2mod.xmlSchemaValidateSetFilename(self._o, filename)

class xmlTextReaderLocator:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    #
    # xmlTextReaderLocator functions from module xmlreader
    #

    def BaseURI(self):
        """Obtain the base URI for the given locator. """
        ret = libxml2mod.xmlTextReaderLocatorBaseURI(self._o)
        return ret

    def LineNumber(self):
        """Obtain the line number for the given locator. """
        ret = libxml2mod.xmlTextReaderLocatorLineNumber(self._o)
        return ret

class xmlTextReader(xmlTextReaderCore):
    def __init__(self, _obj=None):
        self.input = None
        self._o = _obj
        xmlTextReaderCore.__init__(self, _obj=_obj)

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeTextReader(self._o)
        self._o = None

    #
    # xmlTextReader functions from module xmlreader
    #

    def AttributeCount(self):
        """Provides the number of attributes of the current node """
        ret = libxml2mod.xmlTextReaderAttributeCount(self._o)
        return ret

    def BaseUri(self):
        """The base URI of the node. """
        ret = libxml2mod.xmlTextReaderConstBaseUri(self._o)
        return ret

    def ByteConsumed(self):
        """This function provides the current index of the parser used
          by the reader, relative to the start of the current entity.
          This function actually just wraps a call to
          xmlBytesConsumed() for the parser context associated with
           the reader. See xmlBytesConsumed() for more information. """
        ret = libxml2mod.xmlTextReaderByteConsumed(self._o)
        return ret

    def Close(self):
        """This method releases any resources allocated by the current
          instance changes the state to Closed and close any
           underlying input. """
        ret = libxml2mod.xmlTextReaderClose(self._o)
        return ret

    def CurrentDoc(self):
        """Hacking interface allowing to get the xmlDocPtr
          correponding to the current document being accessed by the
          xmlTextReader. NOTE: as a result of this call, the reader
          will not destroy the associated XML document and calling
          xmlFreeDoc() on the result is needed once the reader
           parsing has finished. """
        ret = libxml2mod.xmlTextReaderCurrentDoc(self._o)
        if ret is None:raise treeError('xmlTextReaderCurrentDoc() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def CurrentNode(self):
        """Hacking interface allowing to get the xmlNodePtr
          correponding to the current node being accessed by the
          xmlTextReader. This is dangerous because the underlying
           node may be destroyed on the next Reads. """
        ret = libxml2mod.xmlTextReaderCurrentNode(self._o)
        if ret is None:raise treeError('xmlTextReaderCurrentNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def Depth(self):
        """The depth of the node in the tree. """
        ret = libxml2mod.xmlTextReaderDepth(self._o)
        return ret

    def Encoding(self):
        """Determine the encoding of the document being read. """
        ret = libxml2mod.xmlTextReaderConstEncoding(self._o)
        return ret

    def Expand(self):
        """Reads the contents of the current node and the full
          subtree. It then makes the subtree available until the next
           xmlTextReaderRead() call """
        ret = libxml2mod.xmlTextReaderExpand(self._o)
        if ret is None:raise treeError('xmlTextReaderExpand() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def GetAttribute(self, name):
        """Provides the value of the attribute with the specified
           qualified name. """
        ret = libxml2mod.xmlTextReaderGetAttribute(self._o, name)
        return ret

    def GetAttributeNo(self, no):
        """Provides the value of the attribute with the specified
           index relative to the containing element. """
        ret = libxml2mod.xmlTextReaderGetAttributeNo(self._o, no)
        return ret

    def GetAttributeNs(self, localName, namespaceURI):
        """Provides the value of the specified attribute """
        ret = libxml2mod.xmlTextReaderGetAttributeNs(self._o, localName, namespaceURI)
        return ret

    def GetParserColumnNumber(self):
        """Provide the column number of the current parsing point. """
        ret = libxml2mod.xmlTextReaderGetParserColumnNumber(self._o)
        return ret

    def GetParserLineNumber(self):
        """Provide the line number of the current parsing point. """
        ret = libxml2mod.xmlTextReaderGetParserLineNumber(self._o)
        return ret

    def GetParserProp(self, prop):
        """Read the parser internal property. """
        ret = libxml2mod.xmlTextReaderGetParserProp(self._o, prop)
        return ret

    def GetRemainder(self):
        """Method to get the remainder of the buffered XML. this
          method stops the parser, set its state to End Of File and
          return the input stream with what is left that the parser
          did not use.  The implementation is not good, the parser
          certainly procgressed past what's left in reader->input,
          and there is an allocation problem. Best would be to
           rewrite it differently. """
        ret = libxml2mod.xmlTextReaderGetRemainder(self._o)
        if ret is None:raise treeError('xmlTextReaderGetRemainder() failed')
        __tmp = inputBuffer(_obj=ret)
        return __tmp

    def HasAttributes(self):
        """Whether the node has attributes. """
        ret = libxml2mod.xmlTextReaderHasAttributes(self._o)
        return ret

    def HasValue(self):
        """Whether the node can have a text value. """
        ret = libxml2mod.xmlTextReaderHasValue(self._o)
        return ret

    def IsDefault(self):
        """Whether an Attribute  node was generated from the default
           value defined in the DTD or schema. """
        ret = libxml2mod.xmlTextReaderIsDefault(self._o)
        return ret

    def IsEmptyElement(self):
        """Check if the current node is empty """
        ret = libxml2mod.xmlTextReaderIsEmptyElement(self._o)
        return ret

    def IsNamespaceDecl(self):
        """Determine whether the current node is a namespace
           declaration rather than a regular attribute. """
        ret = libxml2mod.xmlTextReaderIsNamespaceDecl(self._o)
        return ret

    def IsValid(self):
        """Retrieve the validity status from the parser context """
        ret = libxml2mod.xmlTextReaderIsValid(self._o)
        return ret

    def LocalName(self):
        """The local name of the node. """
        ret = libxml2mod.xmlTextReaderConstLocalName(self._o)
        return ret

    def LookupNamespace(self, prefix):
        """Resolves a namespace prefix in the scope of the current
           element. """
        ret = libxml2mod.xmlTextReaderLookupNamespace(self._o, prefix)
        return ret

    def MoveToAttribute(self, name):
        """Moves the position of the current instance to the attribute
           with the specified qualified name. """
        ret = libxml2mod.xmlTextReaderMoveToAttribute(self._o, name)
        return ret

    def MoveToAttributeNo(self, no):
        """Moves the position of the current instance to the attribute
          with the specified index relative to the containing element. """
        ret = libxml2mod.xmlTextReaderMoveToAttributeNo(self._o, no)
        return ret

    def MoveToAttributeNs(self, localName, namespaceURI):
        """Moves the position of the current instance to the attribute
           with the specified local name and namespace URI. """
        ret = libxml2mod.xmlTextReaderMoveToAttributeNs(self._o, localName, namespaceURI)
        return ret

    def MoveToElement(self):
        """Moves the position of the current instance to the node that
           contains the current Attribute  node. """
        ret = libxml2mod.xmlTextReaderMoveToElement(self._o)
        return ret

    def MoveToFirstAttribute(self):
        """Moves the position of the current instance to the first
           attribute associated with the current node. """
        ret = libxml2mod.xmlTextReaderMoveToFirstAttribute(self._o)
        return ret

    def MoveToNextAttribute(self):
        """Moves the position of the current instance to the next
           attribute associated with the current node. """
        ret = libxml2mod.xmlTextReaderMoveToNextAttribute(self._o)
        return ret

    def Name(self):
        """The qualified name of the node, equal to Prefix :LocalName. """
        ret = libxml2mod.xmlTextReaderConstName(self._o)
        return ret

    def NamespaceUri(self):
        """The URI defining the namespace associated with the node. """
        ret = libxml2mod.xmlTextReaderConstNamespaceUri(self._o)
        return ret

    def NewDoc(self, cur, URL, encoding, options):
        """Setup an xmltextReader to parse an XML in-memory document.
          The parsing flags @options are a combination of
          xmlParserOption. This reuses the existing @reader
           xmlTextReader. """
        ret = libxml2mod.xmlReaderNewDoc(self._o, cur, URL, encoding, options)
        return ret

    def NewFd(self, fd, URL, encoding, options):
        """Setup an xmltextReader to parse an XML from a file
          descriptor. NOTE that the file descriptor will not be
          closed when the reader is closed or reset. The parsing
          flags @options are a combination of xmlParserOption. This
           reuses the existing @reader xmlTextReader. """
        ret = libxml2mod.xmlReaderNewFd(self._o, fd, URL, encoding, options)
        return ret

    def NewFile(self, filename, encoding, options):
        """parse an XML file from the filesystem or the network. The
          parsing flags @options are a combination of
          xmlParserOption. This reuses the existing @reader
           xmlTextReader. """
        ret = libxml2mod.xmlReaderNewFile(self._o, filename, encoding, options)
        return ret

    def NewMemory(self, buffer, size, URL, encoding, options):
        """Setup an xmltextReader to parse an XML in-memory document.
          The parsing flags @options are a combination of
          xmlParserOption. This reuses the existing @reader
           xmlTextReader. """
        ret = libxml2mod.xmlReaderNewMemory(self._o, buffer, size, URL, encoding, options)
        return ret

    def NewWalker(self, doc):
        """Setup an xmltextReader to parse a preparsed XML document.
           This reuses the existing @reader xmlTextReader. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlReaderNewWalker(self._o, doc__o)
        return ret

    def Next(self):
        """Skip to the node following the current one in document
           order while avoiding the subtree if any. """
        ret = libxml2mod.xmlTextReaderNext(self._o)
        return ret

    def NextSibling(self):
        """Skip to the node following the current one in document
          order while avoiding the subtree if any. Currently
           implemented only for Readers built on a document """
        ret = libxml2mod.xmlTextReaderNextSibling(self._o)
        return ret

    def NodeType(self):
        """Get the node type of the current node Reference:
          http://www.gnu.org/software/dotgnu/pnetlib-doc/System/Xml/Xm
          lNodeType.html """
        ret = libxml2mod.xmlTextReaderNodeType(self._o)
        return ret

    def Normalization(self):
        """The value indicating whether to normalize white space and
          attribute values. Since attribute value and end of line
          normalizations are a MUST in the XML specification only the
          value true is accepted. The broken bahaviour of accepting
          out of range character entities like &#0; is of course not
           supported either. """
        ret = libxml2mod.xmlTextReaderNormalization(self._o)
        return ret

    def Prefix(self):
        """A shorthand reference to the namespace associated with the
           node. """
        ret = libxml2mod.xmlTextReaderConstPrefix(self._o)
        return ret

    def Preserve(self):
        """This tells the XML Reader to preserve the current node. The
          caller must also use xmlTextReaderCurrentDoc() to keep an
           handle on the resulting document once parsing has finished """
        ret = libxml2mod.xmlTextReaderPreserve(self._o)
        if ret is None:raise treeError('xmlTextReaderPreserve() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def QuoteChar(self):
        """The quotation mark character used to enclose the value of
           an attribute. """
        ret = libxml2mod.xmlTextReaderQuoteChar(self._o)
        return ret

    def Read(self):
        """Moves the position of the current instance to the next node
           in the stream, exposing its properties. """
        ret = libxml2mod.xmlTextReaderRead(self._o)
        return ret

    def ReadAttributeValue(self):
        """Parses an attribute value into one or more Text and
           EntityReference nodes. """
        ret = libxml2mod.xmlTextReaderReadAttributeValue(self._o)
        return ret

    def ReadInnerXml(self):
        """Reads the contents of the current node, including child
           nodes and markup. """
        ret = libxml2mod.xmlTextReaderReadInnerXml(self._o)
        return ret

    def ReadOuterXml(self):
        """Reads the contents of the current node, including child
           nodes and markup. """
        ret = libxml2mod.xmlTextReaderReadOuterXml(self._o)
        return ret

    def ReadState(self):
        """Gets the read state of the reader. """
        ret = libxml2mod.xmlTextReaderReadState(self._o)
        return ret

    def ReadString(self):
        """Reads the contents of an element or a text node as a string. """
        ret = libxml2mod.xmlTextReaderReadString(self._o)
        return ret

    def RelaxNGSetSchema(self, schema):
        """Use RelaxNG to validate the document as it is processed.
          Activation is only possible before the first Read(). if
          @schema is None, then RelaxNG validation is desactivated. @
          The @schema should not be freed until the reader is
           deallocated or its use has been deactivated. """
        if schema is None: schema__o = None
        else: schema__o = schema._o
        ret = libxml2mod.xmlTextReaderRelaxNGSetSchema(self._o, schema__o)
        return ret

    def RelaxNGValidate(self, rng):
        """Use RelaxNG schema to validate the document as it is
          processed. Activation is only possible before the first
          Read(). If @rng is None, then RelaxNG schema validation is
           deactivated. """
        ret = libxml2mod.xmlTextReaderRelaxNGValidate(self._o, rng)
        return ret

    def RelaxNGValidateCtxt(self, ctxt, options):
        """Use RelaxNG schema context to validate the document as it
          is processed. Activation is only possible before the first
          Read(). If @ctxt is None, then RelaxNG schema validation is
           deactivated. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlTextReaderRelaxNGValidateCtxt(self._o, ctxt__o, options)
        return ret

    def SchemaValidate(self, xsd):
        """Use W3C XSD schema to validate the document as it is
          processed. Activation is only possible before the first
          Read(). If @xsd is None, then XML Schema validation is
           deactivated. """
        ret = libxml2mod.xmlTextReaderSchemaValidate(self._o, xsd)
        return ret

    def SchemaValidateCtxt(self, ctxt, options):
        """Use W3C XSD schema context to validate the document as it
          is processed. Activation is only possible before the first
          Read(). If @ctxt is None, then XML Schema validation is
           deactivated. """
        if ctxt is None: ctxt__o = None
        else: ctxt__o = ctxt._o
        ret = libxml2mod.xmlTextReaderSchemaValidateCtxt(self._o, ctxt__o, options)
        return ret

    def SetParserProp(self, prop, value):
        """Change the parser processing behaviour by changing some of
          its internal properties. Note that some properties can only
           be changed before any read has been done. """
        ret = libxml2mod.xmlTextReaderSetParserProp(self._o, prop, value)
        return ret

    def SetSchema(self, schema):
        """Use XSD Schema to validate the document as it is processed.
          Activation is only possible before the first Read(). if
          @schema is None, then Schema validation is desactivated. @
          The @schema should not be freed until the reader is
           deallocated or its use has been deactivated. """
        if schema is None: schema__o = None
        else: schema__o = schema._o
        ret = libxml2mod.xmlTextReaderSetSchema(self._o, schema__o)
        return ret

    def Setup(self, input, URL, encoding, options):
        """Setup an XML reader with new options """
        if input is None: input__o = None
        else: input__o = input._o
        ret = libxml2mod.xmlTextReaderSetup(self._o, input__o, URL, encoding, options)
        return ret

    def Standalone(self):
        """Determine the standalone status of the document being read. """
        ret = libxml2mod.xmlTextReaderStandalone(self._o)
        return ret

    def String(self, str):
        """Get an interned string from the reader, allows for example
           to speedup string name comparisons """
        ret = libxml2mod.xmlTextReaderConstString(self._o, str)
        return ret

    def Value(self):
        """Provides the text value of the node if present """
        ret = libxml2mod.xmlTextReaderConstValue(self._o)
        return ret

    def XmlLang(self):
        """The xml:lang scope within which the node resides. """
        ret = libxml2mod.xmlTextReaderConstXmlLang(self._o)
        return ret

    def XmlVersion(self):
        """Determine the XML version of the document being read. """
        ret = libxml2mod.xmlTextReaderConstXmlVersion(self._o)
        return ret

class URI:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeURI(self._o)
        self._o = None

    # accessors for URI
    def authority(self):
        """Get the authority part from an URI """
        ret = libxml2mod.xmlURIGetAuthority(self._o)
        return ret

    def fragment(self):
        """Get the fragment part from an URI """
        ret = libxml2mod.xmlURIGetFragment(self._o)
        return ret

    def opaque(self):
        """Get the opaque part from an URI """
        ret = libxml2mod.xmlURIGetOpaque(self._o)
        return ret

    def path(self):
        """Get the path part from an URI """
        ret = libxml2mod.xmlURIGetPath(self._o)
        return ret

    def port(self):
        """Get the port part from an URI """
        ret = libxml2mod.xmlURIGetPort(self._o)
        return ret

    def query(self):
        """Get the query part from an URI """
        ret = libxml2mod.xmlURIGetQuery(self._o)
        return ret

    def queryRaw(self):
        """Get the raw query part from an URI (i.e. the unescaped
           form). """
        ret = libxml2mod.xmlURIGetQueryRaw(self._o)
        return ret

    def scheme(self):
        """Get the scheme part from an URI """
        ret = libxml2mod.xmlURIGetScheme(self._o)
        return ret

    def server(self):
        """Get the server part from an URI """
        ret = libxml2mod.xmlURIGetServer(self._o)
        return ret

    def setAuthority(self, authority):
        """Set the authority part of an URI. """
        libxml2mod.xmlURISetAuthority(self._o, authority)

    def setFragment(self, fragment):
        """Set the fragment part of an URI. """
        libxml2mod.xmlURISetFragment(self._o, fragment)

    def setOpaque(self, opaque):
        """Set the opaque part of an URI. """
        libxml2mod.xmlURISetOpaque(self._o, opaque)

    def setPath(self, path):
        """Set the path part of an URI. """
        libxml2mod.xmlURISetPath(self._o, path)

    def setPort(self, port):
        """Set the port part of an URI. """
        libxml2mod.xmlURISetPort(self._o, port)

    def setQuery(self, query):
        """Set the query part of an URI. """
        libxml2mod.xmlURISetQuery(self._o, query)

    def setQueryRaw(self, query_raw):
        """Set the raw query part of an URI (i.e. the unescaped form). """
        libxml2mod.xmlURISetQueryRaw(self._o, query_raw)

    def setScheme(self, scheme):
        """Set the scheme part of an URI. """
        libxml2mod.xmlURISetScheme(self._o, scheme)

    def setServer(self, server):
        """Set the server part of an URI. """
        libxml2mod.xmlURISetServer(self._o, server)

    def setUser(self, user):
        """Set the user part of an URI. """
        libxml2mod.xmlURISetUser(self._o, user)

    def user(self):
        """Get the user part from an URI """
        ret = libxml2mod.xmlURIGetUser(self._o)
        return ret

    #
    # URI functions from module uri
    #

    def parseURIReference(self, str):
        """Parse an URI reference string based on RFC 3986 and fills
          in the appropriate fields of the @uri structure
           URI-reference = URI / relative-ref """
        ret = libxml2mod.xmlParseURIReference(self._o, str)
        return ret

    def printURI(self, stream):
        """Prints the URI in the stream @stream. """
        libxml2mod.xmlPrintURI(stream, self._o)

    def saveUri(self):
        """Save the URI as an escaped string """
        ret = libxml2mod.xmlSaveUri(self._o)
        return ret

class ValidCtxt(ValidCtxtCore):
    def __init__(self, _obj=None):
        self._o = _obj
        ValidCtxtCore.__init__(self, _obj=_obj)

    def __del__(self):
        if self._o != None:
            libxml2mod.xmlFreeValidCtxt(self._o)
        self._o = None

    #
    # ValidCtxt functions from module valid
    #

    def validCtxtNormalizeAttributeValue(self, doc, elem, name, value):
        """Does the validation related extra step of the normalization
          of attribute values:  If the declared value is not CDATA,
          then the XML processor must further process the normalized
          attribute value by discarding any leading and trailing
          space (#x20) characters, and by replacing sequences of
          space (#x20) characters by single space (#x20) character.
          Also  check VC: Standalone Document Declaration in P32, and
           update ctxt->valid accordingly """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidCtxtNormalizeAttributeValue(self._o, doc__o, elem__o, name, value)
        return ret

    def validateDocument(self, doc):
        """Try to validate the document instance  basically it does
          the all the checks described by the XML Rec i.e. validates
          the internal and external subset (if present) and validate
           the document tree. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlValidateDocument(self._o, doc__o)
        return ret

    def validateDocumentFinal(self, doc):
        """Does the final step for the document validation once all
          the incremental validation steps have been completed
          basically it does the following checks described by the XML
          Rec  Check all the IDREF/IDREFS attributes definition for
           validity """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlValidateDocumentFinal(self._o, doc__o)
        return ret

    def validateDtd(self, doc, dtd):
        """Try to validate the document against the dtd instance
          Basically it does check all the definitions in the DtD.
          Note the the internal subset (if present) is de-coupled
          (i.e. not used), which could give problems if ID or IDREF
           is present. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if dtd is None: dtd__o = None
        else: dtd__o = dtd._o
        ret = libxml2mod.xmlValidateDtd(self._o, doc__o, dtd__o)
        return ret

    def validateDtdFinal(self, doc):
        """Does the final step for the dtds validation once all the
          subsets have been parsed  basically it does the following
          checks described by the XML Rec - check that ENTITY and
          ENTITIES type attributes default or possible values matches
          one of the defined entities. - check that NOTATION type
          attributes default or possible values matches one of the
           defined notations. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlValidateDtdFinal(self._o, doc__o)
        return ret

    def validateElement(self, doc, elem):
        """Try to validate the subtree under an element """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidateElement(self._o, doc__o, elem__o)
        return ret

    def validateNotationUse(self, doc, notationName):
        """Validate that the given name match a notation declaration.
           - [ VC: Notation Declared ] """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlValidateNotationUse(self._o, doc__o, notationName)
        return ret

    def validateOneAttribute(self, doc, elem, attr, value):
        """Try to validate a single attribute for an element basically
          it does the following checks as described by the XML-1.0
          recommendation: - [ VC: Attribute Value Type ] - [ VC:
          Fixed Attribute Default ] - [ VC: Entity Name ] - [ VC:
          Name Token ] - [ VC: ID ] - [ VC: IDREF ] - [ VC: Entity
          Name ] - [ VC: Notation Attributes ]  The ID/IDREF
           uniqueness and matching are done separately """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        if attr is None: attr__o = None
        else: attr__o = attr._o
        ret = libxml2mod.xmlValidateOneAttribute(self._o, doc__o, elem__o, attr__o, value)
        return ret

    def validateOneElement(self, doc, elem):
        """Try to validate a single element and it's attributes,
          basically it does the following checks as described by the
          XML-1.0 recommendation: - [ VC: Element Valid ] - [ VC:
          Required Attribute ] Then call xmlValidateOneAttribute()
          for each attribute present.  The ID/IDREF checkings are
           done separately """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidateOneElement(self._o, doc__o, elem__o)
        return ret

    def validateOneNamespace(self, doc, elem, prefix, ns, value):
        """Try to validate a single namespace declaration for an
          element basically it does the following checks as described
          by the XML-1.0 recommendation: - [ VC: Attribute Value Type
          ] - [ VC: Fixed Attribute Default ] - [ VC: Entity Name ] -
          [ VC: Name Token ] - [ VC: ID ] - [ VC: IDREF ] - [ VC:
          Entity Name ] - [ VC: Notation Attributes ]  The ID/IDREF
           uniqueness and matching are done separately """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        if ns is None: ns__o = None
        else: ns__o = ns._o
        ret = libxml2mod.xmlValidateOneNamespace(self._o, doc__o, elem__o, prefix, ns__o, value)
        return ret

    def validatePopElement(self, doc, elem, qname):
        """Pop the element end from the validation stack. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidatePopElement(self._o, doc__o, elem__o, qname)
        return ret

    def validatePushCData(self, data, len):
        """check the CData parsed for validation in the current stack """
        ret = libxml2mod.xmlValidatePushCData(self._o, data, len)
        return ret

    def validatePushElement(self, doc, elem, qname):
        """Push a new element start on the validation stack. """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        if elem is None: elem__o = None
        else: elem__o = elem._o
        ret = libxml2mod.xmlValidatePushElement(self._o, doc__o, elem__o, qname)
        return ret

    def validateRoot(self, doc):
        """Try to validate a the root element basically it does the
          following check as described by the XML-1.0 recommendation:
          - [ VC: Root Element Type ] it doesn't try to recurse or
           apply other check to the element """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        ret = libxml2mod.xmlValidateRoot(self._o, doc__o)
        return ret

class xpathContext:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    # accessors for xpathContext
    def contextDoc(self):
        """Get the doc from an xpathContext """
        ret = libxml2mod.xmlXPathGetContextDoc(self._o)
        if ret is None:raise xpathError('xmlXPathGetContextDoc() failed')
        __tmp = xmlDoc(_obj=ret)
        return __tmp

    def contextNode(self):
        """Get the current node from an xpathContext """
        ret = libxml2mod.xmlXPathGetContextNode(self._o)
        if ret is None:raise xpathError('xmlXPathGetContextNode() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def contextPosition(self):
        """Get the current node from an xpathContext """
        ret = libxml2mod.xmlXPathGetContextPosition(self._o)
        return ret

    def contextSize(self):
        """Get the current node from an xpathContext """
        ret = libxml2mod.xmlXPathGetContextSize(self._o)
        return ret

    def function(self):
        """Get the current function name xpathContext """
        ret = libxml2mod.xmlXPathGetFunction(self._o)
        return ret

    def functionURI(self):
        """Get the current function name URI xpathContext """
        ret = libxml2mod.xmlXPathGetFunctionURI(self._o)
        return ret

    def setContextDoc(self, doc):
        """Set the doc of an xpathContext """
        if doc is None: doc__o = None
        else: doc__o = doc._o
        libxml2mod.xmlXPathSetContextDoc(self._o, doc__o)

    def setContextNode(self, node):
        """Set the current node of an xpathContext """
        if node is None: node__o = None
        else: node__o = node._o
        libxml2mod.xmlXPathSetContextNode(self._o, node__o)

    #
    # xpathContext functions from module python
    #

    def registerXPathFunction(self, name, ns_uri, f):
        """Register a Python written function to the XPath interpreter """
        ret = libxml2mod.xmlRegisterXPathFunction(self._o, name, ns_uri, f)
        return ret

    def xpathRegisterVariable(self, name, ns_uri, value):
        """Register a variable with the XPath context """
        ret = libxml2mod.xmlXPathRegisterVariable(self._o, name, ns_uri, value)
        return ret

    #
    # xpathContext functions from module xpath
    #

    def xpathContextSetCache(self, active, value, options):
        """Creates/frees an object cache on the XPath context. If
          activates XPath objects (xmlXPathObject) will be cached
          internally to be reused. @options: 0: This will set the
          XPath object caching: @value: This will set the maximum
          number of XPath objects to be cached per slot There are 5
          slots for: node-set, string, number, boolean, and misc
          objects. Use <0 for the default number (100). Other values
           for @options have currently no effect. """
        ret = libxml2mod.xmlXPathContextSetCache(self._o, active, value, options)
        return ret

    def xpathEval(self, str):
        """Evaluate the XPath Location Path in the given context. """
        ret = libxml2mod.xmlXPathEval(str, self._o)
        if ret is None:raise xpathError('xmlXPathEval() failed')
        return xpathObjectRet(ret)

    def xpathEvalExpression(self, str):
        """Evaluate the XPath expression in the given context. """
        ret = libxml2mod.xmlXPathEvalExpression(str, self._o)
        if ret is None:raise xpathError('xmlXPathEvalExpression() failed')
        return xpathObjectRet(ret)

    def xpathFreeContext(self):
        """Free up an xmlXPathContext """
        libxml2mod.xmlXPathFreeContext(self._o)

    #
    # xpathContext functions from module xpathInternals
    #

    def xpathNewParserContext(self, str):
        """Create a new xmlXPathParserContext """
        ret = libxml2mod.xmlXPathNewParserContext(str, self._o)
        if ret is None:raise xpathError('xmlXPathNewParserContext() failed')
        __tmp = xpathParserContext(_obj=ret)
        return __tmp

    def xpathNsLookup(self, prefix):
        """Search in the namespace declaration array of the context
           for the given namespace name associated to the given prefix """
        ret = libxml2mod.xmlXPathNsLookup(self._o, prefix)
        return ret

    def xpathRegisterAllFunctions(self):
        """Registers all default XPath functions in this context """
        libxml2mod.xmlXPathRegisterAllFunctions(self._o)

    def xpathRegisterNs(self, prefix, ns_uri):
        """Register a new namespace. If @ns_uri is None it unregisters
           the namespace """
        ret = libxml2mod.xmlXPathRegisterNs(self._o, prefix, ns_uri)
        return ret

    def xpathRegisteredFuncsCleanup(self):
        """Cleanup the XPath context data associated to registered
           functions """
        libxml2mod.xmlXPathRegisteredFuncsCleanup(self._o)

    def xpathRegisteredNsCleanup(self):
        """Cleanup the XPath context data associated to registered
           variables """
        libxml2mod.xmlXPathRegisteredNsCleanup(self._o)

    def xpathRegisteredVariablesCleanup(self):
        """Cleanup the XPath context data associated to registered
           variables """
        libxml2mod.xmlXPathRegisteredVariablesCleanup(self._o)

    def xpathVariableLookup(self, name):
        """Search in the Variable array of the context for the given
           variable value. """
        ret = libxml2mod.xmlXPathVariableLookup(self._o, name)
        if ret is None:raise xpathError('xmlXPathVariableLookup() failed')
        return xpathObjectRet(ret)

    def xpathVariableLookupNS(self, name, ns_uri):
        """Search in the Variable array of the context for the given
           variable value. """
        ret = libxml2mod.xmlXPathVariableLookupNS(self._o, name, ns_uri)
        if ret is None:raise xpathError('xmlXPathVariableLookupNS() failed')
        return xpathObjectRet(ret)

    #
    # xpathContext functions from module xpointer
    #

    def xpointerEval(self, str):
        """Evaluate the XPath Location Path in the given context. """
        ret = libxml2mod.xmlXPtrEval(str, self._o)
        if ret is None:raise treeError('xmlXPtrEval() failed')
        return xpathObjectRet(ret)

class xpathParserContext:
    def __init__(self, _obj=None):
        if _obj != None:self._o = _obj;return
        self._o = None

    # accessors for xpathParserContext
    def context(self):
        """Get the xpathContext from an xpathParserContext """
        ret = libxml2mod.xmlXPathParserGetContext(self._o)
        if ret is None:raise xpathError('xmlXPathParserGetContext() failed')
        __tmp = xpathContext(_obj=ret)
        return __tmp

    #
    # xpathParserContext functions from module xpathInternals
    #

    def xpathAddValues(self):
        """Implement the add operation on XPath objects: The numeric
          operators convert their operands to numbers as if by
           calling the number function. """
        libxml2mod.xmlXPathAddValues(self._o)

    def xpathBooleanFunction(self, nargs):
        """Implement the boolean() XPath function boolean
          boolean(object) The boolean function converts its argument
          to a boolean as follows: - a number is true if and only if
          it is neither positive or negative zero nor NaN - a
          node-set is true if and only if it is non-empty - a string
           is true if and only if its length is non-zero """
        libxml2mod.xmlXPathBooleanFunction(self._o, nargs)

    def xpathCeilingFunction(self, nargs):
        """Implement the ceiling() XPath function number
          ceiling(number) The ceiling function returns the smallest
          (closest to negative infinity) number that is not less than
           the argument and that is an integer. """
        libxml2mod.xmlXPathCeilingFunction(self._o, nargs)

    def xpathCompareValues(self, inf, strict):
        """Implement the compare operation on XPath objects: @arg1 <
          @arg2    (1, 1, ... @arg1 <= @arg2   (1, 0, ... @arg1 >
          @arg2    (0, 1, ... @arg1 >= @arg2   (0, 0, ...  When
          neither object to be compared is a node-set and the
          operator is <=, <, >=, >, then the objects are compared by
          converted both objects to numbers and comparing the numbers
          according to IEEE 754. The < comparison will be true if and
          only if the first number is less than the second number.
          The <= comparison will be true if and only if the first
          number is less than or equal to the second number. The >
          comparison will be true if and only if the first number is
          greater than the second number. The >= comparison will be
          true if and only if the first number is greater than or
           equal to the second number. """
        ret = libxml2mod.xmlXPathCompareValues(self._o, inf, strict)
        return ret

    def xpathConcatFunction(self, nargs):
        """Implement the concat() XPath function string concat(string,
          string, string*) The concat function returns the
           concatenation of its arguments. """
        libxml2mod.xmlXPathConcatFunction(self._o, nargs)

    def xpathContainsFunction(self, nargs):
        """Implement the contains() XPath function boolean
          contains(string, string) The contains function returns true
          if the first argument string contains the second argument
           string, and otherwise returns false. """
        libxml2mod.xmlXPathContainsFunction(self._o, nargs)

    def xpathCountFunction(self, nargs):
        """Implement the count() XPath function number count(node-set) """
        libxml2mod.xmlXPathCountFunction(self._o, nargs)

    def xpathDivValues(self):
        """Implement the div operation on XPath objects @arg1 / @arg2:
          The numeric operators convert their operands to numbers as
           if by calling the number function. """
        libxml2mod.xmlXPathDivValues(self._o)

    def xpathEqualValues(self):
        """Implement the equal operation on XPath objects content:
           @arg1 == @arg2 """
        ret = libxml2mod.xmlXPathEqualValues(self._o)
        return ret

    def xpathErr(self, error):
        """Handle an XPath error """
        libxml2mod.xmlXPathErr(self._o, error)

    def xpathEvalExpr(self):
        """Parse and evaluate an XPath expression in the given
           context, then push the result on the context stack """
        libxml2mod.xmlXPathEvalExpr(self._o)

    def xpathFalseFunction(self, nargs):
        """Implement the false() XPath function boolean false() """
        libxml2mod.xmlXPathFalseFunction(self._o, nargs)

    def xpathFloorFunction(self, nargs):
        """Implement the floor() XPath function number floor(number)
          The floor function returns the largest (closest to positive
          infinity) number that is not greater than the argument and
           that is an integer. """
        libxml2mod.xmlXPathFloorFunction(self._o, nargs)

    def xpathFreeParserContext(self):
        """Free up an xmlXPathParserContext """
        libxml2mod.xmlXPathFreeParserContext(self._o)

    def xpathIdFunction(self, nargs):
        """Implement the id() XPath function node-set id(object) The
          id function selects elements by their unique ID (see [5.2.1
          Unique IDs]). When the argument to id is of type node-set,
          then the result is the union of the result of applying id
          to the string value of each of the nodes in the argument
          node-set. When the argument to id is of any other type, the
          argument is converted to a string as if by a call to the
          string function; the string is split into a
          whitespace-separated list of tokens (whitespace is any
          sequence of characters matching the production S); the
          result is a node-set containing the elements in the same
          document as the context node that have a unique ID equal to
           any of the tokens in the list. """
        libxml2mod.xmlXPathIdFunction(self._o, nargs)

    def xpathLangFunction(self, nargs):
        """Implement the lang() XPath function boolean lang(string)
          The lang function returns true or false depending on
          whether the language of the context node as specified by
          xml:lang attributes is the same as or is a sublanguage of
          the language specified by the argument string. The language
          of the context node is determined by the value of the
          xml:lang attribute on the context node, or, if the context
          node has no xml:lang attribute, by the value of the
          xml:lang attribute on the nearest ancestor of the context
          node that has an xml:lang attribute. If there is no such
           attribute, then lang """
        libxml2mod.xmlXPathLangFunction(self._o, nargs)

    def xpathLastFunction(self, nargs):
        """Implement the last() XPath function number last() The last
          function returns the number of nodes in the context node
           list. """
        libxml2mod.xmlXPathLastFunction(self._o, nargs)

    def xpathLocalNameFunction(self, nargs):
        """Implement the local-name() XPath function string
          local-name(node-set?) The local-name function returns a
          string containing the local part of the name of the node in
          the argument node-set that is first in document order. If
          the node-set is empty or the first node has no name, an
          empty string is returned. If the argument is omitted it
           defaults to the context node. """
        libxml2mod.xmlXPathLocalNameFunction(self._o, nargs)

    def xpathModValues(self):
        """Implement the mod operation on XPath objects: @arg1 / @arg2
          The numeric operators convert their operands to numbers as
           if by calling the number function. """
        libxml2mod.xmlXPathModValues(self._o)

    def xpathMultValues(self):
        """Implement the multiply operation on XPath objects: The
          numeric operators convert their operands to numbers as if
           by calling the number function. """
        libxml2mod.xmlXPathMultValues(self._o)

    def xpathNamespaceURIFunction(self, nargs):
        """Implement the namespace-uri() XPath function string
          namespace-uri(node-set?) The namespace-uri function returns
          a string containing the namespace URI of the expanded name
          of the node in the argument node-set that is first in
          document order. If the node-set is empty, the first node
          has no name, or the expanded name has no namespace URI, an
          empty string is returned. If the argument is omitted it
           defaults to the context node. """
        libxml2mod.xmlXPathNamespaceURIFunction(self._o, nargs)

    def xpathNextAncestor(self, cur):
        """Traversal function for the "ancestor" direction the
          ancestor axis contains the ancestors of the context node;
          the ancestors of the context node consist of the parent of
          context node and the parent's parent and so on; the nodes
          are ordered in reverse document order; thus the parent is
          the first node on the axis, and the parent's parent is the
           second node on the axis """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextAncestor(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextAncestor() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextAncestorOrSelf(self, cur):
        """Traversal function for the "ancestor-or-self" direction he
          ancestor-or-self axis contains the context node and
          ancestors of the context node in reverse document order;
          thus the context node is the first node on the axis, and
          the context node's parent the second; parent here is
           defined the same as with the parent axis. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextAncestorOrSelf(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextAncestorOrSelf() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextAttribute(self, cur):
        """Traversal function for the "attribute" direction TODO:
           support DTD inherited default attributes """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextAttribute(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextAttribute() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextChild(self, cur):
        """Traversal function for the "child" direction The child axis
          contains the children of the context node in document order. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextChild(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextChild() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextDescendant(self, cur):
        """Traversal function for the "descendant" direction the
          descendant axis contains the descendants of the context
          node in document order; a descendant is a child or a child
           of a child and so on. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextDescendant(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextDescendant() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextDescendantOrSelf(self, cur):
        """Traversal function for the "descendant-or-self" direction
          the descendant-or-self axis contains the context node and
          the descendants of the context node in document order; thus
          the context node is the first node on the axis, and the
          first child of the context node is the second node on the
           axis """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextDescendantOrSelf(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextDescendantOrSelf() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextFollowing(self, cur):
        """Traversal function for the "following" direction The
          following axis contains all nodes in the same document as
          the context node that are after the context node in
          document order, excluding any descendants and excluding
          attribute nodes and namespace nodes; the nodes are ordered
           in document order """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextFollowing(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextFollowing() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextFollowingSibling(self, cur):
        """Traversal function for the "following-sibling" direction
          The following-sibling axis contains the following siblings
           of the context node in document order. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextFollowingSibling(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextFollowingSibling() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextNamespace(self, cur):
        """Traversal function for the "namespace" direction the
          namespace axis contains the namespace nodes of the context
          node; the order of nodes on this axis is
          implementation-defined; the axis will be empty unless the
          context node is an element  We keep the XML namespace node
           at the end of the list. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextNamespace(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextNamespace() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextParent(self, cur):
        """Traversal function for the "parent" direction The parent
          axis contains the parent of the context node, if there is
           one. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextParent(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextParent() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextPreceding(self, cur):
        """Traversal function for the "preceding" direction the
          preceding axis contains all nodes in the same document as
          the context node that are before the context node in
          document order, excluding any ancestors and excluding
          attribute nodes and namespace nodes; the nodes are ordered
           in reverse document order """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextPreceding(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextPreceding() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextPrecedingSibling(self, cur):
        """Traversal function for the "preceding-sibling" direction
          The preceding-sibling axis contains the preceding siblings
          of the context node in reverse document order; the first
          preceding sibling is first on the axis; the sibling
           preceding that node is the second on the axis and so on. """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextPrecedingSibling(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextPrecedingSibling() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNextSelf(self, cur):
        """Traversal function for the "self" direction The self axis
           contains just the context node itself """
        if cur is None: cur__o = None
        else: cur__o = cur._o
        ret = libxml2mod.xmlXPathNextSelf(self._o, cur__o)
        if ret is None:raise xpathError('xmlXPathNextSelf() failed')
        __tmp = xmlNode(_obj=ret)
        return __tmp

    def xpathNormalizeFunction(self, nargs):
        """Implement the normalize-space() XPath function string
          normalize-space(string?) The normalize-space function
          returns the argument string with white space normalized by
          stripping leading and trailing whitespace and replacing
          sequences of whitespace characters by a single space.
          Whitespace characters are the same allowed by the S
          production in XML. If the argument is omitted, it defaults
          to the context node converted to a string, in other words
           the value of the context node. """
        libxml2mod.xmlXPathNormalizeFunction(self._o, nargs)

    def xpathNotEqualValues(self):
        """Implement the equal operation on XPath objects content:
           @arg1 == @arg2 """
        ret = libxml2mod.xmlXPathNotEqualValues(self._o)
        return ret

    def xpathNotFunction(self, nargs):
        """Implement the not() XPath function boolean not(boolean) The
          not function returns true if its argument is false, and
           false otherwise. """
        libxml2mod.xmlXPathNotFunction(self._o, nargs)

    def xpathNumberFunction(self, nargs):
        """Implement the number() XPath function number number(object?) """
        libxml2mod.xmlXPathNumberFunction(self._o, nargs)

    def xpathParseNCName(self):
        """parse an XML namespace non qualified name.  [NS 3] NCName
          ::= (Letter | '_') (NCNameChar)*  [NS 4] NCNameChar ::=
           Letter | Digit | '.' | '-' | '_' | CombiningChar | Extender """
        ret = libxml2mod.xmlXPathParseNCName(self._o)
        return ret

    def xpathParseName(self):
        """parse an XML name  [4] NameChar ::= Letter | Digit | '.' |
          '-' | '_' | ':' | CombiningChar | Extender  [5] Name ::=
           (Letter | '_' | ':') (NameChar)* """
        ret = libxml2mod.xmlXPathParseName(self._o)
        return ret

    def xpathPopBoolean(self):
        """Pops a boolean from the stack, handling conversion if
           needed. Check error with #xmlXPathCheckError. """
        ret = libxml2mod.xmlXPathPopBoolean(self._o)
        return ret

    def xpathPopNumber(self):
        """Pops a number from the stack, handling conversion if
           needed. Check error with #xmlXPathCheckError. """
        ret = libxml2mod.xmlXPathPopNumber(self._o)
        return ret

    def xpathPopString(self):
        """Pops a string from the stack, handling conversion if
           needed. Check error with #xmlXPathCheckError. """
        ret = libxml2mod.xmlXPathPopString(self._o)
        return ret

    def xpathPositionFunction(self, nargs):
        """Implement the position() XPath function number position()
          The position function returns the position of the context
          node in the context node list. The first position is 1, and
           so the last position will be equal to last(). """
        libxml2mod.xmlXPathPositionFunction(self._o, nargs)

    def xpathRoot(self):
        """Initialize the context to the root of the document """
        libxml2mod.xmlXPathRoot(self._o)

    def xpathRoundFunction(self, nargs):
        """Implement the round() XPath function number round(number)
          The round function returns the number that is closest to
          the argument and that is an integer. If there are two such
           numbers, then the one that is even is returned. """
        libxml2mod.xmlXPathRoundFunction(self._o, nargs)

    def xpathStartsWithFunction(self, nargs):
        """Implement the starts-with() XPath function boolean
          starts-with(string, string) The starts-with function
          returns true if the first argument string starts with the
           second argument string, and otherwise returns false. """
        libxml2mod.xmlXPathStartsWithFunction(self._o, nargs)

    def xpathStringFunction(self, nargs):
        """Implement the string() XPath function string
          string(object?) The string function converts an object to a
          string as follows: - A node-set is converted to a string by
          returning the value of the node in the node-set that is
          first in document order. If the node-set is empty, an empty
          string is returned. - A number is converted to a string as
          follows + NaN is converted to the string NaN + positive
          zero is converted to the string 0 + negative zero is
          converted to the string 0 + positive infinity is converted
          to the string Infinity + negative infinity is converted to
          the string -Infinity + if the number is an integer, the
          number is represented in decimal form as a Number with no
          decimal point and no leading zeros, preceded by a minus
          sign (-) if the number is negative + otherwise, the number
          is represented in decimal form as a Number including a
          decimal point with at least one digit before the decimal
          point and at least one digit after the decimal point,
          preceded by a minus sign (-) if the number is negative;
          there must be no leading zeros before the decimal point
          apart possibly from the one required digit immediately
          before the decimal point; beyond the one required digit
          after the decimal point there must be as many, but only as
          many, more digits as are needed to uniquely distinguish the
          number from all other IEEE 754 numeric values. - The
          boolean false value is converted to the string false. The
          boolean true value is converted to the string true.  If the
          argument is omitted, it defaults to a node-set with the
           context node as its only member. """
        libxml2mod.xmlXPathStringFunction(self._o, nargs)

    def xpathStringLengthFunction(self, nargs):
        """Implement the string-length() XPath function number
          string-length(string?) The string-length returns the number
          of characters in the string (see [3.6 Strings]). If the
          argument is omitted, it defaults to the context node
          converted to a string, in other words the value of the
           context node. """
        libxml2mod.xmlXPathStringLengthFunction(self._o, nargs)

    def xpathSubValues(self):
        """Implement the subtraction operation on XPath objects: The
          numeric operators convert their operands to numbers as if
           by calling the number function. """
        libxml2mod.xmlXPathSubValues(self._o)

    def xpathSubstringAfterFunction(self, nargs):
        """Implement the substring-after() XPath function string
          substring-after(string, string) The substring-after
          function returns the substring of the first argument string
          that follows the first occurrence of the second argument
          string in the first argument string, or the empty stringi
          if the first argument string does not contain the second
          argument string. For example,
          substring-after("1999/04/01","/") returns 04/01, and
           substring-after("1999/04/01","19") returns 99/04/01. """
        libxml2mod.xmlXPathSubstringAfterFunction(self._o, nargs)

    def xpathSubstringBeforeFunction(self, nargs):
        """Implement the substring-before() XPath function string
          substring-before(string, string) The substring-before
          function returns the substring of the first argument string
          that precedes the first occurrence of the second argument
          string in the first argument string, or the empty string if
          the first argument string does not contain the second
          argument string. For example,
           substring-before("1999/04/01","/") returns 1999. """
        libxml2mod.xmlXPathSubstringBeforeFunction(self._o, nargs)

    def xpathSubstringFunction(self, nargs):
        """Implement the substring() XPath function string
          substring(string, number, number?) The substring function
          returns the substring of the first argument starting at the
          position specified in the second argument with length
          specified in the third argument. For example,
          substring("12345",2,3) returns "234". If the third argument
          is not specified, it returns the substring starting at the
          position specified in the second argument and continuing to
          the end of the string. For example, substring("12345",2)
          returns "2345".  More precisely, each character in the
          string (see [3.6 Strings]) is considered to have a numeric
          position: the position of the first character is 1, the
          position of the second character is 2 and so on. The
          returned substring contains those characters for which the
          position of the character is greater than or equal to the
          second argument and, if the third argument is specified,
          less than the sum of the second and third arguments; the
          comparisons and addition used for the above follow the
          standard IEEE 754 rules. Thus: - substring("12345", 1.5,
          2.6) returns "234" - substring("12345", 0, 3) returns "12"
          - substring("12345", 0 div 0, 3) returns "" -
          substring("12345", 1, 0 div 0) returns "" -
          substring("12345", -42, 1 div 0) returns "12345" -
           substring("12345", -1 div 0, 1 div 0) returns "" """
        libxml2mod.xmlXPathSubstringFunction(self._o, nargs)

    def xpathSumFunction(self, nargs):
        """Implement the sum() XPath function number sum(node-set) The
          sum function returns the sum of the values of the nodes in
           the argument node-set. """
        libxml2mod.xmlXPathSumFunction(self._o, nargs)

    def xpathTranslateFunction(self, nargs):
        """Implement the translate() XPath function string
          translate(string, string, string) The translate function
          returns the first argument string with occurrences of
          characters in the second argument string replaced by the
          character at the corresponding position in the third
          argument string. For example, translate("bar","abc","ABC")
          returns the string BAr. If there is a character in the
          second argument string with no character at a corresponding
          position in the third argument string (because the second
          argument string is longer than the third argument string),
          then occurrences of that character in the first argument
          string are removed. For example,
           translate("--aaa--","abc-","ABC") """
        libxml2mod.xmlXPathTranslateFunction(self._o, nargs)

    def xpathTrueFunction(self, nargs):
        """Implement the true() XPath function boolean true() """
        libxml2mod.xmlXPathTrueFunction(self._o, nargs)

    def xpathValueFlipSign(self):
        """Implement the unary - operation on an XPath object The
          numeric operators convert their operands to numbers as if
           by calling the number function. """
        libxml2mod.xmlXPathValueFlipSign(self._o)

    def xpatherror(self, file, line, no):
        """Formats an error message. """
        libxml2mod.xmlXPatherror(self._o, file, line, no)

    #
    # xpathParserContext functions from module xpointer
    #

    def xpointerEvalRangePredicate(self):
        """[8]   Predicate ::=   '[' PredicateExpr ']' [9]
          PredicateExpr ::=   Expr  Evaluate a predicate as in
          xmlXPathEvalPredicate() but for a Location Set instead of a
           node set """
        libxml2mod.xmlXPtrEvalRangePredicate(self._o)

    def xpointerRangeToFunction(self, nargs):
        """Implement the range-to() XPointer function """
        libxml2mod.xmlXPtrRangeToFunction(self._o, nargs)

# xlinkShow
XLINK_SHOW_NONE = 0
XLINK_SHOW_NEW = 1
XLINK_SHOW_EMBED = 2
XLINK_SHOW_REPLACE = 3

# xmlRelaxNGParserFlag
XML_RELAXNGP_NONE = 0
XML_RELAXNGP_FREE_DOC = 1
XML_RELAXNGP_CRNG = 2

# xmlBufferAllocationScheme
XML_BUFFER_ALLOC_DOUBLEIT = 1
XML_BUFFER_ALLOC_EXACT = 2
XML_BUFFER_ALLOC_IMMUTABLE = 3
XML_BUFFER_ALLOC_IO = 4
XML_BUFFER_ALLOC_HYBRID = 5

# xmlParserSeverities
XML_PARSER_SEVERITY_VALIDITY_WARNING = 1
XML_PARSER_SEVERITY_VALIDITY_ERROR = 2
XML_PARSER_SEVERITY_WARNING = 3
XML_PARSER_SEVERITY_ERROR = 4

# xmlAttributeDefault
XML_ATTRIBUTE_NONE = 1
XML_ATTRIBUTE_REQUIRED = 2
XML_ATTRIBUTE_IMPLIED = 3
XML_ATTRIBUTE_FIXED = 4

# xmlSchemaValType
XML_SCHEMAS_UNKNOWN = 0
XML_SCHEMAS_STRING = 1
XML_SCHEMAS_NORMSTRING = 2
XML_SCHEMAS_DECIMAL = 3
XML_SCHEMAS_TIME = 4
XML_SCHEMAS_GDAY = 5
XML_SCHEMAS_GMONTH = 6
XML_SCHEMAS_GMONTHDAY = 7
XML_SCHEMAS_GYEAR = 8
XML_SCHEMAS_GYEARMONTH = 9
XML_SCHEMAS_DATE = 10
XML_SCHEMAS_DATETIME = 11
XML_SCHEMAS_DURATION = 12
XML_SCHEMAS_FLOAT = 13
XML_SCHEMAS_DOUBLE = 14
XML_SCHEMAS_BOOLEAN = 15
XML_SCHEMAS_TOKEN = 16
XML_SCHEMAS_LANGUAGE = 17
XML_SCHEMAS_NMTOKEN = 18
XML_SCHEMAS_NMTOKENS = 19
XML_SCHEMAS_NAME = 20
XML_SCHEMAS_QNAME = 21
XML_SCHEMAS_NCNAME = 22
XML_SCHEMAS_ID = 23
XML_SCHEMAS_IDREF = 24
XML_SCHEMAS_IDREFS = 25
XML_SCHEMAS_ENTITY = 26
XML_SCHEMAS_ENTITIES = 27
XML_SCHEMAS_NOTATION = 28
XML_SCHEMAS_ANYURI = 29
XML_SCHEMAS_INTEGER = 30
XML_SCHEMAS_NPINTEGER = 31
XML_SCHEMAS_NINTEGER = 32
XML_SCHEMAS_NNINTEGER = 33
XML_SCHEMAS_PINTEGER = 34
XML_SCHEMAS_INT = 35
XML_SCHEMAS_UINT = 36
XML_SCHEMAS_LONG = 37
XML_SCHEMAS_ULONG = 38
XML_SCHEMAS_SHORT = 39
XML_SCHEMAS_USHORT = 40
XML_SCHEMAS_BYTE = 41
XML_SCHEMAS_UBYTE = 42
XML_SCHEMAS_HEXBINARY = 43
XML_SCHEMAS_BASE64BINARY = 44
XML_SCHEMAS_ANYTYPE = 45
XML_SCHEMAS_ANYSIMPLETYPE = 46

# xmlParserInputState
XML_PARSER_EOF = -1
XML_PARSER_START = 0
XML_PARSER_MISC = 1
XML_PARSER_PI = 2
XML_PARSER_DTD = 3
XML_PARSER_PROLOG = 4
XML_PARSER_COMMENT = 5
XML_PARSER_START_TAG = 6
XML_PARSER_CONTENT = 7
XML_PARSER_CDATA_SECTION = 8
XML_PARSER_END_TAG = 9
XML_PARSER_ENTITY_DECL = 10
XML_PARSER_ENTITY_VALUE = 11
XML_PARSER_ATTRIBUTE_VALUE = 12
XML_PARSER_SYSTEM_LITERAL = 13
XML_PARSER_EPILOG = 14
XML_PARSER_IGNORE = 15
XML_PARSER_PUBLIC_LITERAL = 16

# xmlEntityType
XML_INTERNAL_GENERAL_ENTITY = 1
XML_EXTERNAL_GENERAL_PARSED_ENTITY = 2
XML_EXTERNAL_GENERAL_UNPARSED_ENTITY = 3
XML_INTERNAL_PARAMETER_ENTITY = 4
XML_EXTERNAL_PARAMETER_ENTITY = 5
XML_INTERNAL_PREDEFINED_ENTITY = 6

# xmlSaveOption
XML_SAVE_FORMAT = 1
XML_SAVE_NO_DECL = 2
XML_SAVE_NO_EMPTY = 4
XML_SAVE_NO_XHTML = 8
XML_SAVE_XHTML = 16
XML_SAVE_AS_XML = 32
XML_SAVE_AS_HTML = 64
XML_SAVE_WSNONSIG = 128

# xmlPatternFlags
XML_PATTERN_DEFAULT = 0
XML_PATTERN_XPATH = 1
XML_PATTERN_XSSEL = 2
XML_PATTERN_XSFIELD = 4

# xmlParserErrors
XML_ERR_OK = 0
XML_ERR_INTERNAL_ERROR = 1
XML_ERR_NO_MEMORY = 2
XML_ERR_DOCUMENT_START = 3
XML_ERR_DOCUMENT_EMPTY = 4
XML_ERR_DOCUMENT_END = 5
XML_ERR_INVALID_HEX_CHARREF = 6
XML_ERR_INVALID_DEC_CHARREF = 7
XML_ERR_INVALID_CHARREF = 8
XML_ERR_INVALID_CHAR = 9
XML_ERR_CHARREF_AT_EOF = 10
XML_ERR_CHARREF_IN_PROLOG = 11
XML_ERR_CHARREF_IN_EPILOG = 12
XML_ERR_CHARREF_IN_DTD = 13
XML_ERR_ENTITYREF_AT_EOF = 14
XML_ERR_ENTITYREF_IN_PROLOG = 15
XML_ERR_ENTITYREF_IN_EPILOG = 16
XML_ERR_ENTITYREF_IN_DTD = 17
XML_ERR_PEREF_AT_EOF = 18
XML_ERR_PEREF_IN_PROLOG = 19
XML_ERR_PEREF_IN_EPILOG = 20
XML_ERR_PEREF_IN_INT_SUBSET = 21
XML_ERR_ENTITYREF_NO_NAME = 22
XML_ERR_ENTITYREF_SEMICOL_MISSING = 23
XML_ERR_PEREF_NO_NAME = 24
XML_ERR_PEREF_SEMICOL_MISSING = 25
XML_ERR_UNDECLARED_ENTITY = 26
XML_WAR_UNDECLARED_ENTITY = 27
XML_ERR_UNPARSED_ENTITY = 28
XML_ERR_ENTITY_IS_EXTERNAL = 29
XML_ERR_ENTITY_IS_PARAMETER = 30
XML_ERR_UNKNOWN_ENCODING = 31
XML_ERR_UNSUPPORTED_ENCODING = 32
XML_ERR_STRING_NOT_STARTED = 33
XML_ERR_STRING_NOT_CLOSED = 34
XML_ERR_NS_DECL_ERROR = 35
XML_ERR_ENTITY_NOT_STARTED = 36
XML_ERR_ENTITY_NOT_FINISHED = 37
XML_ERR_LT_IN_ATTRIBUTE = 38
XML_ERR_ATTRIBUTE_NOT_STARTED = 39
XML_ERR_ATTRIBUTE_NOT_FINISHED = 40
XML_ERR_ATTRIBUTE_WITHOUT_VALUE = 41
XML_ERR_ATTRIBUTE_REDEFINED = 42
XML_ERR_LITERAL_NOT_STARTED = 43
XML_ERR_LITERAL_NOT_FINISHED = 44
XML_ERR_COMMENT_NOT_FINISHED = 45
XML_ERR_PI_NOT_STARTED = 46
XML_ERR_PI_NOT_FINISHED = 47
XML_ERR_NOTATION_NOT_STARTED = 48
XML_ERR_NOTATION_NOT_FINISHED = 49
XML_ERR_ATTLIST_NOT_STARTED = 50
XML_ERR_ATTLIST_NOT_FINISHED = 51
XML_ERR_MIXED_NOT_STARTED = 52
XML_ERR_MIXED_NOT_FINISHED = 53
XML_ERR_ELEMCONTENT_NOT_STARTED = 54
XML_ERR_ELEMCONTENT_NOT_FINISHED = 55
XML_ERR_XMLDECL_NOT_STARTED = 56
XML_ERR_XMLDECL_NOT_FINISHED = 57
XML_ERR_CONDSEC_NOT_STARTED = 58
XML_ERR_CONDSEC_NOT_FINISHED = 59
XML_ERR_EXT_SUBSET_NOT_FINISHED = 60
XML_ERR_DOCTYPE_NOT_FINISHED = 61
XML_ERR_MISPLACED_CDATA_END = 62
XML_ERR_CDATA_NOT_FINISHED = 63
XML_ERR_RESERVED_XML_NAME = 64
XML_ERR_SPACE_REQUIRED = 65
XML_ERR_SEPARATOR_REQUIRED = 66
XML_ERR_NMTOKEN_REQUIRED = 67
XML_ERR_NAME_REQUIRED = 68
XML_ERR_PCDATA_REQUIRED = 69
XML_ERR_URI_REQUIRED = 70
XML_ERR_PUBID_REQUIRED = 71
XML_ERR_LT_REQUIRED = 72
XML_ERR_GT_REQUIRED = 73
XML_ERR_LTSLASH_REQUIRED = 74
XML_ERR_EQUAL_REQUIRED = 75
XML_ERR_TAG_NAME_MISMATCH = 76
XML_ERR_TAG_NOT_FINISHED = 77
XML_ERR_STANDALONE_VALUE = 78
XML_ERR_ENCODING_NAME = 79
XML_ERR_HYPHEN_IN_COMMENT = 80
XML_ERR_INVALID_ENCODING = 81
XML_ERR_EXT_ENTITY_STANDALONE = 82
XML_ERR_CONDSEC_INVALID = 83
XML_ERR_VALUE_REQUIRED = 84
XML_ERR_NOT_WELL_BALANCED = 85
XML_ERR_EXTRA_CONTENT = 86
XML_ERR_ENTITY_CHAR_ERROR = 87
XML_ERR_ENTITY_PE_INTERNAL = 88
XML_ERR_ENTITY_LOOP = 89
XML_ERR_ENTITY_BOUNDARY = 90
XML_ERR_INVALID_URI = 91
XML_ERR_URI_FRAGMENT = 92
XML_WAR_CATALOG_PI = 93
XML_ERR_NO_DTD = 94
XML_ERR_CONDSEC_INVALID_KEYWORD = 95
XML_ERR_VERSION_MISSING = 96
XML_WAR_UNKNOWN_VERSION = 97
XML_WAR_LANG_VALUE = 98
XML_WAR_NS_URI = 99
XML_WAR_NS_URI_RELATIVE = 100
XML_ERR_MISSING_ENCODING = 101
XML_WAR_SPACE_VALUE = 102
XML_ERR_NOT_STANDALONE = 103
XML_ERR_ENTITY_PROCESSING = 104
XML_ERR_NOTATION_PROCESSING = 105
XML_WAR_NS_COLUMN = 106
XML_WAR_ENTITY_REDEFINED = 107
XML_ERR_UNKNOWN_VERSION = 108
XML_ERR_VERSION_MISMATCH = 109
XML_ERR_NAME_TOO_LONG = 110
XML_ERR_USER_STOP = 111
XML_NS_ERR_XML_NAMESPACE = 200
XML_NS_ERR_UNDEFINED_NAMESPACE = 201
XML_NS_ERR_QNAME = 202
XML_NS_ERR_ATTRIBUTE_REDEFINED = 203
XML_NS_ERR_EMPTY = 204
XML_NS_ERR_COLON = 205
XML_DTD_ATTRIBUTE_DEFAULT = 500
XML_DTD_ATTRIBUTE_REDEFINED = 501
XML_DTD_ATTRIBUTE_VALUE = 502
XML_DTD_CONTENT_ERROR = 503
XML_DTD_CONTENT_MODEL = 504
XML_DTD_CONTENT_NOT_DETERMINIST = 505
XML_DTD_DIFFERENT_PREFIX = 506
XML_DTD_ELEM_DEFAULT_NAMESPACE = 507
XML_DTD_ELEM_NAMESPACE = 508
XML_DTD_ELEM_REDEFINED = 509
XML_DTD_EMPTY_NOTATION = 510
XML_DTD_ENTITY_TYPE = 511
XML_DTD_ID_FIXED = 512
XML_DTD_ID_REDEFINED = 513
XML_DTD_ID_SUBSET = 514
XML_DTD_INVALID_CHILD = 515
XML_DTD_INVALID_DEFAULT = 516
XML_DTD_LOAD_ERROR = 517
XML_DTD_MISSING_ATTRIBUTE = 518
XML_DTD_MIXED_CORRUPT = 519
XML_DTD_MULTIPLE_ID = 520
XML_DTD_NO_DOC = 521
XML_DTD_NO_DTD = 522
XML_DTD_NO_ELEM_NAME = 523
XML_DTD_NO_PREFIX = 524
XML_DTD_NO_ROOT = 525
XML_DTD_NOTATION_REDEFINED = 526
XML_DTD_NOTATION_VALUE = 527
XML_DTD_NOT_EMPTY = 528
XML_DTD_NOT_PCDATA = 529
XML_DTD_NOT_STANDALONE = 530
XML_DTD_ROOT_NAME = 531
XML_DTD_STANDALONE_WHITE_SPACE = 532
XML_DTD_UNKNOWN_ATTRIBUTE = 533
XML_DTD_UNKNOWN_ELEM = 534
XML_DTD_UNKNOWN_ENTITY = 535
XML_DTD_UNKNOWN_ID = 536
XML_DTD_UNKNOWN_NOTATION = 537
XML_DTD_STANDALONE_DEFAULTED = 538
XML_DTD_XMLID_VALUE = 539
XML_DTD_XMLID_TYPE = 540
XML_DTD_DUP_TOKEN = 541
XML_HTML_STRUCURE_ERROR = 800
XML_HTML_UNKNOWN_TAG = 801
XML_RNGP_ANYNAME_ATTR_ANCESTOR = 1000
XML_RNGP_ATTR_CONFLICT = 1001
XML_RNGP_ATTRIBUTE_CHILDREN = 1002
XML_RNGP_ATTRIBUTE_CONTENT = 1003
XML_RNGP_ATTRIBUTE_EMPTY = 1004
XML_RNGP_ATTRIBUTE_NOOP = 1005
XML_RNGP_CHOICE_CONTENT = 1006
XML_RNGP_CHOICE_EMPTY = 1007
XML_RNGP_CREATE_FAILURE = 1008
XML_RNGP_DATA_CONTENT = 1009
XML_RNGP_DEF_CHOICE_AND_INTERLEAVE = 1010
XML_RNGP_DEFINE_CREATE_FAILED = 1011
XML_RNGP_DEFINE_EMPTY = 1012
XML_RNGP_DEFINE_MISSING = 1013
XML_RNGP_DEFINE_NAME_MISSING = 1014
XML_RNGP_ELEM_CONTENT_EMPTY = 1015
XML_RNGP_ELEM_CONTENT_ERROR = 1016
XML_RNGP_ELEMENT_EMPTY = 1017
XML_RNGP_ELEMENT_CONTENT = 1018
XML_RNGP_ELEMENT_NAME = 1019
XML_RNGP_ELEMENT_NO_CONTENT = 1020
XML_RNGP_ELEM_TEXT_CONFLICT = 1021
XML_RNGP_EMPTY = 1022
XML_RNGP_EMPTY_CONSTRUCT = 1023
XML_RNGP_EMPTY_CONTENT = 1024
XML_RNGP_EMPTY_NOT_EMPTY = 1025
XML_RNGP_ERROR_TYPE_LIB = 1026
XML_RNGP_EXCEPT_EMPTY = 1027
XML_RNGP_EXCEPT_MISSING = 1028
XML_RNGP_EXCEPT_MULTIPLE = 1029
XML_RNGP_EXCEPT_NO_CONTENT = 1030
XML_RNGP_EXTERNALREF_EMTPY = 1031
XML_RNGP_EXTERNAL_REF_FAILURE = 1032
XML_RNGP_EXTERNALREF_RECURSE = 1033
XML_RNGP_FORBIDDEN_ATTRIBUTE = 1034
XML_RNGP_FOREIGN_ELEMENT = 1035
XML_RNGP_GRAMMAR_CONTENT = 1036
XML_RNGP_GRAMMAR_EMPTY = 1037
XML_RNGP_GRAMMAR_MISSING = 1038
XML_RNGP_GRAMMAR_NO_START = 1039
XML_RNGP_GROUP_ATTR_CONFLICT = 1040
XML_RNGP_HREF_ERROR = 1041
XML_RNGP_INCLUDE_EMPTY = 1042
XML_RNGP_INCLUDE_FAILURE = 1043
XML_RNGP_INCLUDE_RECURSE = 1044
XML_RNGP_INTERLEAVE_ADD = 1045
XML_RNGP_INTERLEAVE_CREATE_FAILED = 1046
XML_RNGP_INTERLEAVE_EMPTY = 1047
XML_RNGP_INTERLEAVE_NO_CONTENT = 1048
XML_RNGP_INVALID_DEFINE_NAME = 1049
XML_RNGP_INVALID_URI = 1050
XML_RNGP_INVALID_VALUE = 1051
XML_RNGP_MISSING_HREF = 1052
XML_RNGP_NAME_MISSING = 1053
XML_RNGP_NEED_COMBINE = 1054
XML_RNGP_NOTALLOWED_NOT_EMPTY = 1055
XML_RNGP_NSNAME_ATTR_ANCESTOR = 1056
XML_RNGP_NSNAME_NO_NS = 1057
XML_RNGP_PARAM_FORBIDDEN = 1058
XML_RNGP_PARAM_NAME_MISSING = 1059
XML_RNGP_PARENTREF_CREATE_FAILED = 1060
XML_RNGP_PARENTREF_NAME_INVALID = 1061
XML_RNGP_PARENTREF_NO_NAME = 1062
XML_RNGP_PARENTREF_NO_PARENT = 1063
XML_RNGP_PARENTREF_NOT_EMPTY = 1064
XML_RNGP_PARSE_ERROR = 1065
XML_RNGP_PAT_ANYNAME_EXCEPT_ANYNAME = 1066
XML_RNGP_PAT_ATTR_ATTR = 1067
XML_RNGP_PAT_ATTR_ELEM = 1068
XML_RNGP_PAT_DATA_EXCEPT_ATTR = 1069
XML_RNGP_PAT_DATA_EXCEPT_ELEM = 1070
XML_RNGP_PAT_DATA_EXCEPT_EMPTY = 1071
XML_RNGP_PAT_DATA_EXCEPT_GROUP = 1072
XML_RNGP_PAT_DATA_EXCEPT_INTERLEAVE = 1073
XML_RNGP_PAT_DATA_EXCEPT_LIST = 1074
XML_RNGP_PAT_DATA_EXCEPT_ONEMORE = 1075
XML_RNGP_PAT_DATA_EXCEPT_REF = 1076
XML_RNGP_PAT_DATA_EXCEPT_TEXT = 1077
XML_RNGP_PAT_LIST_ATTR = 1078
XML_RNGP_PAT_LIST_ELEM = 1079
XML_RNGP_PAT_LIST_INTERLEAVE = 1080
XML_RNGP_PAT_LIST_LIST = 1081
XML_RNGP_PAT_LIST_REF = 1082
XML_RNGP_PAT_LIST_TEXT = 1083
XML_RNGP_PAT_NSNAME_EXCEPT_ANYNAME = 1084
XML_RNGP_PAT_NSNAME_EXCEPT_NSNAME = 1085
XML_RNGP_PAT_ONEMORE_GROUP_ATTR = 1086
XML_RNGP_PAT_ONEMORE_INTERLEAVE_ATTR = 1087
XML_RNGP_PAT_START_ATTR = 1088
XML_RNGP_PAT_START_DATA = 1089
XML_RNGP_PAT_START_EMPTY = 1090
XML_RNGP_PAT_START_GROUP = 1091
XML_RNGP_PAT_START_INTERLEAVE = 1092
XML_RNGP_PAT_START_LIST = 1093
XML_RNGP_PAT_START_ONEMORE = 1094
XML_RNGP_PAT_START_TEXT = 1095
XML_RNGP_PAT_START_VALUE = 1096
XML_RNGP_PREFIX_UNDEFINED = 1097
XML_RNGP_REF_CREATE_FAILED = 1098
XML_RNGP_REF_CYCLE = 1099
XML_RNGP_REF_NAME_INVALID = 1100
XML_RNGP_REF_NO_DEF = 1101
XML_RNGP_REF_NO_NAME = 1102
XML_RNGP_REF_NOT_EMPTY = 1103
XML_RNGP_START_CHOICE_AND_INTERLEAVE = 1104
XML_RNGP_START_CONTENT = 1105
XML_RNGP_START_EMPTY = 1106
XML_RNGP_START_MISSING = 1107
XML_RNGP_TEXT_EXPECTED = 1108
XML_RNGP_TEXT_HAS_CHILD = 1109
XML_RNGP_TYPE_MISSING = 1110
XML_RNGP_TYPE_NOT_FOUND = 1111
XML_RNGP_TYPE_VALUE = 1112
XML_RNGP_UNKNOWN_ATTRIBUTE = 1113
XML_RNGP_UNKNOWN_COMBINE = 1114
XML_RNGP_UNKNOWN_CONSTRUCT = 1115
XML_RNGP_UNKNOWN_TYPE_LIB = 1116
XML_RNGP_URI_FRAGMENT = 1117
XML_RNGP_URI_NOT_ABSOLUTE = 1118
XML_RNGP_VALUE_EMPTY = 1119
XML_RNGP_VALUE_NO_CONTENT = 1120
XML_RNGP_XMLNS_NAME = 1121
XML_RNGP_XML_NS = 1122
XML_XPATH_EXPRESSION_OK = 1200
XML_XPATH_NUMBER_ERROR = 1201
XML_XPATH_UNFINISHED_LITERAL_ERROR = 1202
XML_XPATH_START_LITERAL_ERROR = 1203
XML_XPATH_VARIABLE_REF_ERROR = 1204
XML_XPATH_UNDEF_VARIABLE_ERROR = 1205
XML_XPATH_INVALID_PREDICATE_ERROR = 1206
XML_XPATH_EXPR_ERROR = 1207
XML_XPATH_UNCLOSED_ERROR = 1208
XML_XPATH_UNKNOWN_FUNC_ERROR = 1209
XML_XPATH_INVALID_OPERAND = 1210
XML_XPATH_INVALID_TYPE = 1211
XML_XPATH_INVALID_ARITY = 1212
XML_XPATH_INVALID_CTXT_SIZE = 1213
XML_XPATH_INVALID_CTXT_POSITION = 1214
XML_XPATH_MEMORY_ERROR = 1215
XML_XPTR_SYNTAX_ERROR = 1216
XML_XPTR_RESOURCE_ERROR = 1217
XML_XPTR_SUB_RESOURCE_ERROR = 1218
XML_XPATH_UNDEF_PREFIX_ERROR = 1219
XML_XPATH_ENCODING_ERROR = 1220
XML_XPATH_INVALID_CHAR_ERROR = 1221
XML_TREE_INVALID_HEX = 1300
XML_TREE_INVALID_DEC = 1301
XML_TREE_UNTERMINATED_ENTITY = 1302
XML_TREE_NOT_UTF8 = 1303
XML_SAVE_NOT_UTF8 = 1400
XML_SAVE_CHAR_INVALID = 1401
XML_SAVE_NO_DOCTYPE = 1402
XML_SAVE_UNKNOWN_ENCODING = 1403
XML_REGEXP_COMPILE_ERROR = 1450
XML_IO_UNKNOWN = 1500
XML_IO_EACCES = 1501
XML_IO_EAGAIN = 1502
XML_IO_EBADF = 1503
XML_IO_EBADMSG = 1504
XML_IO_EBUSY = 1505
XML_IO_ECANCELED = 1506
XML_IO_ECHILD = 1507
XML_IO_EDEADLK = 1508
XML_IO_EDOM = 1509
XML_IO_EEXIST = 1510
XML_IO_EFAULT = 1511
XML_IO_EFBIG = 1512
XML_IO_EINPROGRESS = 1513
XML_IO_EINTR = 1514
XML_IO_EINVAL = 1515
XML_IO_EIO = 1516
XML_IO_EISDIR = 1517
XML_IO_EMFILE = 1518
XML_IO_EMLINK = 1519
XML_IO_EMSGSIZE = 1520
XML_IO_ENAMETOOLONG = 1521
XML_IO_ENFILE = 1522
XML_IO_ENODEV = 1523
XML_IO_ENOENT = 1524
XML_IO_ENOEXEC = 1525
XML_IO_ENOLCK = 1526
XML_IO_ENOMEM = 1527
XML_IO_ENOSPC = 1528
XML_IO_ENOSYS = 1529
XML_IO_ENOTDIR = 1530
XML_IO_ENOTEMPTY = 1531
XML_IO_ENOTSUP = 1532
XML_IO_ENOTTY = 1533
XML_IO_ENXIO = 1534
XML_IO_EPERM = 1535
XML_IO_EPIPE = 1536
XML_IO_ERANGE = 1537
XML_IO_EROFS = 1538
XML_IO_ESPIPE = 1539
XML_IO_ESRCH = 1540
XML_IO_ETIMEDOUT = 1541
XML_IO_EXDEV = 1542
XML_IO_NETWORK_ATTEMPT = 1543
XML_IO_ENCODER = 1544
XML_IO_FLUSH = 1545
XML_IO_WRITE = 1546
XML_IO_NO_INPUT = 1547
XML_IO_BUFFER_FULL = 1548
XML_IO_LOAD_ERROR = 1549
XML_IO_ENOTSOCK = 1550
XML_IO_EISCONN = 1551
XML_IO_ECONNREFUSED = 1552
XML_IO_ENETUNREACH = 1553
XML_IO_EADDRINUSE = 1554
XML_IO_EALREADY = 1555
XML_IO_EAFNOSUPPORT = 1556
XML_XINCLUDE_RECURSION = 1600
XML_XINCLUDE_PARSE_VALUE = 1601
XML_XINCLUDE_ENTITY_DEF_MISMATCH = 1602
XML_XINCLUDE_NO_HREF = 1603
XML_XINCLUDE_NO_FALLBACK = 1604
XML_XINCLUDE_HREF_URI = 1605
XML_XINCLUDE_TEXT_FRAGMENT = 1606
XML_XINCLUDE_TEXT_DOCUMENT = 1607
XML_XINCLUDE_INVALID_CHAR = 1608
XML_XINCLUDE_BUILD_FAILED = 1609
XML_XINCLUDE_UNKNOWN_ENCODING = 1610
XML_XINCLUDE_MULTIPLE_ROOT = 1611
XML_XINCLUDE_XPTR_FAILED = 1612
XML_XINCLUDE_XPTR_RESULT = 1613
XML_XINCLUDE_INCLUDE_IN_INCLUDE = 1614
XML_XINCLUDE_FALLBACKS_IN_INCLUDE = 1615
XML_XINCLUDE_FALLBACK_NOT_IN_INCLUDE = 1616
XML_XINCLUDE_DEPRECATED_NS = 1617
XML_XINCLUDE_FRAGMENT_ID = 1618
XML_CATALOG_MISSING_ATTR = 1650
XML_CATALOG_ENTRY_BROKEN = 1651
XML_CATALOG_PREFER_VALUE = 1652
XML_CATALOG_NOT_CATALOG = 1653
XML_CATALOG_RECURSION = 1654
XML_SCHEMAP_PREFIX_UNDEFINED = 1700
XML_SCHEMAP_ATTRFORMDEFAULT_VALUE = 1701
XML_SCHEMAP_ATTRGRP_NONAME_NOREF = 1702
XML_SCHEMAP_ATTR_NONAME_NOREF = 1703
XML_SCHEMAP_COMPLEXTYPE_NONAME_NOREF = 1704
XML_SCHEMAP_ELEMFORMDEFAULT_VALUE = 1705
XML_SCHEMAP_ELEM_NONAME_NOREF = 1706
XML_SCHEMAP_EXTENSION_NO_BASE = 1707
XML_SCHEMAP_FACET_NO_VALUE = 1708
XML_SCHEMAP_FAILED_BUILD_IMPORT = 1709
XML_SCHEMAP_GROUP_NONAME_NOREF = 1710
XML_SCHEMAP_IMPORT_NAMESPACE_NOT_URI = 1711
XML_SCHEMAP_IMPORT_REDEFINE_NSNAME = 1712
XML_SCHEMAP_IMPORT_SCHEMA_NOT_URI = 1713
XML_SCHEMAP_INVALID_BOOLEAN = 1714
XML_SCHEMAP_INVALID_ENUM = 1715
XML_SCHEMAP_INVALID_FACET = 1716
XML_SCHEMAP_INVALID_FACET_VALUE = 1717
XML_SCHEMAP_INVALID_MAXOCCURS = 1718
XML_SCHEMAP_INVALID_MINOCCURS = 1719
XML_SCHEMAP_INVALID_REF_AND_SUBTYPE = 1720
XML_SCHEMAP_INVALID_WHITE_SPACE = 1721
XML_SCHEMAP_NOATTR_NOREF = 1722
XML_SCHEMAP_NOTATION_NO_NAME = 1723
XML_SCHEMAP_NOTYPE_NOREF = 1724
XML_SCHEMAP_REF_AND_SUBTYPE = 1725
XML_SCHEMAP_RESTRICTION_NONAME_NOREF = 1726
XML_SCHEMAP_SIMPLETYPE_NONAME = 1727
XML_SCHEMAP_TYPE_AND_SUBTYPE = 1728
XML_SCHEMAP_UNKNOWN_ALL_CHILD = 1729
XML_SCHEMAP_UNKNOWN_ANYATTRIBUTE_CHILD = 1730
XML_SCHEMAP_UNKNOWN_ATTR_CHILD = 1731
XML_SCHEMAP_UNKNOWN_ATTRGRP_CHILD = 1732
XML_SCHEMAP_UNKNOWN_ATTRIBUTE_GROUP = 1733
XML_SCHEMAP_UNKNOWN_BASE_TYPE = 1734
XML_SCHEMAP_UNKNOWN_CHOICE_CHILD = 1735
XML_SCHEMAP_UNKNOWN_COMPLEXCONTENT_CHILD = 1736
XML_SCHEMAP_UNKNOWN_COMPLEXTYPE_CHILD = 1737
XML_SCHEMAP_UNKNOWN_ELEM_CHILD = 1738
XML_SCHEMAP_UNKNOWN_EXTENSION_CHILD = 1739
XML_SCHEMAP_UNKNOWN_FACET_CHILD = 1740
XML_SCHEMAP_UNKNOWN_FACET_TYPE = 1741
XML_SCHEMAP_UNKNOWN_GROUP_CHILD = 1742
XML_SCHEMAP_UNKNOWN_IMPORT_CHILD = 1743
XML_SCHEMAP_UNKNOWN_LIST_CHILD = 1744
XML_SCHEMAP_UNKNOWN_NOTATION_CHILD = 1745
XML_SCHEMAP_UNKNOWN_PROCESSCONTENT_CHILD = 1746
XML_SCHEMAP_UNKNOWN_REF = 1747
XML_SCHEMAP_UNKNOWN_RESTRICTION_CHILD = 1748
XML_SCHEMAP_UNKNOWN_SCHEMAS_CHILD = 1749
XML_SCHEMAP_UNKNOWN_SEQUENCE_CHILD = 1750
XML_SCHEMAP_UNKNOWN_SIMPLECONTENT_CHILD = 1751
XML_SCHEMAP_UNKNOWN_SIMPLETYPE_CHILD = 1752
XML_SCHEMAP_UNKNOWN_TYPE = 1753
XML_SCHEMAP_UNKNOWN_UNION_CHILD = 1754
XML_SCHEMAP_ELEM_DEFAULT_FIXED = 1755
XML_SCHEMAP_REGEXP_INVALID = 1756
XML_SCHEMAP_FAILED_LOAD = 1757
XML_SCHEMAP_NOTHING_TO_PARSE = 1758
XML_SCHEMAP_NOROOT = 1759
XML_SCHEMAP_REDEFINED_GROUP = 1760
XML_SCHEMAP_REDEFINED_TYPE = 1761
XML_SCHEMAP_REDEFINED_ELEMENT = 1762
XML_SCHEMAP_REDEFINED_ATTRGROUP = 1763
XML_SCHEMAP_REDEFINED_ATTR = 1764
XML_SCHEMAP_REDEFINED_NOTATION = 1765
XML_SCHEMAP_FAILED_PARSE = 1766
XML_SCHEMAP_UNKNOWN_PREFIX = 1767
XML_SCHEMAP_DEF_AND_PREFIX = 1768
XML_SCHEMAP_UNKNOWN_INCLUDE_CHILD = 1769
XML_SCHEMAP_INCLUDE_SCHEMA_NOT_URI = 1770
XML_SCHEMAP_INCLUDE_SCHEMA_NO_URI = 1771
XML_SCHEMAP_NOT_SCHEMA = 1772
XML_SCHEMAP_UNKNOWN_MEMBER_TYPE = 1773
XML_SCHEMAP_INVALID_ATTR_USE = 1774
XML_SCHEMAP_RECURSIVE = 1775
XML_SCHEMAP_SUPERNUMEROUS_LIST_ITEM_TYPE = 1776
XML_SCHEMAP_INVALID_ATTR_COMBINATION = 1777
XML_SCHEMAP_INVALID_ATTR_INLINE_COMBINATION = 1778
XML_SCHEMAP_MISSING_SIMPLETYPE_CHILD = 1779
XML_SCHEMAP_INVALID_ATTR_NAME = 1780
XML_SCHEMAP_REF_AND_CONTENT = 1781
XML_SCHEMAP_CT_PROPS_CORRECT_1 = 1782
XML_SCHEMAP_CT_PROPS_CORRECT_2 = 1783
XML_SCHEMAP_CT_PROPS_CORRECT_3 = 1784
XML_SCHEMAP_CT_PROPS_CORRECT_4 = 1785
XML_SCHEMAP_CT_PROPS_CORRECT_5 = 1786
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_1 = 1787
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_2_1_1 = 1788
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_2_1_2 = 1789
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_2_2 = 1790
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_3 = 1791
XML_SCHEMAP_WILDCARD_INVALID_NS_MEMBER = 1792
XML_SCHEMAP_INTERSECTION_NOT_EXPRESSIBLE = 1793
XML_SCHEMAP_UNION_NOT_EXPRESSIBLE = 1794
XML_SCHEMAP_SRC_IMPORT_3_1 = 1795
XML_SCHEMAP_SRC_IMPORT_3_2 = 1796
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_4_1 = 1797
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_4_2 = 1798
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_4_3 = 1799
XML_SCHEMAP_COS_CT_EXTENDS_1_3 = 1800
XML_SCHEMAV_NOROOT = 1801
XML_SCHEMAV_UNDECLAREDELEM = 1802
XML_SCHEMAV_NOTTOPLEVEL = 1803
XML_SCHEMAV_MISSING = 1804
XML_SCHEMAV_WRONGELEM = 1805
XML_SCHEMAV_NOTYPE = 1806
XML_SCHEMAV_NOROLLBACK = 1807
XML_SCHEMAV_ISABSTRACT = 1808
XML_SCHEMAV_NOTEMPTY = 1809
XML_SCHEMAV_ELEMCONT = 1810
XML_SCHEMAV_HAVEDEFAULT = 1811
XML_SCHEMAV_NOTNILLABLE = 1812
XML_SCHEMAV_EXTRACONTENT = 1813
XML_SCHEMAV_INVALIDATTR = 1814
XML_SCHEMAV_INVALIDELEM = 1815
XML_SCHEMAV_NOTDETERMINIST = 1816
XML_SCHEMAV_CONSTRUCT = 1817
XML_SCHEMAV_INTERNAL = 1818
XML_SCHEMAV_NOTSIMPLE = 1819
XML_SCHEMAV_ATTRUNKNOWN = 1820
XML_SCHEMAV_ATTRINVALID = 1821
XML_SCHEMAV_VALUE = 1822
XML_SCHEMAV_FACET = 1823
XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_1 = 1824
XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_2 = 1825
XML_SCHEMAV_CVC_DATATYPE_VALID_1_2_3 = 1826
XML_SCHEMAV_CVC_TYPE_3_1_1 = 1827
XML_SCHEMAV_CVC_TYPE_3_1_2 = 1828
XML_SCHEMAV_CVC_FACET_VALID = 1829
XML_SCHEMAV_CVC_LENGTH_VALID = 1830
XML_SCHEMAV_CVC_MINLENGTH_VALID = 1831
XML_SCHEMAV_CVC_MAXLENGTH_VALID = 1832
XML_SCHEMAV_CVC_MININCLUSIVE_VALID = 1833
XML_SCHEMAV_CVC_MAXINCLUSIVE_VALID = 1834
XML_SCHEMAV_CVC_MINEXCLUSIVE_VALID = 1835
XML_SCHEMAV_CVC_MAXEXCLUSIVE_VALID = 1836
XML_SCHEMAV_CVC_TOTALDIGITS_VALID = 1837
XML_SCHEMAV_CVC_FRACTIONDIGITS_VALID = 1838
XML_SCHEMAV_CVC_PATTERN_VALID = 1839
XML_SCHEMAV_CVC_ENUMERATION_VALID = 1840
XML_SCHEMAV_CVC_COMPLEX_TYPE_2_1 = 1841
XML_SCHEMAV_CVC_COMPLEX_TYPE_2_2 = 1842
XML_SCHEMAV_CVC_COMPLEX_TYPE_2_3 = 1843
XML_SCHEMAV_CVC_COMPLEX_TYPE_2_4 = 1844
XML_SCHEMAV_CVC_ELT_1 = 1845
XML_SCHEMAV_CVC_ELT_2 = 1846
XML_SCHEMAV_CVC_ELT_3_1 = 1847
XML_SCHEMAV_CVC_ELT_3_2_1 = 1848
XML_SCHEMAV_CVC_ELT_3_2_2 = 1849
XML_SCHEMAV_CVC_ELT_4_1 = 1850
XML_SCHEMAV_CVC_ELT_4_2 = 1851
XML_SCHEMAV_CVC_ELT_4_3 = 1852
XML_SCHEMAV_CVC_ELT_5_1_1 = 1853
XML_SCHEMAV_CVC_ELT_5_1_2 = 1854
XML_SCHEMAV_CVC_ELT_5_2_1 = 1855
XML_SCHEMAV_CVC_ELT_5_2_2_1 = 1856
XML_SCHEMAV_CVC_ELT_5_2_2_2_1 = 1857
XML_SCHEMAV_CVC_ELT_5_2_2_2_2 = 1858
XML_SCHEMAV_CVC_ELT_6 = 1859
XML_SCHEMAV_CVC_ELT_7 = 1860
XML_SCHEMAV_CVC_ATTRIBUTE_1 = 1861
XML_SCHEMAV_CVC_ATTRIBUTE_2 = 1862
XML_SCHEMAV_CVC_ATTRIBUTE_3 = 1863
XML_SCHEMAV_CVC_ATTRIBUTE_4 = 1864
XML_SCHEMAV_CVC_COMPLEX_TYPE_3_1 = 1865
XML_SCHEMAV_CVC_COMPLEX_TYPE_3_2_1 = 1866
XML_SCHEMAV_CVC_COMPLEX_TYPE_3_2_2 = 1867
XML_SCHEMAV_CVC_COMPLEX_TYPE_4 = 1868
XML_SCHEMAV_CVC_COMPLEX_TYPE_5_1 = 1869
XML_SCHEMAV_CVC_COMPLEX_TYPE_5_2 = 1870
XML_SCHEMAV_ELEMENT_CONTENT = 1871
XML_SCHEMAV_DOCUMENT_ELEMENT_MISSING = 1872
XML_SCHEMAV_CVC_COMPLEX_TYPE_1 = 1873
XML_SCHEMAV_CVC_AU = 1874
XML_SCHEMAV_CVC_TYPE_1 = 1875
XML_SCHEMAV_CVC_TYPE_2 = 1876
XML_SCHEMAV_CVC_IDC = 1877
XML_SCHEMAV_CVC_WILDCARD = 1878
XML_SCHEMAV_MISC = 1879
XML_XPTR_UNKNOWN_SCHEME = 1900
XML_XPTR_CHILDSEQ_START = 1901
XML_XPTR_EVAL_FAILED = 1902
XML_XPTR_EXTRA_OBJECTS = 1903
XML_C14N_CREATE_CTXT = 1950
XML_C14N_REQUIRES_UTF8 = 1951
XML_C14N_CREATE_STACK = 1952
XML_C14N_INVALID_NODE = 1953
XML_C14N_UNKNOW_NODE = 1954
XML_C14N_RELATIVE_NAMESPACE = 1955
XML_FTP_PASV_ANSWER = 2000
XML_FTP_EPSV_ANSWER = 2001
XML_FTP_ACCNT = 2002
XML_FTP_URL_SYNTAX = 2003
XML_HTTP_URL_SYNTAX = 2020
XML_HTTP_USE_IP = 2021
XML_HTTP_UNKNOWN_HOST = 2022
XML_SCHEMAP_SRC_SIMPLE_TYPE_1 = 3000
XML_SCHEMAP_SRC_SIMPLE_TYPE_2 = 3001
XML_SCHEMAP_SRC_SIMPLE_TYPE_3 = 3002
XML_SCHEMAP_SRC_SIMPLE_TYPE_4 = 3003
XML_SCHEMAP_SRC_RESOLVE = 3004
XML_SCHEMAP_SRC_RESTRICTION_BASE_OR_SIMPLETYPE = 3005
XML_SCHEMAP_SRC_LIST_ITEMTYPE_OR_SIMPLETYPE = 3006
XML_SCHEMAP_SRC_UNION_MEMBERTYPES_OR_SIMPLETYPES = 3007
XML_SCHEMAP_ST_PROPS_CORRECT_1 = 3008
XML_SCHEMAP_ST_PROPS_CORRECT_2 = 3009
XML_SCHEMAP_ST_PROPS_CORRECT_3 = 3010
XML_SCHEMAP_COS_ST_RESTRICTS_1_1 = 3011
XML_SCHEMAP_COS_ST_RESTRICTS_1_2 = 3012
XML_SCHEMAP_COS_ST_RESTRICTS_1_3_1 = 3013
XML_SCHEMAP_COS_ST_RESTRICTS_1_3_2 = 3014
XML_SCHEMAP_COS_ST_RESTRICTS_2_1 = 3015
XML_SCHEMAP_COS_ST_RESTRICTS_2_3_1_1 = 3016
XML_SCHEMAP_COS_ST_RESTRICTS_2_3_1_2 = 3017
XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_1 = 3018
XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_2 = 3019
XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_3 = 3020
XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_4 = 3021
XML_SCHEMAP_COS_ST_RESTRICTS_2_3_2_5 = 3022
XML_SCHEMAP_COS_ST_RESTRICTS_3_1 = 3023
XML_SCHEMAP_COS_ST_RESTRICTS_3_3_1 = 3024
XML_SCHEMAP_COS_ST_RESTRICTS_3_3_1_2 = 3025
XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_2 = 3026
XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_1 = 3027
XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_3 = 3028
XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_4 = 3029
XML_SCHEMAP_COS_ST_RESTRICTS_3_3_2_5 = 3030
XML_SCHEMAP_COS_ST_DERIVED_OK_2_1 = 3031
XML_SCHEMAP_COS_ST_DERIVED_OK_2_2 = 3032
XML_SCHEMAP_S4S_ELEM_NOT_ALLOWED = 3033
XML_SCHEMAP_S4S_ELEM_MISSING = 3034
XML_SCHEMAP_S4S_ATTR_NOT_ALLOWED = 3035
XML_SCHEMAP_S4S_ATTR_MISSING = 3036
XML_SCHEMAP_S4S_ATTR_INVALID_VALUE = 3037
XML_SCHEMAP_SRC_ELEMENT_1 = 3038
XML_SCHEMAP_SRC_ELEMENT_2_1 = 3039
XML_SCHEMAP_SRC_ELEMENT_2_2 = 3040
XML_SCHEMAP_SRC_ELEMENT_3 = 3041
XML_SCHEMAP_P_PROPS_CORRECT_1 = 3042
XML_SCHEMAP_P_PROPS_CORRECT_2_1 = 3043
XML_SCHEMAP_P_PROPS_CORRECT_2_2 = 3044
XML_SCHEMAP_E_PROPS_CORRECT_2 = 3045
XML_SCHEMAP_E_PROPS_CORRECT_3 = 3046
XML_SCHEMAP_E_PROPS_CORRECT_4 = 3047
XML_SCHEMAP_E_PROPS_CORRECT_5 = 3048
XML_SCHEMAP_E_PROPS_CORRECT_6 = 3049
XML_SCHEMAP_SRC_INCLUDE = 3050
XML_SCHEMAP_SRC_ATTRIBUTE_1 = 3051
XML_SCHEMAP_SRC_ATTRIBUTE_2 = 3052
XML_SCHEMAP_SRC_ATTRIBUTE_3_1 = 3053
XML_SCHEMAP_SRC_ATTRIBUTE_3_2 = 3054
XML_SCHEMAP_SRC_ATTRIBUTE_4 = 3055
XML_SCHEMAP_NO_XMLNS = 3056
XML_SCHEMAP_NO_XSI = 3057
XML_SCHEMAP_COS_VALID_DEFAULT_1 = 3058
XML_SCHEMAP_COS_VALID_DEFAULT_2_1 = 3059
XML_SCHEMAP_COS_VALID_DEFAULT_2_2_1 = 3060
XML_SCHEMAP_COS_VALID_DEFAULT_2_2_2 = 3061
XML_SCHEMAP_CVC_SIMPLE_TYPE = 3062
XML_SCHEMAP_COS_CT_EXTENDS_1_1 = 3063
XML_SCHEMAP_SRC_IMPORT_1_1 = 3064
XML_SCHEMAP_SRC_IMPORT_1_2 = 3065
XML_SCHEMAP_SRC_IMPORT_2 = 3066
XML_SCHEMAP_SRC_IMPORT_2_1 = 3067
XML_SCHEMAP_SRC_IMPORT_2_2 = 3068
XML_SCHEMAP_INTERNAL = 3069
XML_SCHEMAP_NOT_DETERMINISTIC = 3070
XML_SCHEMAP_SRC_ATTRIBUTE_GROUP_1 = 3071
XML_SCHEMAP_SRC_ATTRIBUTE_GROUP_2 = 3072
XML_SCHEMAP_SRC_ATTRIBUTE_GROUP_3 = 3073
XML_SCHEMAP_MG_PROPS_CORRECT_1 = 3074
XML_SCHEMAP_MG_PROPS_CORRECT_2 = 3075
XML_SCHEMAP_SRC_CT_1 = 3076
XML_SCHEMAP_DERIVATION_OK_RESTRICTION_2_1_3 = 3077
XML_SCHEMAP_AU_PROPS_CORRECT_2 = 3078
XML_SCHEMAP_A_PROPS_CORRECT_2 = 3079
XML_SCHEMAP_C_PROPS_CORRECT = 3080
XML_SCHEMAP_SRC_REDEFINE = 3081
XML_SCHEMAP_SRC_IMPORT = 3082
XML_SCHEMAP_WARN_SKIP_SCHEMA = 3083
XML_SCHEMAP_WARN_UNLOCATED_SCHEMA = 3084
XML_SCHEMAP_WARN_ATTR_REDECL_PROH = 3085
XML_SCHEMAP_WARN_ATTR_POINTLESS_PROH = 3086
XML_SCHEMAP_AG_PROPS_CORRECT = 3087
XML_SCHEMAP_COS_CT_EXTENDS_1_2 = 3088
XML_SCHEMAP_AU_PROPS_CORRECT = 3089
XML_SCHEMAP_A_PROPS_CORRECT_3 = 3090
XML_SCHEMAP_COS_ALL_LIMITED = 3091
XML_SCHEMATRONV_ASSERT = 4000
XML_SCHEMATRONV_REPORT = 4001
XML_MODULE_OPEN = 4900
XML_MODULE_CLOSE = 4901
XML_CHECK_FOUND_ELEMENT = 5000
XML_CHECK_FOUND_ATTRIBUTE = 5001
XML_CHECK_FOUND_TEXT = 5002
XML_CHECK_FOUND_CDATA = 5003
XML_CHECK_FOUND_ENTITYREF = 5004
XML_CHECK_FOUND_ENTITY = 5005
XML_CHECK_FOUND_PI = 5006
XML_CHECK_FOUND_COMMENT = 5007
XML_CHECK_FOUND_DOCTYPE = 5008
XML_CHECK_FOUND_FRAGMENT = 5009
XML_CHECK_FOUND_NOTATION = 5010
XML_CHECK_UNKNOWN_NODE = 5011
XML_CHECK_ENTITY_TYPE = 5012
XML_CHECK_NO_PARENT = 5013
XML_CHECK_NO_DOC = 5014
XML_CHECK_NO_NAME = 5015
XML_CHECK_NO_ELEM = 5016
XML_CHECK_WRONG_DOC = 5017
XML_CHECK_NO_PREV = 5018
XML_CHECK_WRONG_PREV = 5019
XML_CHECK_NO_NEXT = 5020
XML_CHECK_WRONG_NEXT = 5021
XML_CHECK_NOT_DTD = 5022
XML_CHECK_NOT_ATTR = 5023
XML_CHECK_NOT_ATTR_DECL = 5024
XML_CHECK_NOT_ELEM_DECL = 5025
XML_CHECK_NOT_ENTITY_DECL = 5026
XML_CHECK_NOT_NS_DECL = 5027
XML_CHECK_NO_HREF = 5028
XML_CHECK_WRONG_PARENT = 5029
XML_CHECK_NS_SCOPE = 5030
XML_CHECK_NS_ANCESTOR = 5031
XML_CHECK_NOT_UTF8 = 5032
XML_CHECK_NO_DICT = 5033
XML_CHECK_NOT_NCNAME = 5034
XML_CHECK_OUTSIDE_DICT = 5035
XML_CHECK_WRONG_NAME = 5036
XML_CHECK_NAME_NOT_NULL = 5037
XML_I18N_NO_NAME = 6000
XML_I18N_NO_HANDLER = 6001
XML_I18N_EXCESS_HANDLER = 6002
XML_I18N_CONV_FAILED = 6003
XML_I18N_NO_OUTPUT = 6004
XML_BUF_OVERFLOW = 7000

# xmlExpNodeType
XML_EXP_EMPTY = 0
XML_EXP_FORBID = 1
XML_EXP_ATOM = 2
XML_EXP_SEQ = 3
XML_EXP_OR = 4
XML_EXP_COUNT = 5

# xmlElementContentType
XML_ELEMENT_CONTENT_PCDATA = 1
XML_ELEMENT_CONTENT_ELEMENT = 2
XML_ELEMENT_CONTENT_SEQ = 3
XML_ELEMENT_CONTENT_OR = 4

# xmlParserProperties
XML_PARSER_LOADDTD = 1
XML_PARSER_DEFAULTATTRS = 2
XML_PARSER_VALIDATE = 3
XML_PARSER_SUBST_ENTITIES = 4

# xmlReaderTypes
XML_READER_TYPE_NONE = 0
XML_READER_TYPE_ELEMENT = 1
XML_READER_TYPE_ATTRIBUTE = 2
XML_READER_TYPE_TEXT = 3
XML_READER_TYPE_CDATA = 4
XML_READER_TYPE_ENTITY_REFERENCE = 5
XML_READER_TYPE_ENTITY = 6
XML_READER_TYPE_PROCESSING_INSTRUCTION = 7
XML_READER_TYPE_COMMENT = 8
XML_READER_TYPE_DOCUMENT = 9
XML_READER_TYPE_DOCUMENT_TYPE = 10
XML_READER_TYPE_DOCUMENT_FRAGMENT = 11
XML_READER_TYPE_NOTATION = 12
XML_READER_TYPE_WHITESPACE = 13
XML_READER_TYPE_SIGNIFICANT_WHITESPACE = 14
XML_READER_TYPE_END_ELEMENT = 15
XML_READER_TYPE_END_ENTITY = 16
XML_READER_TYPE_XML_DECLARATION = 17

# xmlCatalogPrefer
XML_CATA_PREFER_NONE = 0
XML_CATA_PREFER_PUBLIC = 1
XML_CATA_PREFER_SYSTEM = 2

# xmlElementType
XML_ELEMENT_NODE = 1
XML_ATTRIBUTE_NODE = 2
XML_TEXT_NODE = 3
XML_CDATA_SECTION_NODE = 4
XML_ENTITY_REF_NODE = 5
XML_ENTITY_NODE = 6
XML_PI_NODE = 7
XML_COMMENT_NODE = 8
XML_DOCUMENT_NODE = 9
XML_DOCUMENT_TYPE_NODE = 10
XML_DOCUMENT_FRAG_NODE = 11
XML_NOTATION_NODE = 12
XML_HTML_DOCUMENT_NODE = 13
XML_DTD_NODE = 14
XML_ELEMENT_DECL = 15
XML_ATTRIBUTE_DECL = 16
XML_ENTITY_DECL = 17
XML_NAMESPACE_DECL = 18
XML_XINCLUDE_START = 19
XML_XINCLUDE_END = 20
XML_DOCB_DOCUMENT_NODE = 21

# xlinkActuate
XLINK_ACTUATE_NONE = 0
XLINK_ACTUATE_AUTO = 1
XLINK_ACTUATE_ONREQUEST = 2

# xmlFeature
XML_WITH_THREAD = 1
XML_WITH_TREE = 2
XML_WITH_OUTPUT = 3
XML_WITH_PUSH = 4
XML_WITH_READER = 5
XML_WITH_PATTERN = 6
XML_WITH_WRITER = 7
XML_WITH_SAX1 = 8
XML_WITH_FTP = 9
XML_WITH_HTTP = 10
XML_WITH_VALID = 11
XML_WITH_HTML = 12
XML_WITH_LEGACY = 13
XML_WITH_C14N = 14
XML_WITH_CATALOG = 15
XML_WITH_XPATH = 16
XML_WITH_XPTR = 17
XML_WITH_XINCLUDE = 18
XML_WITH_ICONV = 19
XML_WITH_ISO8859X = 20
XML_WITH_UNICODE = 21
XML_WITH_REGEXP = 22
XML_WITH_AUTOMATA = 23
XML_WITH_EXPR = 24
XML_WITH_SCHEMAS = 25
XML_WITH_SCHEMATRON = 26
XML_WITH_MODULES = 27
XML_WITH_DEBUG = 28
XML_WITH_DEBUG_MEM = 29
XML_WITH_DEBUG_RUN = 30
XML_WITH_ZLIB = 31
XML_WITH_ICU = 32
XML_WITH_LZMA = 33
XML_WITH_NONE = 99999

# xmlElementContentOccur
XML_ELEMENT_CONTENT_ONCE = 1
XML_ELEMENT_CONTENT_OPT = 2
XML_ELEMENT_CONTENT_MULT = 3
XML_ELEMENT_CONTENT_PLUS = 4

# xmlXPathError
XPATH_EXPRESSION_OK = 0
XPATH_NUMBER_ERROR = 1
XPATH_UNFINISHED_LITERAL_ERROR = 2
XPATH_START_LITERAL_ERROR = 3
XPATH_VARIABLE_REF_ERROR = 4
XPATH_UNDEF_VARIABLE_ERROR = 5
XPATH_INVALID_PREDICATE_ERROR = 6
XPATH_EXPR_ERROR = 7
XPATH_UNCLOSED_ERROR = 8
XPATH_UNKNOWN_FUNC_ERROR = 9
XPATH_INVALID_OPERAND = 10
XPATH_INVALID_TYPE = 11
XPATH_INVALID_ARITY = 12
XPATH_INVALID_CTXT_SIZE = 13
XPATH_INVALID_CTXT_POSITION = 14
XPATH_MEMORY_ERROR = 15
XPTR_SYNTAX_ERROR = 16
XPTR_RESOURCE_ERROR = 17
XPTR_SUB_RESOURCE_ERROR = 18
XPATH_UNDEF_PREFIX_ERROR = 19
XPATH_ENCODING_ERROR = 20
XPATH_INVALID_CHAR_ERROR = 21
XPATH_INVALID_CTXT = 22
XPATH_STACK_ERROR = 23
XPATH_FORBID_VARIABLE_ERROR = 24

# xmlTextReaderMode
XML_TEXTREADER_MODE_INITIAL = 0
XML_TEXTREADER_MODE_INTERACTIVE = 1
XML_TEXTREADER_MODE_ERROR = 2
XML_TEXTREADER_MODE_EOF = 3
XML_TEXTREADER_MODE_CLOSED = 4
XML_TEXTREADER_MODE_READING = 5

# xmlErrorLevel
XML_ERR_NONE = 0
XML_ERR_WARNING = 1
XML_ERR_ERROR = 2
XML_ERR_FATAL = 3

# xmlCharEncoding
XML_CHAR_ENCODING_ERROR = -1
XML_CHAR_ENCODING_NONE = 0
XML_CHAR_ENCODING_UTF8 = 1
XML_CHAR_ENCODING_UTF16LE = 2
XML_CHAR_ENCODING_UTF16BE = 3
XML_CHAR_ENCODING_UCS4LE = 4
XML_CHAR_ENCODING_UCS4BE = 5
XML_CHAR_ENCODING_EBCDIC = 6
XML_CHAR_ENCODING_UCS4_2143 = 7
XML_CHAR_ENCODING_UCS4_3412 = 8
XML_CHAR_ENCODING_UCS2 = 9
XML_CHAR_ENCODING_8859_1 = 10
XML_CHAR_ENCODING_8859_2 = 11
XML_CHAR_ENCODING_8859_3 = 12
XML_CHAR_ENCODING_8859_4 = 13
XML_CHAR_ENCODING_8859_5 = 14
XML_CHAR_ENCODING_8859_6 = 15
XML_CHAR_ENCODING_8859_7 = 16
XML_CHAR_ENCODING_8859_8 = 17
XML_CHAR_ENCODING_8859_9 = 18
XML_CHAR_ENCODING_2022_JP = 19
XML_CHAR_ENCODING_SHIFT_JIS = 20
XML_CHAR_ENCODING_EUC_JP = 21
XML_CHAR_ENCODING_ASCII = 22

# xmlErrorDomain
XML_FROM_NONE = 0
XML_FROM_PARSER = 1
XML_FROM_TREE = 2
XML_FROM_NAMESPACE = 3
XML_FROM_DTD = 4
XML_FROM_HTML = 5
XML_FROM_MEMORY = 6
XML_FROM_OUTPUT = 7
XML_FROM_IO = 8
XML_FROM_FTP = 9
XML_FROM_HTTP = 10
XML_FROM_XINCLUDE = 11
XML_FROM_XPATH = 12
XML_FROM_XPOINTER = 13
XML_FROM_REGEXP = 14
XML_FROM_DATATYPE = 15
XML_FROM_SCHEMASP = 16
XML_FROM_SCHEMASV = 17
XML_FROM_RELAXNGP = 18
XML_FROM_RELAXNGV = 19
XML_FROM_CATALOG = 20
XML_FROM_C14N = 21
XML_FROM_XSLT = 22
XML_FROM_VALID = 23
XML_FROM_CHECK = 24
XML_FROM_WRITER = 25
XML_FROM_MODULE = 26
XML_FROM_I18N = 27
XML_FROM_SCHEMATRONV = 28
XML_FROM_BUFFER = 29
XML_FROM_URI = 30

# htmlStatus
HTML_NA = 0
HTML_INVALID = 1
HTML_DEPRECATED = 2
HTML_VALID = 4
HTML_REQUIRED = 12

# xmlSchemaValidOption
XML_SCHEMA_VAL_VC_I_CREATE = 1

# xmlSchemaWhitespaceValueType
XML_SCHEMA_WHITESPACE_UNKNOWN = 0
XML_SCHEMA_WHITESPACE_PRESERVE = 1
XML_SCHEMA_WHITESPACE_REPLACE = 2
XML_SCHEMA_WHITESPACE_COLLAPSE = 3

# htmlParserOption
HTML_PARSE_RECOVER = 1
HTML_PARSE_NODEFDTD = 4
HTML_PARSE_NOERROR = 32
HTML_PARSE_NOWARNING = 64
HTML_PARSE_PEDANTIC = 128
HTML_PARSE_NOBLANKS = 256
HTML_PARSE_NONET = 2048
HTML_PARSE_NOIMPLIED = 8192
HTML_PARSE_COMPACT = 65536
HTML_PARSE_IGNORE_ENC = 2097152

# xmlRelaxNGValidErr
XML_RELAXNG_OK = 0
XML_RELAXNG_ERR_MEMORY = 1
XML_RELAXNG_ERR_TYPE = 2
XML_RELAXNG_ERR_TYPEVAL = 3
XML_RELAXNG_ERR_DUPID = 4
XML_RELAXNG_ERR_TYPECMP = 5
XML_RELAXNG_ERR_NOSTATE = 6
XML_RELAXNG_ERR_NODEFINE = 7
XML_RELAXNG_ERR_LISTEXTRA = 8
XML_RELAXNG_ERR_LISTEMPTY = 9
XML_RELAXNG_ERR_INTERNODATA = 10
XML_RELAXNG_ERR_INTERSEQ = 11
XML_RELAXNG_ERR_INTEREXTRA = 12
XML_RELAXNG_ERR_ELEMNAME = 13
XML_RELAXNG_ERR_ATTRNAME = 14
XML_RELAXNG_ERR_ELEMNONS = 15
XML_RELAXNG_ERR_ATTRNONS = 16
XML_RELAXNG_ERR_ELEMWRONGNS = 17
XML_RELAXNG_ERR_ATTRWRONGNS = 18
XML_RELAXNG_ERR_ELEMEXTRANS = 19
XML_RELAXNG_ERR_ATTREXTRANS = 20
XML_RELAXNG_ERR_ELEMNOTEMPTY = 21
XML_RELAXNG_ERR_NOELEM = 22
XML_RELAXNG_ERR_NOTELEM = 23
XML_RELAXNG_ERR_ATTRVALID = 24
XML_RELAXNG_ERR_CONTENTVALID = 25
XML_RELAXNG_ERR_EXTRACONTENT = 26
XML_RELAXNG_ERR_INVALIDATTR = 27
XML_RELAXNG_ERR_DATAELEM = 28
XML_RELAXNG_ERR_VALELEM = 29
XML_RELAXNG_ERR_LISTELEM = 30
XML_RELAXNG_ERR_DATATYPE = 31
XML_RELAXNG_ERR_VALUE = 32
XML_RELAXNG_ERR_LIST = 33
XML_RELAXNG_ERR_NOGRAMMAR = 34
XML_RELAXNG_ERR_EXTRADATA = 35
XML_RELAXNG_ERR_LACKDATA = 36
XML_RELAXNG_ERR_INTERNAL = 37
XML_RELAXNG_ERR_ELEMWRONG = 38
XML_RELAXNG_ERR_TEXTWRONG = 39

# xmlCatalogAllow
XML_CATA_ALLOW_NONE = 0
XML_CATA_ALLOW_GLOBAL = 1
XML_CATA_ALLOW_DOCUMENT = 2
XML_CATA_ALLOW_ALL = 3

# xmlAttributeType
XML_ATTRIBUTE_CDATA = 1
XML_ATTRIBUTE_ID = 2
XML_ATTRIBUTE_IDREF = 3
XML_ATTRIBUTE_IDREFS = 4
XML_ATTRIBUTE_ENTITY = 5
XML_ATTRIBUTE_ENTITIES = 6
XML_ATTRIBUTE_NMTOKEN = 7
XML_ATTRIBUTE_NMTOKENS = 8
XML_ATTRIBUTE_ENUMERATION = 9
XML_ATTRIBUTE_NOTATION = 10

# xmlSchematronValidOptions
XML_SCHEMATRON_OUT_QUIET = 1
XML_SCHEMATRON_OUT_TEXT = 2
XML_SCHEMATRON_OUT_XML = 4
XML_SCHEMATRON_OUT_ERROR = 8
XML_SCHEMATRON_OUT_FILE = 256
XML_SCHEMATRON_OUT_BUFFER = 512
XML_SCHEMATRON_OUT_IO = 1024

# xmlSchemaContentType
XML_SCHEMA_CONTENT_UNKNOWN = 0
XML_SCHEMA_CONTENT_EMPTY = 1
XML_SCHEMA_CONTENT_ELEMENTS = 2
XML_SCHEMA_CONTENT_MIXED = 3
XML_SCHEMA_CONTENT_SIMPLE = 4
XML_SCHEMA_CONTENT_MIXED_OR_ELEMENTS = 5
XML_SCHEMA_CONTENT_BASIC = 6
XML_SCHEMA_CONTENT_ANY = 7

# xmlSchemaTypeType
XML_SCHEMA_TYPE_BASIC = 1
XML_SCHEMA_TYPE_ANY = 2
XML_SCHEMA_TYPE_FACET = 3
XML_SCHEMA_TYPE_SIMPLE = 4
XML_SCHEMA_TYPE_COMPLEX = 5
XML_SCHEMA_TYPE_SEQUENCE = 6
XML_SCHEMA_TYPE_CHOICE = 7
XML_SCHEMA_TYPE_ALL = 8
XML_SCHEMA_TYPE_SIMPLE_CONTENT = 9
XML_SCHEMA_TYPE_COMPLEX_CONTENT = 10
XML_SCHEMA_TYPE_UR = 11
XML_SCHEMA_TYPE_RESTRICTION = 12
XML_SCHEMA_TYPE_EXTENSION = 13
XML_SCHEMA_TYPE_ELEMENT = 14
XML_SCHEMA_TYPE_ATTRIBUTE = 15
XML_SCHEMA_TYPE_ATTRIBUTEGROUP = 16
XML_SCHEMA_TYPE_GROUP = 17
XML_SCHEMA_TYPE_NOTATION = 18
XML_SCHEMA_TYPE_LIST = 19
XML_SCHEMA_TYPE_UNION = 20
XML_SCHEMA_TYPE_ANY_ATTRIBUTE = 21
XML_SCHEMA_TYPE_IDC_UNIQUE = 22
XML_SCHEMA_TYPE_IDC_KEY = 23
XML_SCHEMA_TYPE_IDC_KEYREF = 24
XML_SCHEMA_TYPE_PARTICLE = 25
XML_SCHEMA_TYPE_ATTRIBUTE_USE = 26
XML_SCHEMA_FACET_MININCLUSIVE = 1000
XML_SCHEMA_FACET_MINEXCLUSIVE = 1001
XML_SCHEMA_FACET_MAXINCLUSIVE = 1002
XML_SCHEMA_FACET_MAXEXCLUSIVE = 1003
XML_SCHEMA_FACET_TOTALDIGITS = 1004
XML_SCHEMA_FACET_FRACTIONDIGITS = 1005
XML_SCHEMA_FACET_PATTERN = 1006
XML_SCHEMA_FACET_ENUMERATION = 1007
XML_SCHEMA_FACET_WHITESPACE = 1008
XML_SCHEMA_FACET_LENGTH = 1009
XML_SCHEMA_FACET_MAXLENGTH = 1010
XML_SCHEMA_FACET_MINLENGTH = 1011
XML_SCHEMA_EXTRA_QNAMEREF = 2000
XML_SCHEMA_EXTRA_ATTR_USE_PROHIB = 2001

# xmlModuleOption
XML_MODULE_LAZY = 1
XML_MODULE_LOCAL = 2

# xmlParserMode
XML_PARSE_UNKNOWN = 0
XML_PARSE_DOM = 1
XML_PARSE_SAX = 2
XML_PARSE_PUSH_DOM = 3
XML_PARSE_PUSH_SAX = 4
XML_PARSE_READER = 5

# xmlC14NMode
XML_C14N_1_0 = 0
XML_C14N_EXCLUSIVE_1_0 = 1
XML_C14N_1_1 = 2

# xmlParserOption
XML_PARSE_RECOVER = 1
XML_PARSE_NOENT = 2
XML_PARSE_DTDLOAD = 4
XML_PARSE_DTDATTR = 8
XML_PARSE_DTDVALID = 16
XML_PARSE_NOERROR = 32
XML_PARSE_NOWARNING = 64
XML_PARSE_PEDANTIC = 128
XML_PARSE_NOBLANKS = 256
XML_PARSE_SAX1 = 512
XML_PARSE_XINCLUDE = 1024
XML_PARSE_NONET = 2048
XML_PARSE_NODICT = 4096
XML_PARSE_NSCLEAN = 8192
XML_PARSE_NOCDATA = 16384
XML_PARSE_NOXINCNODE = 32768
XML_PARSE_COMPACT = 65536
XML_PARSE_OLD10 = 131072
XML_PARSE_NOBASEFIX = 262144
XML_PARSE_HUGE = 524288
XML_PARSE_OLDSAX = 1048576
XML_PARSE_IGNORE_ENC = 2097152
XML_PARSE_BIG_LINES = 4194304

# xmlElementTypeVal
XML_ELEMENT_TYPE_UNDEFINED = 0
XML_ELEMENT_TYPE_EMPTY = 1
XML_ELEMENT_TYPE_ANY = 2
XML_ELEMENT_TYPE_MIXED = 3
XML_ELEMENT_TYPE_ELEMENT = 4

# xmlDocProperties
XML_DOC_WELLFORMED = 1
XML_DOC_NSVALID = 2
XML_DOC_OLD10 = 4
XML_DOC_DTDVALID = 8
XML_DOC_XINCLUDE = 16
XML_DOC_USERBUILT = 32
XML_DOC_INTERNAL = 64
XML_DOC_HTML = 128

# xlinkType
XLINK_TYPE_NONE = 0
XLINK_TYPE_SIMPLE = 1
XLINK_TYPE_EXTENDED = 2
XLINK_TYPE_EXTENDED_SET = 3

# xmlXPathObjectType
XPATH_UNDEFINED = 0
XPATH_NODESET = 1
XPATH_BOOLEAN = 2
XPATH_NUMBER = 3
XPATH_STRING = 4
XPATH_POINT = 5
XPATH_RANGE = 6
XPATH_LOCATIONSET = 7
XPATH_USERS = 8
XPATH_XSLT_TREE = 9

# xmlSchemaValidError
XML_SCHEMAS_ERR_OK = 0
XML_SCHEMAS_ERR_NOROOT = 1
XML_SCHEMAS_ERR_UNDECLAREDELEM = 2
XML_SCHEMAS_ERR_NOTTOPLEVEL = 3
XML_SCHEMAS_ERR_MISSING = 4
XML_SCHEMAS_ERR_WRONGELEM = 5
XML_SCHEMAS_ERR_NOTYPE = 6
XML_SCHEMAS_ERR_NOROLLBACK = 7
XML_SCHEMAS_ERR_ISABSTRACT = 8
XML_SCHEMAS_ERR_NOTEMPTY = 9
XML_SCHEMAS_ERR_ELEMCONT = 10
XML_SCHEMAS_ERR_HAVEDEFAULT = 11
XML_SCHEMAS_ERR_NOTNILLABLE = 12
XML_SCHEMAS_ERR_EXTRACONTENT = 13
XML_SCHEMAS_ERR_INVALIDATTR = 14
XML_SCHEMAS_ERR_INVALIDELEM = 15
XML_SCHEMAS_ERR_NOTDETERMINIST = 16
XML_SCHEMAS_ERR_CONSTRUCT = 17
XML_SCHEMAS_ERR_INTERNAL = 18
XML_SCHEMAS_ERR_NOTSIMPLE = 19
XML_SCHEMAS_ERR_ATTRUNKNOWN = 20
XML_SCHEMAS_ERR_ATTRINVALID = 21
XML_SCHEMAS_ERR_VALUE = 22
XML_SCHEMAS_ERR_FACET = 23
XML_SCHEMAS_ERR_ = 24
XML_SCHEMAS_ERR_XXX = 25
