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

_target = "auto"

_SUCCESS = 0
_CANVAS_PREBUILT_NOT_FOUND_ERROR = 1
_NODE_NOT_FOUND_ERROR_CODE = 127
_PERMISSION_DENIED_ERROR_CODE = 243


def _get_client_app_path():
    (tcviz_dir, _) = _os.path.split(_os.path.dirname(__file__))

    if (
        _sys.platform != "darwin"
        and _sys.platform != "linux2"
        and _sys.platform != "linux"
    ):
        raise NotImplementedError(
            "Visualization is currently supported only on macOS and Linux."
        )

    if _sys.platform == "darwin":
        return _os.path.join(
            tcviz_dir,
            "Turi Create Visualization.app",
            "Contents",
            "MacOS",
            "Turi Create Visualization",
        )

    if _sys.platform == "linux2" or _sys.platform == "linux":
        return _os.path.join(
            tcviz_dir, "Turi Create Visualization", "visualization_client"
        )


def _ensure_web_server():
    import turicreate as tc

    if (
        tc.config.get_runtime_config()["TURI_VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY"]
        == ""
    ):
        path_to_client = _get_client_app_path()
        tc.config.set_runtime_config(
            "TURI_VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY",
            _os.path.abspath(
                _os.path.join(
                    _os.path.dirname(path_to_client), "..", "Resources", "build"
                )
            ),
        )


def _run_cmdline(command):
    # runs a shell command
    p = _Popen(args=command, stdout=_PIPE, stderr=_PIPE, shell=True)
    stdout_feed, stderr_feed = p.communicate()  # wait for completion
    exit_code = p.poll()
    return (exit_code, stdout_feed, stderr_feed)


