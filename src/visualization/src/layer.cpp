#include <iostream>

#include <sys/types.h>
#include <unistd.h>

#include "include/wrapper/cef_helpers.h"

#include "layer.h"
#include "handler.h"
#include "V8Handler.h"

std::string getExecutableBasePath() {
  std::string path = "";
  pid_t pid     = getpid();
  char buf[20] = {0};

  sprintf(buf, "%d", pid);

  std::string _link = "/proc/";
  _link.append(buf);
  _link.append("/exe");
  char proc[512];
  int ch = readlink(_link.c_str(), proc, 512);

  if (ch != -1 ) {
    proc[ch] = 0;
    path     = proc;
    std::string::size_type t = path.find_last_of("/");
    path = path.substr(0, t);
  }

  return path;
}

Layer::Layer(CefRefPtr<JavascriptCaller> javascript_caller_reference){
  Javascript_Caller_Reference = javascript_caller_reference;
}

void Layer::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<Handler> handler(new Handler(false));

  CefBrowserSettings browser_settings;

  std::string path = getExecutableBasePath();
  path = std::string("file://") + path + std::string("/html/index.html");

  CefWindowInfo window_info;
  CefRefPtr<CefBrowser> current_browser = CefBrowserHost::CreateBrowserSync(window_info, handler, path, browser_settings, NULL);

  Browser = current_browser;
}

void Layer::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
  CEF_REQUIRE_RENDERER_THREAD();
  if(!created){
    created = true;

    CefRefPtr<CefV8Value> object = context->GetGlobal();
    CefRefPtr<CefV8Handler> handler = new V8Handler();
    CefRefPtr<CefV8Value> post_message_func = CefV8Value::CreateFunction("postMessageToNativeClient", handler);

    object->SetValue("postMessageToNativeClient", post_message_func, V8_PROPERTY_ATTRIBUTE_NONE);

    Javascript_Caller_Reference->initialize(browser, frame, context);
    Javascript_Caller_Reference->loaded();
  }
}


 bool Layer::OnProcessMessageReceived(CefRefPtr< CefBrowser > browser, CefProcessId source_process, CefRefPtr< CefProcessMessage > message){
  CEF_REQUIRE_RENDERER_THREAD();

  CefRefPtr<CefListValue> argument = message->GetArgumentList();

  CefString message_name = message->GetName();
  CefString message_body = argument->GetString(0);

  std::string message_name_parsed = message_name.ToString();

  if(message_name_parsed.compare("cef_ipc_message") == 0){
    std::string parsed_message = message_body.ToString();
    Javascript_Caller_Reference->sendSpec(parsed_message);
  }

  return true;
 }
