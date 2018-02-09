#include "pipe.h"
#include <chrono>

#include "include/cef_process_message.h"

Pipe::Pipe(){
}

Pipe::~Pipe(){
  // TODO send kill signal to threads
}

void Pipe::read(){
  while(true){
    std::string name;
    std::getline(std::cin, name);
    if(!name.empty()){
      std::lock_guard<std::mutex> lock(m);
      q.push(name);
    }
  }
}

void Pipe::initialize(){
  pipe_thread = std::thread([this](){
    read();
  });
  pipe_thread.detach();
}

void Pipe::pipe_loop(CefRefPtr<Layer> &app, std::queue<std::string> &q1, std::mutex &mtx1){
  while(true){
    if(app != NULL){
      if(app->Browser != NULL){
        if(!app->Browser->IsLoading() && app->Browser->HasDocument()) {
          if(q1.size() > 0){
            std::lock_guard<std::mutex> lock(mtx1);
            std::string value = q1.front();
            q1.pop();

            CefRefPtr<CefProcessMessage> msg= CefProcessMessage::Create("tc_message");
            CefRefPtr<CefListValue> argument = msg->GetArgumentList();
            argument->SetString(0, value.c_str());
            app->Browser->SendProcessMessage(PID_RENDERER, msg);
            continue;
          }
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void Pipe::runPipeLoop(CefRefPtr<Layer> &app){
  loop_thread = std::thread([&app, this](){
    pipe_loop(app, q, m);
  });
  loop_thread.detach();
}

bool Pipe::hasNext(){
  std::lock_guard<std::mutex> lock(m);
  if(q.size() > 0){
    return true;
  }
  return false;
}

std::string Pipe::peek(){
  std::lock_guard<std::mutex> lock(m);
  std::string value = q.front();
  return value;
}

std::string Pipe::pop(){
  std::lock_guard<std::mutex> lock(m);
  std::string value = q.front();
  q.pop();
  return value;
}
