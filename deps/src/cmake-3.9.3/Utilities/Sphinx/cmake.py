# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

import os
import re

# Monkey patch for pygments reporting an error when generator expressions are
# used.
# https://bitbucket.org/birkenfeld/pygments-main/issue/942/cmake-generator-expressions-not-handled
from pygments.lexers import CMakeLexer
from pygments.token import Name, Operator
from pygments.lexer import bygroups
CMakeLexer.tokens["args"].append(('(\\$<)(.+?)(>)',
                                  bygroups(Operator, Name.Variable, Operator)))

# Monkey patch for sphinx generating invalid content for qcollectiongenerator
# https://bitbucket.org/birkenfeld/sphinx/issue/1435/qthelp-builder-should-htmlescape-keywords
from sphinx.util.pycompat import htmlescape
from sphinx.builders.qthelp import QtHelpBuilder
old_build_keywords = QtHelpBuilder.build_keywords
def new_build_keywords(self, title, refs, subitems):
  old_items = old_build_keywords(self, title, refs, subitems)
  new_items = []
  for item in old_items:
    before, rest = item.split("ref=\"", 1)
    ref, after = rest.split("\"")
    if ("<" in ref and ">" in ref):
      new_items.append(before + "ref=\"" + htmlescape(ref) + "\"" + after)
    else:
      new_items.append(item)
  return new_items
QtHelpBuilder.build_keywords = new_build_keywords


from docutils.parsers.rst import Directive, directives
from docutils.transforms import Transform
try:
    from docutils.utils.error_reporting import SafeString, ErrorString
except ImportError:
    # error_reporting was not in utils before version 0.11:
    from docutils.error_reporting import SafeString, ErrorString

from docutils import io, nodes

from sphinx.directives import ObjectDescription
from sphinx.domains import Domain, ObjType
from sphinx.roles import XRefRole
from sphinx.util.nodes import make_refnode
from sphinx import addnodes

# Needed for checking if Sphinx version is >= 1.4.
# See https://github.com/sphinx-doc/sphinx/issues/2673
old_sphinx = False

try:
    from sphinx import version_info
    if version_info < (1, 4):
        old_sphinx = True
except ImportError:
    # The `sphinx.version_info` tuple was added in Sphinx v1.2:
    old_sphinx = True


class CMakeModule(Directive):
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {'encoding': directives.encoding}

    def __init__(self, *args, **keys):
        self.re_start = re.compile(r'^#\[(?P<eq>=*)\[\.rst:$')
        Directive.__init__(self, *args, **keys)

    def run(self):
        settings = self.state.document.settings
        if not settings.file_insertion_enabled:
            raise self.warning('"%s" directive disabled.' % self.name)

        env = self.state.document.settings.env
        rel_path, path = env.relfn2path(self.arguments[0])
        path = os.path.normpath(path)
        encoding = self.options.get('encoding', settings.input_encoding)
        e_handler = settings.input_encoding_error_handler
        try:
            settings.record_dependencies.add(path)
            f = io.FileInput(source_path=path, encoding=encoding,
                             error_handler=e_handler)
        except UnicodeEncodeError as error:
            raise self.severe('Problems with "%s" directive path:\n'
                              'Cannot encode input file path "%s" '
                              '(wrong locale?).' %
                              (self.name, SafeString(path)))
        except IOError as error:
            raise self.severe('Problems with "%s" directive path:\n%s.' %
                      (self.name, ErrorString(error)))
        raw_lines = f.read().splitlines()
        f.close()
        rst = None
        lines = []
        for line in raw_lines:
            if rst is not None and rst != '#':
                # Bracket mode: check for end bracket
                pos = line.find(rst)
                if pos >= 0:
                    if line[0] == '#':
                        line = ''
                    else:
                        line = line[0:pos]
                    rst = None
            else:
                # Line mode: check for .rst start (bracket or line)
                m = self.re_start.match(line)
                if m:
                    rst = ']%s]' % m.group('eq')
                    line = ''
                elif line == '#.rst:':
                    rst = '#'
                    line = ''
                elif rst == '#':
                    if line == '#' or line[:2] == '# ':
                        line = line[2:]
                    else:
                        rst = None
                        line = ''
                elif rst is None:
                    line = ''
            lines.append(line)
        if rst is not None and rst != '#':
            raise self.warning('"%s" found unclosed bracket "#[%s[.rst:" in %s' %
                               (self.name, rst[1:-1], path))
        self.state_machine.insert_input(lines, path)
        return []

class _cmake_index_entry:
    def __init__(self, desc):
        self.desc = desc

    def __call__(self, title, targetid, main = 'main'):
        # See https://github.com/sphinx-doc/sphinx/issues/2673
        if old_sphinx:
            return ('pair', u'%s ; %s' % (self.desc, title), targetid, main)
        else:
            return ('pair', u'%s ; %s' % (self.desc, title), targetid, main, None)

