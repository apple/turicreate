from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import logging as _logging
import json as _json
import os as _os
from tempfile import mkstemp as _mkstemp
from subprocess import Popen as _Popen 
from subprocess import PIPE as _PIPE

_target = 'auto'

_NODE_NOT_FOUND_ERROR_CODE = 127

def _run_cmdline(command):
    # runs a shell command
    p = _Popen(args=command, stdout=_PIPE, stderr=_PIPE, shell=True)
    stdout_feed, stderr_feed = p.communicate() # wait for completion
    exit_code = p.poll()
    return (exit_code, stdout_feed, stderr_feed)

def set_target(target='auto'):
    """
    Sets the target for visualizations launched with the `show` method. If
    unset, or if target is not provided, defaults to 'auto'.

    Notes
    -----
    - When in 'auto' target, `show` will display plot output inline when in
      Jupyter Notebook, and otherwise will open a native GUI window on macOS
      and Linux.

    Parameters
    ----------
    target : str
        The target for rendering visualizations launched with `show` methods.
        Possible values are:
        * 'auto': display plot output inline when in Jupyter Notebook, and
          otherwise launch a native GUI window.
        * 'gui': always launch a native GUI window.
    """
    global _target
    if target not in ['auto', 'gui']:
        raise ValueError("Expected target to be one of: 'auto', 'gui'.")
    _target = target


