#ifndef V8ACCESSOR_H_
#define V8ACCESSOR_H_

#include "include/cef_v8.h"

class V8Accessor : public CefV8Accessor {
  public:
    V8Accessor() {}

  virtual bool Get(const CefString& name, const CefRefPtr<CefV8Value> object, CefRefPtr<CefV8Value>& retval, CefString& exception) OVERRIDE {
    if (name == "myval") {
      retval = CefV8Value::CreateString(myval_);
      return true;
    }
    return false;
  }

  virtual bool Set(const CefString& name, const CefRefPtr<CefV8Value> object, const CefRefPtr<CefV8Value> value, CefString& exception) OVERRIDE {
    if (name == "myval") {
      if (value->IsString()) {
        myval_ = value->GetStringValue();
      } else {
        exception = "Invalid value type";
      }
      return true;
    }
    return false;
  }

  CefString myval_;

  IMPLEMENT_REFCOUNTING(V8Accessor);
};

#endif