_cmake_index_objs = {
    'command':    _cmake_index_entry('command'),
    'generator':  _cmake_index_entry('generator'),
    'manual':     _cmake_index_entry('manual'),
    'module':     _cmake_index_entry('module'),
    'policy':     _cmake_index_entry('policy'),
    'prop_cache': _cmake_index_entry('cache property'),
    'prop_dir':   _cmake_index_entry('directory property'),
    'prop_gbl':   _cmake_index_entry('global property'),
    'prop_inst':  _cmake_index_entry('installed file property'),
    'prop_sf':    _cmake_index_entry('source file property'),
    'prop_test':  _cmake_index_entry('test property'),
    'prop_tgt':   _cmake_index_entry('target property'),
    'variable':   _cmake_index_entry('variable'),
    }

def _cmake_object_inventory(env, document, line, objtype, targetid):
    inv = env.domaindata['cmake']['objects']
    if targetid in inv:
        document.reporter.warning(
            'CMake object "%s" also described in "%s".' %
            (targetid, env.doc2path(inv[targetid][0])), line=line)
    inv[targetid] = (env.docname, objtype)

class CMakeTransform(Transform):

    # Run this transform early since we insert nodes we want
    # treated as if they were written in the documents.
    default_priority = 210

    def __init__(self, document, startnode):
        Transform.__init__(self, document, startnode)
        self.titles = {}

    def parse_title(self, docname):
        """Parse a document title as the first line starting in [A-Za-z0-9<]
           or fall back to the document basename if no such line exists.
           The cmake --help-*-list commands also depend on this convention.
           Return the title or False if the document file does not exist.
        """
        env = self.document.settings.env
        title = self.titles.get(docname)
        if title is None:
            fname = os.path.join(env.srcdir, docname+'.rst')
            try:
                f = open(fname, 'r')
            except IOError:
                title = False
            else:
                for line in f:
                    if len(line) > 0 and (line[0].isalnum() or line[0] == '<'):
                        title = line.rstrip()
                        break
                f.close()
                if title is None:
                    title = os.path.basename(docname)
            self.titles[docname] = title
        return title

    def apply(self):
        env = self.document.settings.env

        # Treat some documents as cmake domain objects.
        objtype, sep, tail = env.docname.rpartition('/')
        make_index_entry = _cmake_index_objs.get(objtype)
        if make_index_entry:
            title = self.parse_title(env.docname)
            # Insert the object link target.
            if objtype == 'command':
                targetname = title.lower()
            else:
                targetname = title
            targetid = '%s:%s' % (objtype, targetname)
            targetnode = nodes.target('', '', ids=[targetid])
            self.document.note_explicit_target(targetnode)
            self.document.insert(0, targetnode)
            # Insert the object index entry.
            indexnode = addnodes.index()
            indexnode['entries'] = [make_index_entry(title, targetid)]
            self.document.insert(0, indexnode)
            # Add to cmake domain object inventory
            _cmake_object_inventory(env, self.document, 1, objtype, targetid)

class CMakeObject(ObjectDescription):

    def handle_signature(self, sig, signode):
        # called from sphinx.directives.ObjectDescription.run()
        signode += addnodes.desc_name(sig, sig)
        return sig

    def add_target_and_index(self, name, sig, signode):
        if self.objtype == 'command':
           targetname = name.lower()
        else:
           targetname = name
        targetid = '%s:%s' % (self.objtype, targetname)
        if targetid not in self.state.document.ids:
            signode['names'].append(targetid)
            signode['ids'].append(targetid)
            signode['first'] = (not self.names)
            self.state.document.note_explicit_target(signode)
            _cmake_object_inventory(self.env, self.state.document,
                                    self.lineno, self.objtype, targetid)

        make_index_entry = _cmake_index_objs.get(self.objtype)
        if make_index_entry:
            self.indexnode['entries'].append(make_index_entry(name, targetid))

class CMakeXRefRole(XRefRole):

    # See sphinx.util.nodes.explicit_title_re; \x00 escapes '<'.
    _re = re.compile(r'^(.+?)(\s*)(?<!\x00)<(.*?)>$', re.DOTALL)
    _re_sub = re.compile(r'^([^()\s]+)\s*\(([^()]*)\)$', re.DOTALL)

    def __call__(self, typ, rawtext, text, *args, **keys):
        # Translate CMake command cross-references of the form:
        #  `command_name(SUB_COMMAND)`
        # to have an explicit target:
        #  `command_name(SUB_COMMAND) <command_name>`
        if typ == 'cmake:command':
            m = CMakeXRefRole._re_sub.match(text)
            if m:
                text = '%s <%s>' % (text, m.group(1))
        # CMake cross-reference targets frequently contain '<' so escape
        # any explicit `<target>` with '<' not preceded by whitespace.
        while True:
            m = CMakeXRefRole._re.match(text)
            if m and len(m.group(2)) == 0:
                text = '%s\x00<%s>' % (m.group(1), m.group(3))
            else:
                break
        return XRefRole.__call__(self, typ, rawtext, text, *args, **keys)

    # We cannot insert index nodes using the result_nodes method
    # because CMakeXRefRole is processed before substitution_reference
    # nodes are evaluated so target nodes (with 'ids' fields) would be
    # duplicated in each evaluted substitution replacement.  The
    # docutils substitution transform does not allow this.  Instead we
    # use our own CMakeXRefTransform below to add index entries after
    # substitutions are completed.
    #
    # def result_nodes(self, document, env, node, is_ref):
    #     pass

