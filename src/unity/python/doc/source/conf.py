# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#
# Turi Create documentation build configuration file, created by
# sphinx-quickstart on Mon Feb 24 17:26:36 2014.
#
# This file is execfile()d with the current directory set to its containing dir.
#
# Note that not all possible configuration values are present in this
# autogenerated file.
#
# All configuration values have a default; values that are commented out
# serve to show the default.

import sys, os

# Import turicreate and several submodules so that sphinx can find them.
# For example, it can now find turicreate.recommender.PopularityModel.
import turicreate
for m in [
          'activity_classifier',
          'boosted_trees_classifier',
          'boosted_trees_regression',
          'classifier',
          'connected_components',
          'dbscan',
          'degree_counting',
          'distances',
          'drawing_classifier',
          'evaluation',
          'extensions',
          'graph_coloring',
          'image_analysis',
          'kcore',
          'kmeans',
          'label_propagation',
          'linear_regression',
          'logistic_classifier',
          'load_sarray',
          'load_sframe',
          'load_sgraph',
          'load_model',
          'nearest_neighbors',
          'nearest_neighbor_classifier',
          'pagerank',
          'recommender',
          'random_forest_classifier',
          'random_forest_regression',
          'decision_tree_classifier',
          'decision_tree_regression',
          'regression',
          'shortest_path',
          'text_classifier',
          'sound_classifier',
          'svm_classifier',
          'style_transfer',
          'text_analytics',
          'object_detector',
          'image_classifier',
          'image_similarity',
          'topic_model',
          'triangle_counting',
          'visualization'
          ]:
    module_name = 'turicreate.' + m
    sys.modules[module_name] = eval(module_name)

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
sys.path.insert(0, os.path.abspath('.'))
print 'CURRENT PATH', sys.path
#sys.path.insert(0, os.path.abspath('../venv/lib/python2.7/site-packages/'))

# -- General configuration -----------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be extensions
# coming with Sphinx (named 'sphinx.ext.*') or your custom ones.
extensions = ['sphinx.ext.autodoc', 'numpydoc', 'sphinx.ext.coverage', 'sphinx.ext.mathjax',
              'sphinx.ext.inheritance_diagram', 'sphinx.ext.autosummary', 'sphinx_turicreate_ext.autorun']

autosummary_generate = True

# Math
mathjax_path = "https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.1/MathJax.js?config=TeX-AMS-MML_HTMLorMML"

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix of source filenames.
source_suffix = '.rst'

# The encoding of source files.
#source_encoding = 'utf-8-sig'

# The master toctree document.
master_doc = 'index' #'turicreate'

# General information about the project.
project = u'Turi Create API'
copyright = u'2017 Apple Inc.'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
version = turicreate.version_info.version
release = version

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#language = None

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
#today = ''
# Else, today_fmt is used as the format for a strftime call.
#today_fmt = '%B %d, %Y'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = ['*demo*', '*test*', 'test_*', '*cython*']
numpydoc_show_class_members = False

# The reST default role (used for this markup: `text`) to use for all documents.
#default_role = None

# If true, '()' will be appended to :func: etc. cross-reference text.
#add_function_parentheses = True

# If true, the current module name will be prepended to all description
# unit titles (such as .. function::).
#add_module_names = True

# If true, sectionauthor and moduleauthor directives will be shown in the
# output. They are ignored by default.
#show_authors = False

# The name of the Pygments (syntax highlighting) style to use.
#pygments_style = 'sphinx'

# A list of ignored prefixes for module index sorting.
#modindex_common_prefix = []

# -- Customizations -------------------

autodoc_default_flags = [] #'members']
                         #'private-members',
                         #'special-members',
                         #'show-inheritance']

def autodoc_skip_member(app, what, name, obj, skip, options):
    exclusions = ('__weakref__',  # special-members
                  '__doc__', '__module__', '__dict__',  # undoc-members
                 )
    exclude = name in exclusions
    return skip or exclude

