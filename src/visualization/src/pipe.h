#ifndef PIPE_H_
#define PIPE_H_

#include "layer.h"

#include <thread>
#include <mutex>
#include <queue>
#include <iostream>

#include "include/cef_app.h"
#include "include/cef_render_process_handler.h"
#include "include/cef_base.h"

class Pipe : public CefBaseRefCounted {
  public:
    std::queue<std::string> q;
    std::mutex m;

    std::thread pipe_thread;
    std::thread loop_thread;
    Pipe();
    ~Pipe();

    void initialize();

    void runPipeLoop(CefRefPtr<Layer> &app);

  private:
    void read();
    void pipe_loop(CefRefPtr<Layer> &app);

    IMPLEMENT_REFCOUNTING(Pipe);
};

#endif
