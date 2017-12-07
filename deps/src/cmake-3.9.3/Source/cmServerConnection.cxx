/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmServerConnection.h"

#include "cmFileMonitor.h"
#include "cmServer.h"
#include "cmServerDictionary.h"

#include <assert.h>
#include <string.h>

namespace {

struct write_req_t
{
  uv_write_t req;
  uv_buf_t buf;
};

void on_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
  (void)(handle);
  char* rawBuffer = new char[suggested_size];
  *buf = uv_buf_init(rawBuffer, static_cast<unsigned int>(suggested_size));
}

void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
  auto conn = reinterpret_cast<cmServerConnection*>(stream->data);
  if (nread >= 0) {
    conn->ReadData(std::string(buf->base, buf->base + nread));
  } else {
    conn->TriggerShutdown();
  }

  delete[](buf->base);
}

void on_write(uv_write_t* req, int status)
{
  (void)(status);
  auto conn = reinterpret_cast<cmServerConnection*>(req->data);

  // Free req and buffer
  write_req_t* wr = reinterpret_cast<write_req_t*>(req);
  delete[](wr->buf.base);
  delete wr;

  conn->ProcessNextRequest();
}

void on_new_connection(uv_stream_t* stream, int status)
{
  (void)(status);
  auto conn = reinterpret_cast<cmServerConnection*>(stream->data);
  conn->Connect(stream);
}

void on_signal(uv_signal_t* signal, int signum)
{
  auto conn = reinterpret_cast<cmServerConnection*>(signal->data);
  (void)(signum);
  conn->TriggerShutdown();
}

void on_signal_close(uv_handle_t* handle)
{
  delete reinterpret_cast<uv_signal_t*>(handle);
}

void on_pipe_close(uv_handle_t* handle)
{
  delete reinterpret_cast<uv_pipe_t*>(handle);
}

void on_tty_close(uv_handle_t* handle)
{
  delete reinterpret_cast<uv_tty_t*>(handle);
}

} // namespace

class LoopGuard
{
public:
  LoopGuard(cmServerConnection* connection)
    : Connection(connection)
  {
    this->Connection->mLoop = uv_default_loop();
    if (!this->Connection->mLoop) {
      return;
    }
    this->Connection->mFileMonitor =
      new cmFileMonitor(this->Connection->mLoop);
  }

  ~LoopGuard()
  {
    if (!this->Connection->mLoop) {
      return;
    }

    if (this->Connection->mFileMonitor) {
      delete this->Connection->mFileMonitor;
    }
    uv_loop_close(this->Connection->mLoop);
    this->Connection->mLoop = nullptr;
  }

private:
  cmServerConnection* Connection;
};

cmServerConnection::cmServerConnection()
{
}

cmServerConnection::~cmServerConnection()
{
}

void cmServerConnection::SetServer(cmServer* s)
{
  this->Server = s;
}

bool cmServerConnection::ProcessEvents(std::string* errorMessage)
{
  assert(this->Server);
  errorMessage->clear();

  this->RawReadBuffer.clear();
  this->RequestBuffer.clear();

  LoopGuard guard(this);
  (void)(guard);
  if (!this->mLoop) {
    *errorMessage = "Internal Error: Failed to create event loop.";
    return false;
  }

  this->SIGINTHandler = new uv_signal_t;
  uv_signal_init(this->mLoop, this->SIGINTHandler);
  this->SIGINTHandler->data = static_cast<void*>(this);
  uv_signal_start(this->SIGINTHandler, &on_signal, SIGINT);

  this->SIGHUPHandler = new uv_signal_t;
  uv_signal_init(this->mLoop, this->SIGHUPHandler);
  this->SIGHUPHandler->data = static_cast<void*>(this);
  uv_signal_start(this->SIGHUPHandler, &on_signal, SIGHUP);

  if (!DoSetup(errorMessage)) {
    return false;
  }

  if (uv_run(this->mLoop, UV_RUN_DEFAULT) != 0) {
    *errorMessage = "Internal Error: Event loop stopped in unclean state.";
    return false;
  }

  // These need to be cleaned up by now:
  assert(!this->ReadStream);
  assert(!this->WriteStream);

  this->RawReadBuffer.clear();
  this->RequestBuffer.clear();

  return true;
}

