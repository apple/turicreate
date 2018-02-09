#ifndef V8HANDLER_H_
#define V8HANDLER_H_

#include <iostream>
#include "include/cef_v8.h"

class V8Handler : public CefV8Handler {
  public:
    V8Handler(){};

    virtual bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) OVERRIDE {
      if (name == "linux_two_coms") {
        std::cout << arguments[0]->GetStringValue().ToString() << std::endl;
      }

      return false;
    }

  IMPLEMENT_REFCOUNTING(V8Handler);
};

#endif
