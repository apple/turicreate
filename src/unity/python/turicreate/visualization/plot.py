from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import logging as _logging

import json as _json

class Plot(object):

    def __init__(self, _proxy=None):
        if (_proxy):
            self.__proxy__ = _proxy
        else:
            self.__proxy__ = None

    def show(self):
        import sys
        import os
        if sys.platform != 'darwin' and sys.platform != 'linux2':
             raise NotImplementedError('Visualization is currently supported only on macOS and Linux.')

        self.__proxy__.get('call_function', {'__function_name__': 'show'})

    def save_vega(self, filepath, include_data=True):
        file_contents = self.__proxy__.get('call_function', {'__function_name__': 'get_spec'})

        with open(filepath, 'w') as fp:
            _json.dump(file_contents, fp)

    def get_data(self):
        return _json.loads(self.__proxy__.get('call_function', {'__function_name__': 'get_data'}))

    def get_vega(self, include_data=True):
        if(include_data):
            spec = _json.loads(self.__proxy__.get('call_function', {'__function_name__': 'get_spec'}))
            data = _json.loads(self.__proxy__.get('call_function', {'__function_name__': 'get_data'}))["data_spec"]
            for x in range(0, len(spec["vega_spec"]["data"])):
                if(spec["vega_spec"]["data"][x]["name"] == "source_2"):
                    spec["vega_spec"]["data"][x] = data
                    break;
            return spec
        else:
            return _json.loads(self.__proxy__.get('call_function', {'__function_name__': 'get_spec'}))

    def _repr_javascript_(self):
        from IPython.core.display import display, HTML

        vega_spec = self.get_vega(True)["vega_spec"]

        vega_html = '<html lang="en"><head><script src="https://cdnjs.cloudflare.com/ajax/libs/vega/3.0.8/vega.js"></script><script src="https://cdnjs.cloudflare.com/ajax/libs/vega-embed/3.0.0-rc7/vega-embed.js"></script></head><body><div id="vis"></div><script>var vega_json = '+_json.dumps(_json.dumps(vega_spec)).replace("'", "&apos;")+'; var vega_json_parsed = JSON.parse(vega_json); vegaEmbed("#vis", vega_json_parsed);</script></body></html>'
        display(HTML('<html><body><iframe style="border:0;margin:0" width="'+str(vega_spec["width"]+200)+'" height="'+str(vega_spec["height"]+200)+'" srcdoc='+"'"+vega_html+"'"+' src="demo_iframe_srcdoc.htm"><p>Your browser does not support iframes.</p></iframe></body></html>'))
