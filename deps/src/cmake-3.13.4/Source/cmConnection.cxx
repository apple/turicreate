/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmConnection.h"

#include "cmServer.h"
#include "cm_uv.h"

#include <cassert>
#include <cstring>

struct write_req_t
{
  uv_write_t req;
  uv_buf_t buf;
};

void cmEventBasedConnection::on_alloc_buffer(uv_handle_t* handle,
                                             size_t suggested_size,
                                             uv_buf_t* buf)
{
  (void)(handle);
  char* rawBuffer = new char[suggested_size];
  *buf = uv_buf_init(rawBuffer, static_cast<unsigned int>(suggested_size));
}

void cmEventBasedConnection::on_read(uv_stream_t* stream, ssize_t nread,
                                     const uv_buf_t* buf)
{
  auto conn = static_cast<cmEventBasedConnection*>(stream->data);
  if (conn) {
    if (nread >= 0) {
      conn->ReadData(std::string(buf->base, buf->base + nread));
    } else {
      conn->OnDisconnect(static_cast<int>(nread));
    }
  }

  delete[](buf->base);
}

void cmEventBasedConnection::on_close(uv_handle_t* /*handle*/)
{
}

void cmEventBasedConnection::on_write(uv_write_t* req, int status)
{
  (void)(status);

  // Free req and buffer
  write_req_t* wr = reinterpret_cast<write_req_t*>(req);
  delete[](wr->buf.base);
  delete wr;
}

void cmEventBasedConnection::on_new_connection(uv_stream_t* stream, int status)
{
  (void)(status);
  auto conn = static_cast<cmEventBasedConnection*>(stream->data);

  if (conn) {
    conn->Connect(stream);
  }
}

bool cmEventBasedConnection::IsOpen() const
{
  return this->WriteStream != nullptr;
}

void cmEventBasedConnection::WriteData(const std::string& _data)
{
#ifndef NDEBUG
  auto curr_thread_id = uv_thread_self();
  assert(this->Server);
  assert(uv_thread_equal(&curr_thread_id, &this->Server->ServeThreadId));
#endif

  auto data = _data;
  assert(this->WriteStream.get());
  if (BufferStrategy) {
    data = BufferStrategy->BufferOutMessage(data);
  }

  auto ds = data.size();

  write_req_t* req = new write_req_t;
  req->req.data = this;
  req->buf = uv_buf_init(new char[ds], static_cast<unsigned int>(ds));
  memcpy(req->buf.base, data.c_str(), ds);
  uv_write(reinterpret_cast<uv_write_t*>(req), this->WriteStream, &req->buf, 1,
           on_write);
}

void cmEventBasedConnection::ReadData(const std::string& data)
{
  this->RawReadBuffer += data;
  if (BufferStrategy) {
    std::string packet = BufferStrategy->BufferMessage(this->RawReadBuffer);
    while (!packet.empty()) {
      ProcessRequest(packet);
      packet = BufferStrategy->BufferMessage(this->RawReadBuffer);
    }
  } else {
    ProcessRequest(this->RawReadBuffer);
    this->RawReadBuffer.clear();
  }
}

cmEventBasedConnection::cmEventBasedConnection(
  cmConnectionBufferStrategy* bufferStrategy)
  : BufferStrategy(bufferStrategy)
{
}

void cmEventBasedConnection::Connect(uv_stream_t* server)
{
  (void)server;
  Server->OnConnected(nullptr);
}

void cmEventBasedConnection::OnDisconnect(int onerror)
{
  (void)onerror;
  this->OnConnectionShuttingDown();
  if (this->Server) {
    this->Server->OnDisconnect(this);
  }
}

cmConnection::~cmConnection()
{
}

bool cmConnection::OnConnectionShuttingDown()
{
  this->Server = nullptr;
  return true;
}

void cmConnection::SetServer(cmServerBase* s)
{
  Server = s;
}

void cmConnection::ProcessRequest(const std::string& request)
{
  Server->ProcessRequest(this, request);
}

bool cmConnection::OnServeStart(std::string* errString)
{
  (void)errString;
  return true;
}

bool cmEventBasedConnection::OnConnectionShuttingDown()
{
  if (this->WriteStream.get()) {
    this->WriteStream->data = nullptr;
  }

  WriteStream.reset();

  return true;
}