void cmServerConnection::ReadData(const std::string& data)
{
  this->RawReadBuffer += data;

  for (;;) {
    auto needle = this->RawReadBuffer.find('\n');

    if (needle == std::string::npos) {
      return;
    }
    std::string line = this->RawReadBuffer.substr(0, needle);
    const auto ls = line.size();
    if (ls > 1 && line.at(ls - 1) == '\r') {
      line.erase(ls - 1, 1);
    }
    this->RawReadBuffer.erase(this->RawReadBuffer.begin(),
                              this->RawReadBuffer.begin() +
                                static_cast<long>(needle) + 1);
    if (line == kSTART_MAGIC) {
      this->RequestBuffer.clear();
      continue;
    }
    if (line == kEND_MAGIC) {
      this->Server->QueueRequest(this->RequestBuffer);
      this->RequestBuffer.clear();
    } else {
      this->RequestBuffer += line;
      this->RequestBuffer += "\n";
    }
  }
}

void cmServerConnection::TriggerShutdown()
{
  this->FileMonitor()->StopMonitoring();

  uv_signal_stop(this->SIGINTHandler);
  uv_signal_stop(this->SIGHUPHandler);

  uv_close(reinterpret_cast<uv_handle_t*>(this->SIGINTHandler),
           &on_signal_close); // delete handle
  uv_close(reinterpret_cast<uv_handle_t*>(this->SIGHUPHandler),
           &on_signal_close); // delete handle

  this->SIGINTHandler = nullptr;
  this->SIGHUPHandler = nullptr;

  this->TearDown();
}

void cmServerConnection::WriteData(const std::string& data)
{
  assert(this->WriteStream);

  auto ds = data.size();

  write_req_t* req = new write_req_t;
  req->req.data = this;
  req->buf = uv_buf_init(new char[ds], static_cast<unsigned int>(ds));
  memcpy(req->buf.base, data.c_str(), ds);

  uv_write(reinterpret_cast<uv_write_t*>(req),
           static_cast<uv_stream_t*>(this->WriteStream), &req->buf, 1,
           on_write);
}

void cmServerConnection::ProcessNextRequest()
{
  Server->PopOne();
}

void cmServerConnection::SendGreetings()
{
  Server->PrintHello();
}

cmServerStdIoConnection::cmServerStdIoConnection()
{
  this->Input.tty = nullptr;
  this->Output.tty = nullptr;
}

bool cmServerStdIoConnection::DoSetup(std::string* errorMessage)
{
  (void)(errorMessage);

  if (uv_guess_handle(1) == UV_TTY) {
    usesTty = true;
    this->Input.tty = new uv_tty_t;
    uv_tty_init(this->Loop(), this->Input.tty, 0, 1);
    uv_tty_set_mode(this->Input.tty, UV_TTY_MODE_NORMAL);
    Input.tty->data = this;
    this->ReadStream = reinterpret_cast<uv_stream_t*>(this->Input.tty);

    this->Output.tty = new uv_tty_t;
    uv_tty_init(this->Loop(), this->Output.tty, 1, 0);
    uv_tty_set_mode(this->Output.tty, UV_TTY_MODE_NORMAL);
    Output.tty->data = this;
    this->WriteStream = reinterpret_cast<uv_stream_t*>(this->Output.tty);
  } else {
    usesTty = false;
    this->Input.pipe = new uv_pipe_t;
    uv_pipe_init(this->Loop(), this->Input.pipe, 0);
    uv_pipe_open(this->Input.pipe, 0);
    Input.pipe->data = this;
    this->ReadStream = reinterpret_cast<uv_stream_t*>(this->Input.pipe);

    this->Output.pipe = new uv_pipe_t;
    uv_pipe_init(this->Loop(), this->Output.pipe, 0);
    uv_pipe_open(this->Output.pipe, 1);
    Output.pipe->data = this;
    this->WriteStream = reinterpret_cast<uv_stream_t*>(this->Output.pipe);
  }

  SendGreetings();
  uv_read_start(this->ReadStream, on_alloc_buffer, on_read);

  return true;
}

