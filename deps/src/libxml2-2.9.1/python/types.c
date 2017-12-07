/*
 * types.c: converter functions between the internal representation
 *          and the Python objects
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#include "libxml_wrap.h"
#include <libxml/xpathInternals.h>

#if PY_MAJOR_VERSION >= 3
#define PY_IMPORT_STRING_SIZE PyUnicode_FromStringAndSize
#define PY_IMPORT_STRING PyUnicode_FromString
#define PY_IMPORT_INT PyLong_FromLong
#else
#define PY_IMPORT_STRING_SIZE PyString_FromStringAndSize
#define PY_IMPORT_STRING PyString_FromString
#define PY_IMPORT_INT PyInt_FromLong
#endif

#if PY_MAJOR_VERSION >= 3
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

FILE *
libxml_PyFileGet(PyObject *f) {
    int fd, flags;
    FILE *res;
    const char *mode;

    fd = PyObject_AsFileDescriptor(f);
    if (!_PyVerify_fd(fd))
        return(NULL);
    /*
     * Get the flags on the fd to understand how it was opened
     */
    flags = fcntl(fd, F_GETFL, 0);
    switch (flags & O_ACCMODE) {
        case O_RDWR:
	    if (flags & O_APPEND)
	        mode = "a+";
	    else
	        mode = "rw";
	    break;
        case O_RDONLY:
	    if (flags & O_APPEND)
	        mode = "r+";
	    else
	        mode = "r";
	    break;
	case O_WRONLY:
	    if (flags & O_APPEND)
	        mode = "a";
	    else
	        mode = "w";
	    break;
	default:
	    return(NULL);
    }

    /*
     * the FILE struct gets a new fd, so that it can be closed
     * independently of the file descriptor given. The risk though is
     * lack of sync. So at the python level sync must be implemented
     * before and after a conversion took place. No way around it
     * in the Python3 infrastructure !
     * The duplicated fd and FILE * will be released in the subsequent
     * call to libxml_PyFileRelease() which must be genrated accodingly
     */
    fd = dup(fd);
    if (fd == -1)
        return(NULL);
    res = fdopen(fd, mode);
    if (res == NULL) {
        close(fd);
	return(NULL);
    }
    return(res);
}

void libxml_PyFileRelease(FILE *f) {
    if (f != NULL)
        fclose(f);
}
#endif

PyObject *
libxml_intWrap(int val)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_intWrap: val = %d\n", val);
#endif
    ret = PY_IMPORT_INT((long) val);
    return (ret);
}

PyObject *
libxml_longWrap(long val)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_longWrap: val = %ld\n", val);
#endif
    ret = PyLong_FromLong(val);
    return (ret);
}

PyObject *
libxml_doubleWrap(double val)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_doubleWrap: val = %f\n", val);
#endif
    ret = PyFloat_FromDouble((double) val);
    return (ret);
}

PyObject *
libxml_charPtrWrap(char *str)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlcharPtrWrap: str = %s\n", str);
#endif
    if (str == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PY_IMPORT_STRING(str);
    xmlFree(str);
    return (ret);
}

PyObject *
libxml_charPtrConstWrap(const char *str)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlcharPtrWrap: str = %s\n", str);
#endif
    if (str == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PY_IMPORT_STRING(str);
    return (ret);
}

PyObject *
libxml_xmlCharPtrWrap(xmlChar * str)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlCharPtrWrap: str = %s\n", str);
#endif
    if (str == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PY_IMPORT_STRING((char *) str);
    xmlFree(str);
    return (ret);
}

PyObject *
libxml_xmlCharPtrConstWrap(const xmlChar * str)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlCharPtrWrap: str = %s\n", str);
#endif
    if (str == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PY_IMPORT_STRING((char *) str);
    return (ret);
}

PyObject *
libxml_constcharPtrWrap(const char *str)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlcharPtrWrap: str = %s\n", str);
#endif
    if (str == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PY_IMPORT_STRING(str);
    return (ret);
}

PyObject *
libxml_constxmlCharPtrWrap(const xmlChar * str)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlCharPtrWrap: str = %s\n", str);
#endif
    if (str == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PY_IMPORT_STRING((char *) str);
    return (ret);
}

