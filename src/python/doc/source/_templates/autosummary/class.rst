:orphan:

..
  This template is borrowed from Pandas documentation:
  https://github.com/pydata/pandas/blob/master/doc/_templates/autosummary/class.rst

{% extends "!autosummary/class.rst" %}

{% block methods %}
{% if methods %}

.. rubric:: Methods

..
   HACK -- the point here is that we don't want this to appear in the output, but the autosummary should still generate the pages.
.. autosummary::
   :toctree:
  {% for item in all_methods %}
  {%- if not (item.startswith('_') or item in ['__call__'] or
    (name == 'SArray' and item in
    ['__add__', '__mul__', '__sub__', '__div__',
     '__radd__', '__rmul__', '__rsub__', '__rdiv__',
     '__and__', '__or__',
     '__gt__', '__lt__', '__eq__', '__ne__', '__ge__', '__le__',
     ]) or (name == 'SFrame' and item in
    [
    '__delitem__',
    '__format__',
    '__getitem__',
    '__init__',
    '__iter__',
    '__len__',
    '__new__',
    '__nonzero__',
    '__reduce__',
    '__reduce_ex__',
    '__repr__',
    '__setitem__',
    '__sizeof__',
    '__str__',
    '__subclasshook__',
     ]))
  %}
  {{ name }}.{{ item }}
  {%- endif -%}
  {%- endfor %}

{% endif %}
{% endblock %}

{% block attributes %}
{% if attributes %}

.. rubric:: Attributes

..
   HACK -- the point here is that we don't want this to appear in the output, but the autosummary should still generate the pages.
.. autosummary::
   :toctree:
   :nosignature
  {% for item in all_attributes %}
  {%- if not item.startswith('_') %}
  {{ name }}.{{ item }}
  {%- endif -%}
  {%- endfor %}

{% endif %}
{% endblock %}
