#ifndef WINDOW_DELEGATE_H_
#define WINDOW_DELEGATE_H_

#include "include/cef_app.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

class WindowDelegate : public CefWindowDelegate {
  public:
    explicit WindowDelegate(CefRefPtr<CefBrowserView> browser_view) : browser_view_(browser_view) {}

    void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE {
      window->AddChildView(browser_view_);
      window->Show();
      browser_view_->RequestFocus();
    }

    void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE {
      browser_view_ = NULL;
    }

    bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE {
      CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
      if (browser){
        return browser->GetHost()->TryCloseBrowser();
      }
      return true;
    }

  private:
    CefRefPtr<CefBrowserView> browser_view_;

    IMPLEMENT_REFCOUNTING(WindowDelegate);
    DISALLOW_COPY_AND_ASSIGN(WindowDelegate);
};

#endif