PyObject *
libxml_xmlDocPtrWrap(xmlDocPtr doc)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlDocPtrWrap: doc = %p\n", doc);
#endif
    if (doc == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    /* TODO: look at deallocation */
    ret = PyCapsule_New((void *) doc, (char *) "xmlDocPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlNodePtrWrap(xmlNodePtr node)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlNodePtrWrap: node = %p\n", node);
#endif
    if (node == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *) node, (char *) "xmlNodePtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlURIPtrWrap(xmlURIPtr uri)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlURIPtrWrap: uri = %p\n", uri);
#endif
    if (uri == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *) uri, (char *) "xmlURIPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlNsPtrWrap(xmlNsPtr ns)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlNsPtrWrap: node = %p\n", ns);
#endif
    if (ns == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *) ns, (char *) "xmlNsPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlAttrPtrWrap(xmlAttrPtr attr)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlAttrNodePtrWrap: attr = %p\n", attr);
#endif
    if (attr == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *) attr, (char *) "xmlAttrPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlAttributePtrWrap(xmlAttributePtr attr)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlAttributePtrWrap: attr = %p\n", attr);
#endif
    if (attr == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *) attr, (char *) "xmlAttributePtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlElementPtrWrap(xmlElementPtr elem)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlElementNodePtrWrap: elem = %p\n", elem);
#endif
    if (elem == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *) elem, (char *) "xmlElementPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlXPathContextPtrWrap(xmlXPathContextPtr ctxt)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlXPathContextPtrWrap: ctxt = %p\n", ctxt);
#endif
    if (ctxt == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *) ctxt, (char *) "xmlXPathContextPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlXPathParserContextPtrWrap(xmlXPathParserContextPtr ctxt)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlXPathParserContextPtrWrap: ctxt = %p\n", ctxt);
#endif
    if (ctxt == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *)ctxt, (char *)"xmlXPathParserContextPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlParserCtxtPtrWrap(xmlParserCtxtPtr ctxt)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlParserCtxtPtrWrap: ctxt = %p\n", ctxt);
#endif
    if (ctxt == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }

    ret = PyCapsule_New((void *) ctxt, (char *) "xmlParserCtxtPtr", NULL);
    return (ret);
}

/**
 * libxml_xmlXPathDestructNsNode:
 * cap: xmlNsPtr namespace node capsule object
 *
 * This function is called if and when a namespace node returned in
 * an XPath node set is to be destroyed. That's the only kind of
 * object returned in node set not directly linked to the original
 * xmlDoc document, see xmlXPathNodeSetDupNs.
 */
#if PY_VERSION_HEX < 0x02070000
static void
libxml_xmlXPathDestructNsNode(void *cap, void *desc ATTRIBUTE_UNUSED)
#else
static void
libxml_xmlXPathDestructNsNode(PyObject *cap)
#endif
{
#ifdef DEBUG
    fprintf(stderr, "libxml_xmlXPathDestructNsNode called %p\n", cap);
#endif
#if PY_VERSION_HEX < 0x02070000
    xmlXPathNodeSetFreeNs((xmlNsPtr) cap);
#else
    xmlXPathNodeSetFreeNs((xmlNsPtr) PyCapsule_GetPointer(cap, "xmlNsPtr"));
#endif
}

