if(window.navigator.platform == 'MacIntel'){
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