def setup(app):
    app.connect('autodoc-skip-member', autodoc_skip_member)

# -- Options for HTML output ---------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = "sphinx_rtd_theme"
html_theme_path = ["_themes", ]

html_theme_options = {
    #'theme_globaltoc_depth': 3
}

html_context = {
  # The same as the default but without jquery (it is already loaded)
  # https://github.com/sphinx-doc/sphinx/blob/795190950648277a39cbb13e51a2ae1bdd244cca/sphinx/builders/html.py#L91
  'script_files': ['_static/underscore.js', '_static/doctools.js']
}


# Remove search results entirely, since we can't get them to format right.
html_copy_source = False

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.

# Add any paths that contain custom themes here, relative to this directory.
#html_theme_path = []

# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
#html_title = None

# A shorter title for the navigation bar.  Default is the same as html_title.
html_short_title = "Turi Create"

org_url="https://github.com/apple/turicreate"

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
html_logo = '../images/tc_logo_white.png'

# The name of an image file (within the static path) to use as favicon of the
# docs.  This file should be a Windows icon file (.ico) being 16x16 or 32x32
# pixels large.
html_favicon = '../images/favicon.ico'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['../images']

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
html_last_updated_fmt = '%b %d, %Y'

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
#html_use_smartypants = True

# Custom sidebar templates, maps document names to template names.
#html_sidebars = {'**': ['sidebartoc.html']}

# Additional templates that should be rendered to pages, maps page names to
# template names.
#html_additional_pages = {}

# If false, no module index is generated.
#html_domain_indices = True

# If false, no index is generated.
#html_use_index = True

# If true, the index is split into individual pages for each letter.
#html_split_index = False

# If true, links to the reST sources are added to the pages.
html_show_sourcelink = False

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
html_show_copyright = False

# If true, an OpenSearch description file will be output, and all pages will
# contain a <link> tag referring to it.  The value of this option must be the
# base URL from which the finished HTML is served.
#html_use_opensearch = ''

# This is the file name suffix for HTML files (e.g. ".xhtml").
# html_file_suffix = '.html'

# Output file base name for HTML help builder.
htmlhelp_basename = 'TuriCreatedoc'

# -- Options for LaTeX output --------------------------------------------------

latex_elements = {
# The paper size ('letterpaper' or 'a4paper').
#'papersize': 'letterpaper',

# The font size ('10pt', '11pt' or '12pt').
#'pointsize': '10pt',

# Additional stuff for the LaTeX preamble.
#'preamble': '',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, documentclass [howto/manual]).
latex_documents = [
  ('index', 'TuriCreate.tex', u'Turi Create Documentation',
   u'Turi', 'manual'),
]

# The name of an image file (relative to this directory) to place at the top of
# the title page.
#latex_logo = None

# For "manual" documents, if this is true, then toplevel headings are parts,
# not chapters.
#latex_use_parts = False

# If true, show page references after internal links.
#latex_show_pagerefs = False

# If true, show URL addresses after external links.
#latex_show_urls = False

# Documents to append as an appendix to all manuals.
#latex_appendices = []

# If false, no module index is generated.
#latex_domain_indices = True


# -- Options for manual page output --------------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    ('index', 'turicreate', u'Turi Create Documentation',
     [u'Turi'], 1)
]

# If true, show URL addresses after external links.
#man_show_urls = False


# -- Options for Texinfo output ------------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
  ('index', 'TuriCreate', u'Turi Create Documentation',
   u'Turi', 'TuriCreate', 'One line description of project.',
   'Miscellaneous'),
]

# Documents to append as an appendix to all manuals.
#texinfo_appendices = []

# If false, no module index is generated.
#texinfo_domain_indices = True

# How to display URL addresses: 'footnote', 'no', or 'inline'.
#texinfo_show_urls = 'footnote'
