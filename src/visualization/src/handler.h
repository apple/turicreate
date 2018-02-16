#ifndef HANDLER_H_
#define HANDLER_H_

#include <list>

#include "include/cef_client.h"

class Handler : public CefClient, public CefDisplayHandler, public CefLifeSpanHandler, public CefLoadHandler {
  public:
    explicit Handler(bool use_views);
    ~Handler();

      static Handler* GetInstance();

      virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE {
        return this;
      }

      virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE {
        return this;
      }

      virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE {
        return this;
      }

      virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) OVERRIDE;

      virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;

      virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

      virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

      virtual void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) OVERRIDE;

      virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line) OVERRIDE;

      void CloseAllBrowsers(bool force_close);

      bool IsClosing() const {
        return is_closing_;
      }

  private:
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);

    const bool use_views_;

    typedef std::list<CefRefPtr<CefBrowser>> BrowserList;

    BrowserList browser_list_;

    bool is_closing_;

    IMPLEMENT_REFCOUNTING(Handler);
};

#endif