def set_target(target="auto"):
    """
    Sets the target for visualizations launched with the `show`
    method. If unset, or if target is not provided, defaults to 'auto'.

    Notes
    -----
    - When in 'auto' target, `show` and `explore` will display plot output
      inline when in Jupyter Notebook, and otherwise will open a native GUI
      window.
    - Only `show` and `explore` can render inline in Jupyter Notebook
      or a browser.
      `annotate` will ignore this setting and open a GUI window
      unless target is set to `None`.

    Parameters
    ----------
    target : str
        The target for rendering visualizations launched with `show` methods.
        Possible values are:
        * 'auto': display plot output inline when in Jupyter Notebook, and
          otherwise launch a native GUI window.
        * 'browser': opens a web browser pointing to http://localhost.
        * 'gui': always launch a native GUI window.
        * 'none': prevent all visualizations from being displayed.
    """
    global _target
    if target not in ["auto", "browser", "gui", "none"]:
        raise ValueError(
            "Expected target to be one of: 'auto', 'browser', 'gui', 'none'."
        )
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

    def __init__(self, vega_spec=None, _proxy=None):
        if vega_spec is not None:
            import turicreate as tc

            self.__proxy__ = tc.extensions.plot_from_vega_spec(vega_spec)
        else:
            self.__proxy__ = _proxy

    def show(self):
        """
        A method for displaying the Plot object

        Notes
        -----
        - The plot will render either inline in a Jupyter Notebook, in a web
          browser, or in a native GUI window, depending on the value provided in
          `turicreate.visualization.set_target` (defaults to 'auto').

        Examples
        --------
        Suppose 'plt' is an Plot Object

        We can view it using:

        >>> plt.show()

        """
        global _target

        # Suppress visualization output if 'none' target is set
        if _target == "none":
            return

        # If browser target is set, launch in web browser
        if _target == "browser":
            # First, make sure TURI_VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY is set
            _ensure_web_server()

            # Launch this plot's URL using Python built-in webbrowser module
            import webbrowser

            url = self.get_url()
            webbrowser.open_new_tab(url)
            return

        # If auto target is set, try to show inline in Jupyter Notebook
        try:
            if _target == "auto" and (
                get_ipython().__class__.__name__ == "ZMQInteractiveShell"
                or get_ipython().__class__.__name__ == "Shell"
            ):
                self._repr_javascript_()
                return
        except NameError:
            pass

        path_to_client = _get_client_app_path()

        # _target can only be one of ['auto', 'browser', 'gui', 'none'].
        # We have already returned early for auto (in Jupyter Notebook) and
        # browser. At this point, we expect _target to be either "auto"
        # (not in Jupyter Notebook) or "gui". This is enforced in set_target.
        # Thus, proceed to launch the GUI.

        # TODO: allow autodetection of light/dark mode.
        # Disabled for now, since the GUI side needs some work (ie. background color).
        plot_variation = 0x10  # force light mode
        self.__proxy__.call_function(
            "show", {"path_to_client": path_to_client, "variation": plot_variation}
        )

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
            spec = self.get_vega(include_data=True)
            with open(filepath, "w") as fp:
                _json.dump(spec, fp)
        elif filepath.endswith(".png") or filepath.endswith(".svg"):
            # save as png/svg, but json first
            spec = self.get_vega(include_data=True)
            EXTENSION_START_INDEX = -3
            extension = filepath[EXTENSION_START_INDEX:]
            temp_file_tuple = _mkstemp()
            temp_file_path = temp_file_tuple[1]
            with open(temp_file_path, "w") as fp:
                _json.dump(spec, fp)
            dirname = _os.path.dirname(__file__)
            relative_path_to_vg2png_vg2svg = "../vg2" + extension
            absolute_path_to_vg2png_vg2svg = _os.path.join(
                dirname, relative_path_to_vg2png_vg2svg
            )
            # try node vg2[png|svg] json_filepath out_filepath
            (exitcode, stdout, stderr) = _run_cmdline(
                "node "
                + absolute_path_to_vg2png_vg2svg
                + " "
                + temp_file_path
                + " "
                + filepath
            )

            if exitcode == _NODE_NOT_FOUND_ERROR_CODE:
                # user doesn't have node installed
                raise RuntimeError(
                    "Node.js not found. Saving as PNG and SVG"
                    + " requires Node.js, please download and install Node.js "
                    + "from here and try again: https://nodejs.org/en/download/"
                )
            elif exitcode == _CANVAS_PREBUILT_NOT_FOUND_ERROR:
                # try to see if canvas-prebuilt is globally installed
                # if it is, then link it
                # if not, tell the user to install it
                (
                    is_installed_exitcode,
                    is_installed_stdout,
                    is_installed_stderr,
                ) = _run_cmdline("npm ls -g -json | grep canvas-prebuilt")
                if is_installed_exitcode == _SUCCESS:
                    # npm link canvas-prebuilt
                    link_exitcode, link_stdout, link_stderr = _run_cmdline(
                        "npm link canvas-prebuilt"
                    )
                    if link_exitcode == _PERMISSION_DENIED_ERROR_CODE:
                        # They don't have permission, tell them.
                        raise RuntimeError(
                            link_stderr
                            + "\n\n"
                            + "`npm link canvas-prebuilt` failed, "
                            + "Permission Denied."
                        )
                    elif link_exitcode == _SUCCESS:
                        # canvas-prebuilt link is now successful, so run the
                        # node vg2[png|svg] json_filepath out_filepath
                        # command again.
                        (exitcode, stdout, stderr) = _run_cmdline(
                            "node "
                            + absolute_path_to_vg2png_vg2svg
                            + " "
                            + temp_file_path
                            + " "
                            + filepath
                        )
                        if exitcode != _SUCCESS:
                            # something else that we have not identified yet
                            # happened.
                            raise RuntimeError(stderr)
                    else:
                        raise RuntimeError(link_stderr)
                else:
                    raise RuntimeError(
                        "canvas-prebuilt not found. "
                        + "Saving as PNG and SVG requires canvas-prebuilt, "
                        + "please download and install canvas-prebuilt by "
                        + "running this command, and try again: "
                        + "`npm install -g canvas-prebuilt`"
                    )
            elif exitcode == _SUCCESS:
                pass
            else:
                raise RuntimeError(stderr)
            # delete temp file that user didn't ask for
            _run_cmdline("rm " + temp_file_path)
        else:
            raise NotImplementedError("filename must end in" + " .json, .svg, or .png")

    def get_data(self):
        return _json.loads(self.__proxy__.call_function("get_data"))

    def get_vega(self, include_data=True):
        # TODO: allow autodetection of light/dark mode.
        # Disabled for now, since the GUI side needs some work (ie. background color).
        plot_variation = 0x10  # force light mode
        return _json.loads(
            self.__proxy__.call_function(
                "get_spec", {"include_data": include_data, "variation": plot_variation}
            )
        )

    def materialize(self):
        self.__proxy__.call_function("materialize")

    def get_url(self):
        """
        Returns a URL to the Plot hosted as a web page.

        Notes
        --------
        The URL will be served by Turi Create on http://localhost.
        """
        return self.__proxy__.call_function("get_url")

    def _repr_javascript_(self):
        from IPython.core.display import display, HTML

        self.materialize()
        vega_spec = self.get_vega(True)

        vega_html = (
            '<html lang="en"> \
                        <head> \
                            <script src="https://cdnjs.cloudflare.com/ajax/libs/vega/5.4.0/vega.js"></script> \
                            <script src="https://cdnjs.cloudflare.com/ajax/libs/vega-embed/4.0.0/vega-embed.js"></script> \
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
                                var vega_json = '
            + _json.dumps(_json.dumps(vega_spec))
            .replace("&", "&amp;")
            .replace("'", "&apos;")
            + '; \
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
        )

        display(
            HTML(
                '<html> \
                <body> \
                    <iframe style="border:0;margin:0" width="'
                + str((vega_spec["width"] if "width" in vega_spec else 600) + 200)
                + '" height="'
                + str(vega_spec["height"] + 220)
                + '" srcdoc='
                + "'"
                + vega_html
                + "'"
                + ' src="demo_iframe_srcdoc.htm"> \
                        <p>Your browser does not support iframes.</p> \
                    </iframe> \
                </body> \
            </html>'
            )
        )


