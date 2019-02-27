/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmServerConnection.h"

#include "cmConfigure.h"
#include "cmServer.h"
#include "cmServerDictionary.h"
#include "cm_uv.h"

#include <algorithm>
#ifdef _WIN32
#  include "io.h"
#else
#  include <unistd.h>
#endif
#include <cassert>

cmStdIoConnection::cmStdIoConnection(
  cmConnectionBufferStrategy* bufferStrategy)
  : cmEventBasedConnection(bufferStrategy)
{
}

cm::uv_stream_ptr cmStdIoConnection::SetupStream(int file_id)
{
  switch (uv_guess_handle(file_id)) {
    case UV_TTY: {
      cm::uv_tty_ptr tty;
      tty.init(*this->Server->GetLoop(), file_id, file_id == 0,
               static_cast<cmEventBasedConnection*>(this));
      uv_tty_set_mode(tty, UV_TTY_MODE_NORMAL);
      return std::move(tty);
    }
    case UV_FILE:
      if (file_id == 0) {
        return nullptr;
      }
      // Intentional fallthrough; stdin can _not_ be treated as a named
      // pipe, however stdout can be.
      CM_FALLTHROUGH;
    case UV_NAMED_PIPE: {
      cm::uv_pipe_ptr pipe;
      pipe.init(*this->Server->GetLoop(), 0,
                static_cast<cmEventBasedConnection*>(this));
      uv_pipe_open(pipe, file_id);
      return std::move(pipe);
    }
    default:
      assert(false && "Unable to determine stream type");
      return nullptr;
  }
}

void cmStdIoConnection::SetServer(cmServerBase* s)
{
  cmConnection::SetServer(s);
  if (!s) {
    return;
  }

  this->ReadStream = SetupStream(0);
  this->WriteStream = SetupStream(1);
}

void shutdown_connection(uv_prepare_t* prepare)
{
  cmStdIoConnection* connection =
    static_cast<cmStdIoConnection*>(prepare->data);

  if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(prepare))) {
    uv_close(reinterpret_cast<uv_handle_t*>(prepare),
             &cmEventBasedConnection::on_close_delete<uv_prepare_t>);
  }
  connection->OnDisconnect(0);
}

bool cmStdIoConnection::OnServeStart(std::string* pString)
{
  Server->OnConnected(this);
  if (this->ReadStream.get()) {
    uv_read_start(this->ReadStream, on_alloc_buffer, on_read);
  } else if (uv_guess_handle(0) == UV_FILE) {
    char buffer[1024];
    while (auto len = read(0, buffer, sizeof(buffer))) {
      ReadData(std::string(buffer, buffer + len));
    }

    // We can't start the disconnect from here, add a prepare hook to do that
    // for us
    auto prepare = new uv_prepare_t();
    prepare->data = this;
    uv_prepare_init(Server->GetLoop(), prepare);
    uv_prepare_start(prepare, shutdown_connection);
  }
  return cmConnection::OnServeStart(pString);
}

bool cmStdIoConnection::OnConnectionShuttingDown()
{
  if (ReadStream.get()) {
    uv_read_stop(ReadStream);
    ReadStream->data = nullptr;
  }

  this->ReadStream.reset();

  cmEventBasedConnection::OnConnectionShuttingDown();

  return true;
}

cmServerPipeConnection::cmServerPipeConnection(const std::string& name)
  : cmPipeConnection(name, new cmServerBufferStrategy)
{
}

cmServerStdIoConnection::cmServerStdIoConnection()
  : cmStdIoConnection(new cmServerBufferStrategy)
{
}

cmConnectionBufferStrategy::~cmConnectionBufferStrategy()
{
}

void cmConnectionBufferStrategy::clear()
{
}

std::string cmServerBufferStrategy::BufferOutMessage(
  const std::string& rawBuffer) const
{
  return std::string("\n") + kSTART_MAGIC + std::string("\n") + rawBuffer +
    kEND_MAGIC + std::string("\n");
}

std::string cmServerBufferStrategy::BufferMessage(std::string& RawReadBuffer)
{
  for (;;) {
    auto needle = RawReadBuffer.find('\n');

    if (needle == std::string::npos) {
      return "";
    }
    std::string line = RawReadBuffer.substr(0, needle);
    const auto ls = line.size();
    if (ls > 1 && line.at(ls - 1) == '\r') {
      line.erase(ls - 1, 1);
    }
    RawReadBuffer.erase(RawReadBuffer.begin(),
                        RawReadBuffer.begin() + static_cast<long>(needle) + 1);
    if (line == kSTART_MAGIC) {
      RequestBuffer.clear();
      continue;
    }
    if (line == kEND_MAGIC) {
      std::string rtn;
      rtn.swap(this->RequestBuffer);
      return rtn;
    }

    this->RequestBuffer += line;
    this->RequestBuffer += "\n";
  }
}