class CMakeXRefTransform(Transform):

    # Run this transform early since we insert nodes we want
    # treated as if they were written in the documents, but
    # after the sphinx (210) and docutils (220) substitutions.
    default_priority = 221

    def apply(self):
        env = self.document.settings.env

        # Find CMake cross-reference nodes and add index and target
        # nodes for them.
        for ref in self.document.traverse(addnodes.pending_xref):
            if not ref['refdomain'] == 'cmake':
                continue

            objtype = ref['reftype']
            make_index_entry = _cmake_index_objs.get(objtype)
            if not make_index_entry:
                continue

            objname = ref['reftarget']
            targetnum = env.new_serialno('index-%s:%s' % (objtype, objname))

            targetid = 'index-%s-%s:%s' % (targetnum, objtype, objname)
            targetnode = nodes.target('', '', ids=[targetid])
            self.document.note_explicit_target(targetnode)

            indexnode = addnodes.index()
            indexnode['entries'] = [make_index_entry(objname, targetid, '')]
            ref.replace_self([indexnode, targetnode, ref])

class CMakeDomain(Domain):
    """CMake domain."""
    name = 'cmake'
    label = 'CMake'
    object_types = {
        'command':    ObjType('command',    'command'),
        'generator':  ObjType('generator',  'generator'),
        'variable':   ObjType('variable',   'variable'),
        'module':     ObjType('module',     'module'),
        'policy':     ObjType('policy',     'policy'),
        'prop_cache': ObjType('prop_cache', 'prop_cache'),
        'prop_dir':   ObjType('prop_dir',   'prop_dir'),
        'prop_gbl':   ObjType('prop_gbl',   'prop_gbl'),
        'prop_inst':  ObjType('prop_inst',  'prop_inst'),
        'prop_sf':    ObjType('prop_sf',    'prop_sf'),
        'prop_test':  ObjType('prop_test',  'prop_test'),
        'prop_tgt':   ObjType('prop_tgt',   'prop_tgt'),
        'manual':     ObjType('manual',     'manual'),
    }
    directives = {
        'command':    CMakeObject,
        'variable':   CMakeObject,
        # Other object types cannot be created except by the CMakeTransform
        # 'generator':  CMakeObject,
        # 'module':     CMakeObject,
        # 'policy':     CMakeObject,
        # 'prop_cache': CMakeObject,
        # 'prop_dir':   CMakeObject,
        # 'prop_gbl':   CMakeObject,
        # 'prop_inst':  CMakeObject,
        # 'prop_sf':    CMakeObject,
        # 'prop_test':  CMakeObject,
        # 'prop_tgt':   CMakeObject,
        # 'manual':     CMakeObject,
    }
    roles = {
        'command':    CMakeXRefRole(fix_parens = True, lowercase = True),
        'generator':  CMakeXRefRole(),
        'variable':   CMakeXRefRole(),
        'module':     CMakeXRefRole(),
        'policy':     CMakeXRefRole(),
        'prop_cache': CMakeXRefRole(),
        'prop_dir':   CMakeXRefRole(),
        'prop_gbl':   CMakeXRefRole(),
        'prop_inst':  CMakeXRefRole(),
        'prop_sf':    CMakeXRefRole(),
        'prop_test':  CMakeXRefRole(),
        'prop_tgt':   CMakeXRefRole(),
        'manual':     CMakeXRefRole(),
    }
    initial_data = {
        'objects': {},  # fullname -> docname, objtype
    }

    def clear_doc(self, docname):
        to_clear = set()
        for fullname, (fn, _) in self.data['objects'].items():
            if fn == docname:
                to_clear.add(fullname)
        for fullname in to_clear:
            del self.data['objects'][fullname]

    def resolve_xref(self, env, fromdocname, builder,
                     typ, target, node, contnode):
        targetid = '%s:%s' % (typ, target)
        obj = self.data['objects'].get(targetid)
        if obj is None:
            # TODO: warn somehow?
            return None
        return make_refnode(builder, fromdocname, obj[0], targetid,
                            contnode, target)

    def get_objects(self):
        for refname, (docname, type) in self.data['objects'].items():
            yield (refname, refname, type, docname, refname, 1)

def setup(app):
    app.add_directive('cmake-module', CMakeModule)
    app.add_transform(CMakeTransform)
    app.add_transform(CMakeXRefTransform)
    app.add_domain(CMakeDomain)