class Plot(object):
    """
    An immutable object representation of a visualization.

    Notes
    -----
    - A plot object is returned via the SFrame's or SArray's .plot() method

    Examples
    --------
    Suppose 'plt' is an Plot Object

    We can view it using:

    >>> plt.show()

    We can save it using:

    >>> plt.save('vega_spec.json')

    We can also save the vega representation of the plot without data:

    >>> plt.save('vega_spec.json', False)

    """
    def __init__(self, _proxy=None):
        if (_proxy):
            self.__proxy__ = _proxy
        else:
            self.__proxy__ = None

    def show(self):
        """
        A method for displaying the Plot object

        Notes
        -----
        - The plot will render either inline in a Jupyter Notebook, or in a
          native GUI window, depending on the value provided in
          `turicreate.visualization.set_target` (defaults to 'auto').

        Examples
        --------
        Suppose 'plt' is an Plot Object

        We can view it using:

        >>> plt.show()

        """
        global _target
        display = False
        try:
            if _target == 'auto' and \
               get_ipython().__class__.__name__ == "ZMQInteractiveShell":
                self._repr_javascript_()
                display = True
        except NameError:
            pass
        finally:
            if not display:
                import sys
                if sys.platform != 'darwin' and sys.platform != 'linux2' and sys.platform != 'linux':
                     raise NotImplementedError('Visualization is currently supported only on macOS and Linux.')

                self.__proxy__.get('call_function', {'__function_name__': 'show'})

    def save(self, filepath, include_data=True):
        """
        A method for saving the Plot object in a vega representation

        Parameters
        ----------
        include_data : bool, optional
            If True, save's the Plot in a vega spec with the data spec
            included.

        Notes
        -----
        - The save method saves the Plot object in a vega json format

        Examples
        --------
        Suppose 'plt' is an Plot Object

        We can save it using:

        >>> plt.save('vega_spec.json')

        We can also save the vega representation of the plot without data:

        >>> plt.save('vega_spec.json', False)

        """
        if type(filepath) != str:
            raise ValueError("filepath provided is not a string")

        if filepath.endswith(".json"):
            # save as vega json
            spec = self._get_vega(include_data = include_data)
            with open(filepath, 'w') as fp:
                _json.dump(spec, fp)
        elif filepath.endswith(".png") or filepath.endswith(".svg"):
            # save as png/svg, but json first
            spec = self._get_vega(include_data = True)
            EXTENSION_START_INDEX = -3
            extension = filepath[EXTENSION_START_INDEX:]
            temp_file_tuple = _mkstemp()
            temp_file_path = temp_file_tuple[1]
            with open(temp_file_path, 'w') as fp:
                _json.dump(spec, fp)
            dirname = _os.path.dirname(__file__)
            relative_path_to_vg2png_vg2svg = "../vg2" + extension
            absolute_path_to_vg2png_vg2svg = _os.path.join(dirname, 
                relative_path_to_vg2png_vg2svg)
            (exitcode, stdout, stderr) = _run_cmdline("node " + 
                absolute_path_to_vg2png_vg2svg + " " 
                + temp_file_path + " " + filepath)
            if exitcode == _NODE_NOT_FOUND_ERROR_CODE:
                # user doesn't have node installed
                raise RuntimeError("Node.js not found. Saving as PNG and SVG" +
                    " requires Node.js, please download and install Node.js " +
                    "from here and try again: https://nodejs.org/en/download/")
            elif exitcode == 0:
                # success
                pass
            else:
                # something else
                raise RuntimeError(stderr)
            # delete temp file that user didn't ask for
            _run_cmdline("rm " + temp_file_path) 
        else:
            raise NotImplementedError("filename must end in" +
                " .json, .svg, or .png")

    def _get_data(self):
        return _json.loads(self.__proxy__.get('call_function', {'__function_name__': 'get_data'}))

    def _get_vega(self, include_data=True):
        if(include_data):
            spec = _json.loads(self.__proxy__.get('call_function', {'__function_name__': 'get_spec'}))["vega_spec"]
            data = _json.loads(self.__proxy__.get('call_function', {'__function_name__': 'get_data'}))["data_spec"]
            for x in range(len(spec["data"])):
                if(spec["data"][x]["name"] == "source_2"):
                    spec["data"][x] = data
                    break;
            return spec
        else:
            return _json.loads(self.__proxy__.get('call_function', {'__function_name__': 'get_spec'}))["vega_spec"]

    def _repr_javascript_(self):
        from IPython.core.display import display, HTML

        vega_spec = self._get_vega(True)["vega_spec"]

        vega_html = '<html lang="en"> \
                        <head> \
                            <script src="https://cdnjs.cloudflare.com/ajax/libs/vega/3.0.8/vega.js"></script> \
                            <script src="https://cdnjs.cloudflare.com/ajax/libs/vega-embed/3.0.0-rc7/vega-embed.js"></script> \
                            <script src="https://cdnjs.cloudflare.com/ajax/libs/vega-tooltip/0.5.1/vega-tooltip.min.js"></script> \
                            <link rel="stylesheet" type="text/css" href="https://cdnjs.cloudflare.com/ajax/libs/vega-tooltip/0.5.1/vega-tooltip.min.css"> \
                            <style> \
                            .vega-actions > a{ \
                                color:white; \
                                text-decoration: none; \
                                font-family: "Arial"; \
                                cursor:pointer; \
                                padding:5px; \
                                background:#AAAAAA; \
                                border-radius:4px; \
                                padding-left:10px; \
                                padding-right:10px; \
                                margin-right:5px; \
                            } \
                            .vega-actions{ \
                                margin-top:20px; \
                                text-align:center \
                            }\
                            .vega-actions > a{ \
                                background:#999999;\
                            } \
                            </style> \
                        </head> \
                        <body> \
                            <div id="vis"> \
                            </div> \
                            <script> \
                                var vega_json = '+_json.dumps(_json.dumps(vega_spec)).replace("'", "&apos;")+'; \
                                var vega_json_parsed = JSON.parse(vega_json); \
                                var toolTipOpts = { \
                                    showAllFields: true \
                                }; \
                                if(vega_json_parsed["metadata"] != null){ \
                                    if(vega_json_parsed["metadata"]["bubbleOpts"] != null){ \
                                        toolTipOpts = vega_json_parsed["metadata"]["bubbleOpts"]; \
                                    }; \
                                }; \
                                vegaEmbed("#vis", vega_json_parsed).then(function (result) { \
                                    vegaTooltip.vega(result.view, toolTipOpts);  \
                                }); \
                            </script> \
                        </body> \
                    </html>'

        display(HTML('<html> \
                <body> \
                    <iframe style="border:0;margin:0" width="'+str((vega_spec["width"] if "width" in vega_spec else 600)+200)+'" height="'+str(vega_spec["height"]+220)+'" srcdoc='+"'"+vega_html+"'"+' src="demo_iframe_srcdoc.htm"> \
                        <p>Your browser does not support iframes.</p> \
                    </iframe> \
                </body> \
            </html>'));
