#ifndef JAVASCRIPT_CALLER_H_
#define JAVASCRIPT_CALLER_H_

#include <thread>

#include "include/cef_app.h"
#include "include/cef_render_process_handler.h"
#include "include/cef_base.h"

class JavascriptCaller : public CefBaseRefCounted {
  public:
    JavascriptCaller(): Browser(0), Frame(0), Context(0){};

    void initialize(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context);

    void loaded();

    void sendSpec(std::string &line);
  private:
    CefRefPtr<CefBrowser> Browser;
    CefRefPtr<CefFrame> Frame;
    CefRefPtr<CefV8Context> Context;

    IMPLEMENT_REFCOUNTING(JavascriptCaller);
};


#endif