PyObject *
libxml_xmlXPathObjectPtrWrap(xmlXPathObjectPtr obj)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlXPathObjectPtrWrap: ctxt = %p\n", obj);
#endif
    if (obj == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    switch (obj->type) {
        case XPATH_XSLT_TREE: {
            if ((obj->nodesetval == NULL) ||
		(obj->nodesetval->nodeNr == 0) ||
		(obj->nodesetval->nodeTab == NULL)) {
                ret = PyList_New(0);
	    } else {
		int i, len = 0;
		xmlNodePtr node;

		node = obj->nodesetval->nodeTab[0]->children;
		while (node != NULL) {
		    len++;
		    node = node->next;
		}
		ret = PyList_New(len);
		node = obj->nodesetval->nodeTab[0]->children;
		for (i = 0;i < len;i++) {
                    PyList_SetItem(ret, i, libxml_xmlNodePtrWrap(node));
		    node = node->next;
		}
	    }
	    /*
	     * Return now, do not free the object passed down
	     */
	    return (ret);
	}
        case XPATH_NODESET:
            if ((obj->nodesetval == NULL)
                || (obj->nodesetval->nodeNr == 0)) {
                ret = PyList_New(0);
	    } else {
                int i;
                xmlNodePtr node;

                ret = PyList_New(obj->nodesetval->nodeNr);
                for (i = 0; i < obj->nodesetval->nodeNr; i++) {
                    node = obj->nodesetval->nodeTab[i];
                    if (node->type == XML_NAMESPACE_DECL) {
		        PyObject *ns = PyCapsule_New((void *) node,
                                     (char *) "xmlNsPtr",
				     libxml_xmlXPathDestructNsNode);
			PyList_SetItem(ret, i, ns);
			/* make sure the xmlNsPtr is not destroyed now */
			obj->nodesetval->nodeTab[i] = NULL;
		    } else {
			PyList_SetItem(ret, i, libxml_xmlNodePtrWrap(node));
		    }
                }
            }
            break;
        case XPATH_BOOLEAN:
            ret = PY_IMPORT_INT((long) obj->boolval);
            break;
        case XPATH_NUMBER:
            ret = PyFloat_FromDouble(obj->floatval);
            break;
        case XPATH_STRING:
	    ret = PY_IMPORT_STRING((char *) obj->stringval);
            break;
        case XPATH_POINT:
        {
            PyObject *node;
            PyObject *indexIntoNode;
            PyObject *tuple;

            node = libxml_xmlNodePtrWrap(obj->user);
            indexIntoNode = PY_IMPORT_INT((long) obj->index);

            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, node);
            PyTuple_SetItem(tuple, 1, indexIntoNode);

            ret = tuple;
            break;
        }
        case XPATH_RANGE:
        {
            unsigned short bCollapsedRange;

            bCollapsedRange = ( (obj->user2 == NULL) ||
		                ((obj->user2 == obj->user) && (obj->index == obj->index2)) );
            if ( bCollapsedRange ) {
                PyObject *node;
                PyObject *indexIntoNode;
                PyObject *tuple;
                PyObject *list;

                list = PyList_New(1);

                node = libxml_xmlNodePtrWrap(obj->user);
                indexIntoNode = PY_IMPORT_INT((long) obj->index);

                tuple = PyTuple_New(2);
                PyTuple_SetItem(tuple, 0, node);
                PyTuple_SetItem(tuple, 1, indexIntoNode);

                PyList_SetItem(list, 0, tuple);

                ret = list;
            } else {
                PyObject *node;
                PyObject *indexIntoNode;
                PyObject *tuple;
                PyObject *list;

                list = PyList_New(2);

                node = libxml_xmlNodePtrWrap(obj->user);
                indexIntoNode = PY_IMPORT_INT((long) obj->index);

                tuple = PyTuple_New(2);
                PyTuple_SetItem(tuple, 0, node);
                PyTuple_SetItem(tuple, 1, indexIntoNode);

                PyList_SetItem(list, 0, tuple);

                node = libxml_xmlNodePtrWrap(obj->user2);
                indexIntoNode = PY_IMPORT_INT((long) obj->index2);

                tuple = PyTuple_New(2);
                PyTuple_SetItem(tuple, 0, node);
                PyTuple_SetItem(tuple, 1, indexIntoNode);

                PyList_SetItem(list, 1, tuple);

                ret = list;
            }
            break;
        }
        case XPATH_LOCATIONSET:
        {
            xmlLocationSetPtr set;

            set = obj->user;
            if ( set && set->locNr > 0 ) {
                int i;
                PyObject *list;

                list = PyList_New(set->locNr);

                for (i=0; i<set->locNr; i++) {
                    xmlXPathObjectPtr setobj;
                    PyObject *pyobj;

                    setobj = set->locTab[i]; /*xmlXPathObjectPtr setobj*/

                    pyobj = libxml_xmlXPathObjectPtrWrap(setobj);
                    /* xmlXPathFreeObject(setobj) is called */
                    set->locTab[i] = NULL;

                    PyList_SetItem(list, i, pyobj);
                }
                set->locNr = 0;
                ret = list;
            } else {
                Py_INCREF(Py_None);
                ret = Py_None;
            }
            break;
        }
        default:
#ifdef DEBUG
            printf("Unable to convert XPath object type %d\n", obj->type);
#endif
            Py_INCREF(Py_None);
            ret = Py_None;
    }
    xmlXPathFreeObject(obj);
    return (ret);
}

