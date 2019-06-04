#include "javascript_caller.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <unistd.h>
#include <thread>


void JavascriptCaller::initialize(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context){
  Browser = browser;
  Frame = frame;
  Context = context;
}

void JavascriptCaller::loaded(){
  std::string vega_spec = "{}";

  CefRefPtr<CefV8Value> object = Context->GetGlobal();
  CefRefPtr<CefV8Value> str = CefV8Value::CreateString(vega_spec);

  object->SetValue("vega_spec", str, V8_PROPERTY_ATTRIBUTE_NONE);
}

void JavascriptCaller::sendSpec(std::string &line){
  Context->Enter();

  std::stringstream javascript_function;
  javascript_function << "window.handleInput(";
  javascript_function << line.c_str();
  javascript_function << ");";

  Frame->ExecuteJavaScript(javascript_function.str(), Frame->GetURL(), 0);

  Context->Exit();
}
