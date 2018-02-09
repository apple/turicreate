#include "javascript_caller.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <unistd.h>
#include <thread>
#include "json.hpp"


void JavascriptCaller::initialize(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context){
  Browser = browser;
  Frame = frame;
  Context = context;
}

void JavascriptCaller::start(){
  std::string vega_spec = "{}";

  CefRefPtr<CefV8Value> object = Context->GetGlobal();
  CefRefPtr<CefV8Value> str = CefV8Value::CreateString(vega_spec);

  object->SetValue("vega_spec", str, V8_PROPERTY_ATTRIBUTE_NONE);
}

void JavascriptCaller::sendSpec(std::string &line){
  using json = nlohmann::json;
  Context->Enter();

  json j3;
  j3 = json::parse(line);


  if(!j3["table_spec"].is_null()){
    json input_value;

    input_value["data"] = j3["table_spec"];
    input_value["type"] = "table";

    std::stringstream javascript_function;

    javascript_function << "window.setSpec(";
    javascript_function << input_value.dump();
    javascript_function << ");";

    Frame->ExecuteJavaScript(javascript_function.str(), Frame->GetURL(), 0);
  }

  if(!j3["vega_spec"].is_null()){
    json input_value;

    input_value["data"] = j3["vega_spec"];
    input_value["type"] = "vega";

    std::stringstream javascript_function;

    javascript_function << "window.setSpec(";
    javascript_function << input_value.dump();
    javascript_function << ");";

    Frame->ExecuteJavaScript(javascript_function.str(), Frame->GetURL(), 0);
  }

  if(!j3["data_spec"].is_null()){
    std::stringstream javascript_function;

    javascript_function << "window.updateData(";
    javascript_function << j3["data_spec"].dump();
    javascript_function << ");";

    Frame->ExecuteJavaScript(javascript_function.str(), Frame->GetURL(), 0);
  }

  if(!j3["image_spec"].is_null()){
    json input_value;

    std::stringstream javascript_function;

    input_value["data"] = j3["image_spec"];

    javascript_function << "window.setImageData(";
    javascript_function << input_value.dump();
    javascript_function << ");";

    Frame->ExecuteJavaScript(javascript_function.str(), Frame->GetURL(), 0);
  }

  Context->Exit();
}
