#include "handler.h"

#include <iostream>
#include <sstream>
#include <string>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

namespace {
  Handler* g_instance = NULL;
}

Handler::Handler(bool use_views) : use_views_(use_views), is_closing_(false) {
  DCHECK(!g_instance);
  g_instance = this;
}

Handler::~Handler() {
  g_instance = NULL;
}

Handler* Handler::GetInstance() {
  return g_instance;
}

void Handler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
  CEF_REQUIRE_UI_THREAD();
  if (use_views_){
    CefRefPtr<CefBrowserView> browser_view = CefBrowserView::GetForBrowser(browser);
    if (browser_view){
      CefRefPtr<CefWindow> window = browser_view->GetWindow();
      if (window)
        window->SetTitle(title);
    }
  }else{
    PlatformTitleChange(browser, title);
  }
}

bool Handler::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line) {
  CEF_REQUIRE_UI_THREAD();

  std::size_t found = message.ToString().find("[Tooltip]");

  if(!(level == 3 && found != std::string::npos)){
    std::cerr << "LOG LEVEL: " << level << ", message: " << message.ToString() << std::endl;
  }

  return true;
}

void Handler::OnAfterCreated(CefRefPtr<CefBrowser> browser){
  CEF_REQUIRE_UI_THREAD();
  browser_list_.push_back(browser);
}

bool Handler::DoClose(CefRefPtr<CefBrowser> browser){
  CEF_REQUIRE_UI_THREAD();
  if (browser_list_.size() == 1) {
    is_closing_ = true;
  }
  return false;
}

void Handler::OnBeforeClose(CefRefPtr<CefBrowser> browser){
  CEF_REQUIRE_UI_THREAD();
  BrowserList::iterator bit = browser_list_.begin();
  for (; bit != browser_list_.end(); ++bit) {
    if ((*bit)->IsSame(browser)) {
      browser_list_.erase(bit);
      break;
    }
  }

  if (browser_list_.empty()) {
    CefQuitMessageLoop();
  }
}

void Handler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();

  if (errorCode == ERR_ABORTED){
    return;
  }

  std::stringstream ss;
  ss << "<html><body><span style='color:red'>FATAL:</span>Cannot Find Turi Create Visualization: Source Files</body></html>";
  frame->LoadString(ss.str(), failedUrl);
}

void Handler::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    CefPostTask(TID_UI, base::Bind(&Handler::CloseAllBrowsers, this, force_close));
    return;
  }

  if (browser_list_.empty()){
    return;
  }

  BrowserList::const_iterator it = browser_list_.begin();
  for (; it != browser_list_.end(); ++it) {
    (*it)->GetHost()->CloseBrowser(force_close);
  }
}
