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

    bool hasNext();
    std::string peek();
    std::string pop();
  private:
    void read();
    void pipe_loop(CefRefPtr<Layer> &app, std::queue<std::string> &q1, std::mutex &mtx1);

    IMPLEMENT_REFCOUNTING(Pipe);
};

#endif
