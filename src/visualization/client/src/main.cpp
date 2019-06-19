#include "layer.h"
#include "pipe.h"

#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"

#include "include/cef_process_message.h"

#include <X11/Xlib.h>
#include "include/base/cef_logging.h"
#include <iostream>
#include <thread>

namespace {
  int XErrorHandlerImpl(Display* display, XErrorEvent* event) {
    LOG(WARNING)  << "X error received: "
                  << "type " << event->type << ", "
                  << "serial " << event->serial << ", "
                  << "error_code " << static_cast<int>(event->error_code) << ", "
                  << "request_code " << static_cast<int>(event->request_code)
                  << ", "
                  << "minor_code " << static_cast<int>(event->minor_code);
    return 0;
  }

  int XIOErrorHandlerImpl(Display* display) {
    return 0;
  }
}

int main(int argc, char* argv[]) {

  CefMainArgs main_args(argc, argv);

  CefRefPtr<JavascriptCaller> app_javascript_caller(new JavascriptCaller);

  CefRefPtr<Layer> app(new Layer(app_javascript_caller));

  int exit_code = CefExecuteProcess(main_args, app, NULL);

  if (exit_code >= 0) {
    return exit_code;
  }

  XSetErrorHandler(XErrorHandlerImpl);
  XSetIOErrorHandler(XIOErrorHandlerImpl);

  CefSettings settings;

  CefInitialize(main_args, settings, app, NULL);

  CefRefPtr<Pipe> app_pipe(new Pipe);

  app_pipe->initialize();

  app_pipe->runPipeLoop(app);

  CefRunMessageLoop();

  CefShutdown();

  return 0;
}