xmlXPathObjectPtr
libxml_xmlXPathObjectPtrConvert(PyObject * obj)
{
    xmlXPathObjectPtr ret = NULL;

#ifdef DEBUG
    printf("libxml_xmlXPathObjectPtrConvert: obj = %p\n", obj);
#endif
    if (obj == NULL) {
        return (NULL);
    }
    if PyFloat_Check (obj) {
        ret = xmlXPathNewFloat((double) PyFloat_AS_DOUBLE(obj));
    } else if PyLong_Check(obj) {
#ifdef PyLong_AS_LONG
        ret = xmlXPathNewFloat((double) PyLong_AS_LONG(obj));
#else
        ret = xmlXPathNewFloat((double) PyInt_AS_LONG(obj));
#endif
#ifdef PyBool_Check
    } else if PyBool_Check (obj) {

        if (obj == Py_True) {
          ret = xmlXPathNewBoolean(1);
        }
        else {
          ret = xmlXPathNewBoolean(0);
        }
#endif
    } else if PyBytes_Check (obj) {
        xmlChar *str;

        str = xmlStrndup((const xmlChar *) PyBytes_AS_STRING(obj),
                         PyBytes_GET_SIZE(obj));
        ret = xmlXPathWrapString(str);
#ifdef PyUnicode_Check
    } else if PyUnicode_Check (obj) {
#if PY_VERSION_HEX >= 0x03030000
        xmlChar *str;
	const char *tmp;
	size_t size;

	/* tmp doesn't need to be deallocated */
        tmp = PyUnicode_AsUTF8AndSize(obj, &size);
        str = xmlStrndup(tmp, (int) size);
        ret = xmlXPathWrapString(str);
#else
        xmlChar *str = NULL;
        PyObject *b;

	b = PyUnicode_AsUTF8String(obj);
	if (b != NULL) {
	    str = xmlStrndup((const xmlChar *) PyBytes_AS_STRING(b),
			     PyBytes_GET_SIZE(b));
	    Py_DECREF(b);
	}
	ret = xmlXPathWrapString(str);
#endif
#endif
    } else if PyList_Check (obj) {
        int i;
        PyObject *node;
        xmlNodePtr cur;
        xmlNodeSetPtr set;

        set = xmlXPathNodeSetCreate(NULL);

        for (i = 0; i < PyList_Size(obj); i++) {
            node = PyList_GetItem(obj, i);
            if ((node == NULL) || (node->ob_type == NULL))
                continue;

            cur = NULL;
            if (PyCapsule_CheckExact(node)) {
#ifdef DEBUG
                printf("Got a Capsule\n");
#endif
                cur = PyxmlNode_Get(node);
            } else if ((PyObject_HasAttrString(node, (char *) "_o")) &&
	               (PyObject_HasAttrString(node, (char *) "get_doc"))) {
		PyObject *wrapper;

		wrapper = PyObject_GetAttrString(node, (char *) "_o");
		if (wrapper != NULL)
		    cur = PyxmlNode_Get(wrapper);
            } else {
#ifdef DEBUG
                printf("Unknown object in Python return list\n");
#endif
            }
            if (cur != NULL) {
                xmlXPathNodeSetAdd(set, cur);
            }
        }
        ret = xmlXPathWrapNodeSet(set);
    } else {
#ifdef DEBUG
        printf("Unable to convert Python Object to XPath");
#endif
    }
    return (ret);
}

PyObject *
libxml_xmlValidCtxtPtrWrap(xmlValidCtxtPtr valid)
{
	PyObject *ret;

#ifdef DEBUG
	printf("libxml_xmlValidCtxtPtrWrap: valid = %p\n", valid);
#endif
	if (valid == NULL) {
		Py_INCREF(Py_None);
		return (Py_None);
	}

	ret =
		PyCapsule_New((void *) valid,
									 (char *) "xmlValidCtxtPtr", NULL);

	return (ret);
}

PyObject *
libxml_xmlCatalogPtrWrap(xmlCatalogPtr catal)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlNodePtrWrap: catal = %p\n", catal);
#endif
    if (catal == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) catal,
                                     (char *) "xmlCatalogPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlOutputBufferPtrWrap(xmlOutputBufferPtr buffer)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlOutputBufferPtrWrap: buffer = %p\n", buffer);
#endif
    if (buffer == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) buffer,
                                     (char *) "xmlOutputBufferPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlParserInputBufferPtrWrap(xmlParserInputBufferPtr buffer)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlParserInputBufferPtrWrap: buffer = %p\n", buffer);
