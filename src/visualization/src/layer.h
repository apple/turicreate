#ifndef LAYER_H_
#define LAYER_H_

#include "javascript_caller.h"

#include <memory>

#include "include/cef_app.h"
#include "include/cef_render_process_handler.h"
#include "include/cef_process_message.h"

class Layer : public CefApp, public CefBrowserProcessHandler, public CefRenderProcessHandler {
  public:
    Layer(CefRefPtr<JavascriptCaller> javascript_caller_reference);

    virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()  OVERRIDE {
      return this;
    }

    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE {
      return this;
    }


    virtual void OnContextInitialized() OVERRIDE;

    virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) OVERRIDE;

    virtual bool OnProcessMessageReceived( CefRefPtr< CefBrowser > browser, CefProcessId source_process, CefRefPtr< CefProcessMessage > message) OVERRIDE;

    CefRefPtr<JavascriptCaller> Javascript_Caller_Reference;

    CefRefPtr<CefBrowser> Browser;

    bool created = 0;
  private:

    IMPLEMENT_REFCOUNTING(Layer);
};

#endif
