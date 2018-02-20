import time
import json

class Plot(object):
    def __init__(self):
        self.plot_object = tc.extensions._Plot()

    def show(self):
        self.plot_object.show()

    def save_vega(self, filepath, include_data=True):
        file_contents = self.plot_object.get_vega(include_data)

        with open(filepath, 'w') as fp:
            json.dump(file_contents, fp)

    def get_data(self, filepath, include_data=True):
        return self.plot_object.get_data()

    def get_vega(self, include_data=True):
        return self.plot_object.get_vega(include_data)

    def _repr_javascript_(self):
        from IPython.core.display import display, HTML, clear_output, update_display

        vega_spec = self.get_vega()["vega_spec"]

        vega_html = '<html lang="en"><head><script src="https://cdnjs.cloudflare.com/ajax/libs/vega/3.0.8/vega.js"></script><script src="https://cdnjs.cloudflare.com/ajax/libs/vega-embed/3.0.0-rc7/vega-embed.js"></script></head><body><div id="vis"></div><script>var vega_json = '+json.dumps(json.dumps(vega_spec)).replace("'", "&apos;")+'; var vega_json_parsed = JSON.parse(vega_json); vegaEmbed("#vis", vega_json_parsed);</script></body></html>'
        display(HTML('<html><body><iframe style="border:0;margin:0" width="'+str(width)+'" height="'+str(height)+'" srcdoc='+"'"+vega_html+"'"+' src="demo_iframe_srcdoc.htm"><p>Your browser does not support iframes.</p></iframe></body></html>'));