void cmServerStdIoConnection::TearDown()
{
  if (usesTty) {
    uv_close(reinterpret_cast<uv_handle_t*>(this->Input.tty), &on_tty_close);
    uv_close(reinterpret_cast<uv_handle_t*>(this->Output.tty), &on_tty_close);
    this->Input.tty = nullptr;
    this->Output.tty = nullptr;
  } else {
    uv_close(reinterpret_cast<uv_handle_t*>(this->Input.pipe), &on_pipe_close);
    uv_close(reinterpret_cast<uv_handle_t*>(this->Output.pipe),
             &on_pipe_close);
    this->Input.pipe = nullptr;
    this->Input.pipe = nullptr;
  }
  this->ReadStream = nullptr;
  this->WriteStream = nullptr;
}

cmServerPipeConnection::cmServerPipeConnection(const std::string& name)
  : PipeName(name)
{
}

bool cmServerPipeConnection::DoSetup(std::string* errorMessage)
{
  this->ServerPipe = new uv_pipe_t;
  uv_pipe_init(this->Loop(), this->ServerPipe, 0);
  this->ServerPipe->data = this;

  int r;
  if ((r = uv_pipe_bind(this->ServerPipe, this->PipeName.c_str())) != 0) {
    *errorMessage = std::string("Internal Error with ") + this->PipeName +
      ": " + uv_err_name(r);
    return false;
  }
  auto serverStream = reinterpret_cast<uv_stream_t*>(this->ServerPipe);
  if ((r = uv_listen(serverStream, 1, on_new_connection)) != 0) {
    *errorMessage = std::string("Internal Error listening on ") +
      this->PipeName + ": " + uv_err_name(r);
    return false;
  }

  return true;
}

void cmServerPipeConnection::TearDown()
{
  if (this->ClientPipe) {
    uv_close(reinterpret_cast<uv_handle_t*>(this->ClientPipe), &on_pipe_close);
    this->WriteStream->data = nullptr;
  }
  uv_close(reinterpret_cast<uv_handle_t*>(this->ServerPipe), &on_pipe_close);

  this->ClientPipe = nullptr;
  this->ServerPipe = nullptr;
  this->WriteStream = nullptr;
  this->ReadStream = nullptr;
}

void cmServerPipeConnection::Connect(uv_stream_t* server)
{
  if (this->ClientPipe) {
    // Accept and close all pipes but the first:
    uv_pipe_t* rejectPipe = new uv_pipe_t;

    uv_pipe_init(this->Loop(), rejectPipe, 0);
    auto rejecter = reinterpret_cast<uv_stream_t*>(rejectPipe);
    uv_accept(server, rejecter);
    uv_close(reinterpret_cast<uv_handle_t*>(rejecter), &on_pipe_close);
    return;
  }

  this->ClientPipe = new uv_pipe_t;
  uv_pipe_init(this->Loop(), this->ClientPipe, 0);
  this->ClientPipe->data = this;
  auto client = reinterpret_cast<uv_stream_t*>(this->ClientPipe);
  if (uv_accept(server, client) != 0) {
    uv_close(reinterpret_cast<uv_handle_t*>(client), nullptr);
    return;
  }
  this->ReadStream = client;
  this->WriteStream = client;

  uv_read_start(this->ReadStream, on_alloc_buffer, on_read);

  this->SendGreetings();
}
