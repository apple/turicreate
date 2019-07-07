// Bootstrap the Turi Create Visualization web app
if(location.search) {
  // If there is a query string, it means we're in browser mode.
  // Start by querying the server for the relevant items.
  // TODO: refactor to share code with the non-browser case,
  // and move all of it to src/index instead of public/index
  // (remove the latter)
  window.tcvizBrowserMode = true;
} else if(window.navigator.platform == 'MacIntel'){
  // If no query string, and we're on macOS, it means we're in
  // Turi Create Visualization.app (in a WKWebView).
  window.tcvizBrowserMode = false;
  window.vegaResult = null;
  function log(level) {
    return function() {
      var args = Array.prototype.slice.call(arguments);
      var msgText = args.map(JSON.stringify).join(' ');
      var message = {
        status: 'log',
        level: level,
        message: msgText
      };
      window.webkit.messageHandlers["scriptHandler"].postMessage(message);
    }
  }

  console = {};
  console.log = log("log");
  console.debug = log("debug");
  console.info = log("info");
  console.warn = log("warn");
  console.error = log("error");

  if (document.readyState == 'complete') {
    window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'loaded'});
  } else {
    window.addEventListener('load', function() {
      window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'loaded'});
    });
  }
}
