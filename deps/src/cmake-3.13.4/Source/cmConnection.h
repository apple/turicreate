/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#pragma once

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmUVHandlePtr.h"
#include "cm_uv.h"

#include <cstddef>
#include <memory>
#include <string>

class cmServerBase;

/***
 * Given a sequence of bytes with any kind of buffering, instances of this
 * class arrange logical chunks according to whatever the use case is for
 * the connection.
 */
class cmConnectionBufferStrategy
{
public:
  virtual ~cmConnectionBufferStrategy();

  /***
   * Called whenever with an active raw buffer. If a logical chunk
   * becomes available, that chunk is returned and that portion is
   * removed from the rawBuffer
   *
   * @param rawBuffer in/out parameter. Receive buffer; the buffer strategy is
   * free to manipulate this buffer anyway it needs to.
   *
   * @return Next chunk from the stream. Returns the empty string if a chunk
   * isn't ready yet. Users of this interface should repeatedly call this
   * function until an empty string is returned since its entirely possible
   * multiple chunks come in a single raw buffer.
   */
  virtual std::string BufferMessage(std::string& rawBuffer) = 0;

  /***
   * Called to properly buffer an outgoing message.
   *
   * @param rawBuffer Message to format in the correct way
   *
   * @return Formatted message
   */
  virtual std::string BufferOutMessage(const std::string& rawBuffer) const
  {
    return rawBuffer;
  };
  /***
   * Resets the internal state of the buffering
   */
  virtual void clear();

  // TODO: There should be a callback / flag set for errors
};

class cmConnection
{
  CM_DISABLE_COPY(cmConnection)

public:
  cmConnection() {}

  virtual void WriteData(const std::string& data) = 0;

  virtual ~cmConnection();

  virtual bool OnConnectionShuttingDown();

  virtual bool IsOpen() const = 0;

  virtual void SetServer(cmServerBase* s);

  virtual void ProcessRequest(const std::string& request);

  virtual bool OnServeStart(std::string* pString);

protected:
  cmServerBase* Server = nullptr;
};

/***
 * Abstraction of a connection; ties in event callbacks from libuv and notifies
 * the server when appropriate
 */
class cmEventBasedConnection : public cmConnection
{

public:
  /***
   * @param bufferStrategy If no strategy is given, it will process the raw
   * chunks as they come in. The connection
   * owns the pointer given.
   */
  cmEventBasedConnection(cmConnectionBufferStrategy* bufferStrategy = nullptr);

  virtual void Connect(uv_stream_t* server);

  virtual void ReadData(const std::string& data);

  bool IsOpen() const override;

  void WriteData(const std::string& data) override;
  bool OnConnectionShuttingDown() override;

  virtual void OnDisconnect(int errorCode);

  static void on_close(uv_handle_t* handle);

  template <typename T>
  static void on_close_delete(uv_handle_t* handle)
  {
    delete reinterpret_cast<T*>(handle);
  }

protected:
  cm::uv_stream_ptr WriteStream;

  std::string RawReadBuffer;

  std::unique_ptr<cmConnectionBufferStrategy> BufferStrategy;

  static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

  static void on_write(uv_write_t* req, int status);

  static void on_new_connection(uv_stream_t* stream, int status);

  static void on_alloc_buffer(uv_handle_t* handle, size_t suggested_size,
                              uv_buf_t* buf);
};