#endif
    if (buffer == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) buffer,
                                     (char *) "xmlParserInputBufferPtr", NULL);
    return (ret);
}

#ifdef LIBXML_REGEXP_ENABLED
PyObject *
libxml_xmlRegexpPtrWrap(xmlRegexpPtr regexp)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlRegexpPtrWrap: regexp = %p\n", regexp);
#endif
    if (regexp == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) regexp,
                                     (char *) "xmlRegexpPtr", NULL);
    return (ret);
}
#endif /* LIBXML_REGEXP_ENABLED */

#ifdef LIBXML_READER_ENABLED
PyObject *
libxml_xmlTextReaderPtrWrap(xmlTextReaderPtr reader)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlTextReaderPtrWrap: reader = %p\n", reader);
#endif
    if (reader == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) reader,
                                     (char *) "xmlTextReaderPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlTextReaderLocatorPtrWrap(xmlTextReaderLocatorPtr locator)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlTextReaderLocatorPtrWrap: locator = %p\n", locator);
#endif
    if (locator == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) locator,
                                     (char *) "xmlTextReaderLocatorPtr", NULL);
    return (ret);
}
#endif /* LIBXML_READER_ENABLED */

#ifdef LIBXML_SCHEMAS_ENABLED
PyObject *
libxml_xmlRelaxNGPtrWrap(xmlRelaxNGPtr ctxt)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlRelaxNGPtrWrap: ctxt = %p\n", ctxt);
#endif
    if (ctxt == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) ctxt,
                                     (char *) "xmlRelaxNGPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlRelaxNGParserCtxtPtrWrap(xmlRelaxNGParserCtxtPtr ctxt)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlRelaxNGParserCtxtPtrWrap: ctxt = %p\n", ctxt);
#endif
    if (ctxt == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) ctxt,
                                     (char *) "xmlRelaxNGParserCtxtPtr", NULL);
    return (ret);
}
PyObject *
libxml_xmlRelaxNGValidCtxtPtrWrap(xmlRelaxNGValidCtxtPtr valid)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlRelaxNGValidCtxtPtrWrap: valid = %p\n", valid);
#endif
    if (valid == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret =
        PyCapsule_New((void *) valid,
                                     (char *) "xmlRelaxNGValidCtxtPtr", NULL);
    return (ret);
}

PyObject *
libxml_xmlSchemaPtrWrap(xmlSchemaPtr ctxt)
{
	PyObject *ret;

#ifdef DEBUG
	printf("libxml_xmlSchemaPtrWrap: ctxt = %p\n", ctxt);
#endif
	if (ctxt == NULL) {
		Py_INCREF(Py_None);
		return (Py_None);
	}
	ret =
		PyCapsule_New((void *) ctxt,
									 (char *) "xmlSchemaPtr", NULL);
	return (ret);
}

PyObject *
libxml_xmlSchemaParserCtxtPtrWrap(xmlSchemaParserCtxtPtr ctxt)
{
	PyObject *ret;

#ifdef DEBUG
	printf("libxml_xmlSchemaParserCtxtPtrWrap: ctxt = %p\n", ctxt);
#endif
	if (ctxt == NULL) {
		Py_INCREF(Py_None);
		return (Py_None);
	}
	ret =
		PyCapsule_New((void *) ctxt,
									 (char *) "xmlSchemaParserCtxtPtr", NULL);

	return (ret);
}

PyObject *
libxml_xmlSchemaValidCtxtPtrWrap(xmlSchemaValidCtxtPtr valid)
{
	PyObject *ret;

#ifdef DEBUG
	printf("libxml_xmlSchemaValidCtxtPtrWrap: valid = %p\n", valid);
#endif
	if (valid == NULL) {
		Py_INCREF(Py_None);
		return (Py_None);
	}

	ret =
		PyCapsule_New((void *) valid,
									 (char *) "xmlSchemaValidCtxtPtr", NULL);

	return (ret);
}
#endif /* LIBXML_SCHEMAS_ENABLED */

PyObject *
libxml_xmlErrorPtrWrap(xmlErrorPtr error)
{
    PyObject *ret;

#ifdef DEBUG
    printf("libxml_xmlErrorPtrWrap: error = %p\n", error);
#endif
    if (error == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    ret = PyCapsule_New((void *) error, (char *) "xmlErrorPtr", NULL);
    return (ret);
}
