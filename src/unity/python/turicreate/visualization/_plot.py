from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import json as _json
import os as _os
import sys as _sys
from tempfile import mkstemp as _mkstemp
from subprocess import Popen as _Popen
from subprocess import PIPE as _PIPE

LABEL_DEFAULT = "__TURI_DEFAULT_LABEL"

_target = 'auto'

_SUCCESS = 0
_CANVAS_PREBUILT_NOT_FOUND_ERROR = 1
_NODE_NOT_FOUND_ERROR_CODE = 127
_PERMISSION_DENIED_ERROR_CODE = 243

def _get_client_app_path():
    (tcviz_dir, _) = _os.path.split(_os.path.dirname(__file__))

    if _sys.platform != 'darwin' and _sys.platform != 'linux2' and _sys.platform != 'linux' :
        raise NotImplementedError('Visualization is currently supported only on macOS and Linux.')
    
    if _sys.platform == 'darwin':
        return _os.path.join(tcviz_dir, 'Turi Create Visualization.app', 'Contents', 'MacOS', 'Turi Create Visualization')

    if _sys.platform == 'linux2' or _sys.platform == 'linux':
        return _os.path.join(tcviz_dir, 'Turi Create Visualization', 'visualization_client')


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
                if _sys.platform != 'darwin' and _sys.platform != 'linux2' and _sys.platform != 'linux':
                     raise NotImplementedError('Visualization is currently supported only on macOS and Linux.')

                path_to_client = _get_client_app_path()

                # TODO: allow autodetection of light/dark mode.
                # Disabled for now, since the GUI side needs some work (ie. background color).
                plot_variation = 0x10 # force light mode
                self.__proxy__.call_function('show', {'path_to_client': path_to_client, 'variation': plot_variation})

    def save(self, filepath):
        """
        A method for saving the Plot object in a vega representation

        Parameters
        ----------
        filepath: string
            The destination filepath where the plot object must be saved as.
            The extension of this filepath determines what format the plot will
            be saved as. Currently supported formats are JSON, PNG, and SVG.

        Examples
        --------
        Suppose 'plt' is an Plot Object

        We can save it using:

        >>> plt.save('vega_spec.json')

        We can also save the vega representation of the plot without data:

        >>> plt.save('vega_spec.json', False)

        We can save the plot as a PNG/SVG using:

        >>> plt.save('test.png')
        >>> plt.save('test.svg')

        """
        if type(filepath) != str:
            raise ValueError("filepath provided is not a string")

        if filepath.endswith(".json"):
            # save as vega json
            spec = self.get_vega(include_data = True)
            with open(filepath, 'w') as fp:
                _json.dump(spec, fp)
        elif filepath.endswith(".png") or filepath.endswith(".svg"):
            # save as png/svg, but json first
            spec = self.get_vega(include_data = True)
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
            # try node vg2[png|svg] json_filepath out_filepath
            (exitcode, stdout, stderr) = _run_cmdline("node " +
                absolute_path_to_vg2png_vg2svg + " "
                + temp_file_path + " " + filepath)

            if exitcode == _NODE_NOT_FOUND_ERROR_CODE:
                # user doesn't have node installed
                raise RuntimeError("Node.js not found. Saving as PNG and SVG" +
                    " requires Node.js, please download and install Node.js " +
                    "from here and try again: https://nodejs.org/en/download/")
            elif exitcode == _CANVAS_PREBUILT_NOT_FOUND_ERROR:
                # try to see if canvas-prebuilt is globally installed
                # if it is, then link it
                # if not, tell the user to install it
                (is_installed_exitcode, 
                    is_installed_stdout, 
                    is_installed_stderr) =  _run_cmdline(
                    "npm ls -g -json | grep canvas-prebuilt")
                if is_installed_exitcode == _SUCCESS:
                    # npm link canvas-prebuilt 
                    link_exitcode, link_stdout, link_stderr = _run_cmdline(
                        "npm link canvas-prebuilt")
                    if link_exitcode == _PERMISSION_DENIED_ERROR_CODE:
                        # They don't have permission, tell them.
                        raise RuntimeError(link_stderr + '\n\n' +
                            "`npm link canvas-prebuilt` failed, " +
                            "Permission Denied.")
                    elif link_exitcode == _SUCCESS:
                        # canvas-prebuilt link is now successful, so run the 
                        # node vg2[png|svg] json_filepath out_filepath
                        # command again.
                        (exitcode, stdout, stderr) = _run_cmdline("node " +
                            absolute_path_to_vg2png_vg2svg + " "
                            + temp_file_path + " " + filepath)
                        if exitcode != _SUCCESS:
                            # something else that we have not identified yet
                            # happened.
                            raise RuntimeError(stderr)
                    else:
                        raise RuntimeError(link_stderr)
                else:
                    raise RuntimeError("canvas-prebuilt not found. " +
                        "Saving as PNG and SVG requires canvas-prebuilt, " +
                        "please download and install canvas-prebuilt by " +
                        "running this command, and try again: " +
                        "`npm install -g canvas-prebuilt`")
            elif exitcode == _SUCCESS:
                pass
            else:
                raise RuntimeError(stderr)
            # delete temp file that user didn't ask for
            _run_cmdline("rm " + temp_file_path)
        else:
            raise NotImplementedError("filename must end in" +
                " .json, .svg, or .png")

    def get_data(self):
        return _json.loads(self.__proxy__.call_function('get_data'))

    def get_vega(self, include_data=True):
        # TODO: allow autodetection of light/dark mode.
        # Disabled for now, since the GUI side needs some work (ie. background color).
        plot_variation = 0x10 # force light mode
        return _json.loads(self.__proxy__.call_function('get_spec', {'include_data': include_data, 'variation': plot_variation}))

    def materialize(self):
        self.__proxy__.call_function('materialize')

    def _repr_javascript_(self):
        from IPython.core.display import display, HTML

        self.materialize()
        vega_spec = self.get_vega(True)

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
                                var vega_json = '+_json.dumps(_json.dumps(vega_spec)).replace("&", "&amp;").replace("'", "&apos;")+'; \
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