def display_table_in_notebook(sf, title=None):
    from IPython.core.display import display

    import base64
    from io import BytesIO
    from IPython.display import HTML
    from ..data_structures.image import Image as _Image

    def image_formatter(im):
        image_buffer = BytesIO()
        im.save(image_buffer, format="PNG")
        return (
            '<img src="data:image/png;base64,'
            + base64.b64encode(image_buffer.getvalue()).decode()
            + '"/>'
        )

    import pandas as pd

    maximum_rows = 100
    if len(sf) > maximum_rows:
        import warnings

        warnings.warn("Displaying only the first {} rows.".format(maximum_rows))
        sf = sf[:maximum_rows]

    check_image_column = [_Image == x for x in sf.column_types()]
    zipped_image_columns = zip(sf.column_names(), check_image_column)
    image_columns = filter(lambda a: a[1], zipped_image_columns)
    image_key = [x[0] for x in image_columns]
    image_column_formatter = dict.fromkeys(image_key, image_formatter)

    with pd.option_context(
        "display.max_rows",
        None,
        "display.max_columns",
        None,
        "display.max_colwidth",
        -1,
    ):
        if _sys.version_info.major < 3:
            import cgi

            title = cgi.escape(title, quote=True)
        else:
            import html

            title = html.escape(title, quote=True)

        df = sf.to_dataframe()
        html_string = (
            '<html lang="en">                           \
                          <head>                                   \
                            <style>                                \
                              .sframe {                            \
                                font-size: 12px;                   \
                                font-family: HelveticaNeue;        \
                                border: 1px solid silver;          \
                              }                                    \
                              .sframe thead th {                   \
                                background: #F7F7F7;               \
                                font-family: HelveticaNeue-Medium; \
                                font-size: 14px;                   \
                                line-height: 16.8px;               \
                                padding-top: 16px;                 \
                                padding-bottom: 16px;              \
                                padding-left: 10px;                \
                                padding-right: 38px;               \
                                border-top: 1px solid #E9E9E9;     \
                                border-bottom: 1px solid #E9E9E9;  \
                                white-space: nowrap;               \
                                overflow: hidden;                  \
                                text-overflow:ellipsis;            \
                                text-align:center;                 \
                                font-weight:normal;                \
                              }                                    \
                              .sframe tbody th {                   \
                                background: #FFFFFF;               \
                                text-align:left;                   \
                                font-weight:normal;                \
                                border-right: 1px solid #E9E9E9;   \
                              }                                    \
                              .sframe td {                         \
                                background: #FFFFFF;               \
                                padding-left: 10px;                \
                                padding-right: 38px;               \
                                padding-top: 14px;                 \
                                padding-bottom: 14px;              \
                                border-bottom: 1px solid #E9E9E9;  \
                                max-height: 0px;                   \
                                transition: max-height 5s ease-out;\
                                vertical-align: middle;            \
                                font-family: HelveticaNeue;        \
                                font-size: 12px;                   \
                                line-height: 16.8px;               \
                                background: #FFFFFF;               \
                              }                                    \
                              .sframe tr {                         \
                                padding-left: 10px;                \
                                padding-right: 38px;               \
                                padding-top: 14px;                 \
                                padding-bottom: 14px;              \
                                border-bottom: 1px solid #E9E9E9;  \
                                max-height: 0px;                   \
                                transition: max-height 5s ease-out;\
                                vertical-align: middle;            \
                                font-family: HelveticaNeue;        \
                                font-size: 12px;                   \
                                line-height: 16.8px;               \
                                background: #FFFFFF;               \
                              }                                    \
                              .sframe tr:hover {                   \
                                background: silver;                \
                              },                                   \
                            </style>                               \
                          </head>                                  \
                          <body>                                   \
                            <h1> '
            + title
            + " </h1>                 \
                            "
            + df.to_html(
                formatters=image_column_formatter, escape=False, classes="sframe"
            )
            + "\
                          </body>                                  \
                        </html>"
        )

        display(HTML(html_string))
