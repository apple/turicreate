#include "cmConnection.h"
#include "cmServerConnection.h"
#include <iostream>
#include <string>
#include <vector>

void print_error(const std::vector<std::string>& input,
                 const std::vector<std::string>& output)
{
  std::cerr << "Responses don't equal input messages input." << std::endl;
  std::cerr << "Responses: " << std::endl;

  for (auto& msg : output) {
    std::cerr << "'" << msg << "'" << std::endl;
  }

  std::cerr << "Input messages" << std::endl;
  for (auto& msg : input) {
    std::cerr << "'" << msg << "'" << std::endl;
  }
}

std::string trim_newline(const std::string& _buffer)
{
  auto buffer = _buffer;
  while (!buffer.empty() && (buffer.back() == '\n' || buffer.back() == '\r')) {
    buffer.pop_back();
  }
  return buffer;
}

int testServerBuffering(int, char** const)
{
  std::vector<std::string> messages = {
    "{ \"test\": 10}", "{ \"test\": { \"test2\": false} }",
    "{ \"test\": [1, 2, 3] }",
    "{ \"a\": { \"1\": {}, \n\n\n \"2\":[] \t\t\t\t}}"
  };

  std::string fullMessage;
  for (auto& msg : messages) {
    fullMessage += "[== \"CMake Server\" ==[\n";
    fullMessage += msg;
    fullMessage += "\n]== \"CMake Server\" ==]\n";
  }

  // The buffering strategy should cope with any fragmentation, including
  // just getting the characters one at a time.
  auto bufferingStrategy =
    std::unique_ptr<cmConnectionBufferStrategy>(new cmServerBufferStrategy);
  std::vector<std::string> response;
  std::string rawBuffer;
  for (auto& messageChar : fullMessage) {
    rawBuffer += messageChar;
    std::string packet = bufferingStrategy->BufferMessage(rawBuffer);
    do {
      if (!packet.empty() && packet != "\r\n") {
        response.push_back(trim_newline(packet));
      }
      packet = bufferingStrategy->BufferMessage(rawBuffer);
    } while (!packet.empty());
  }

  if (response != messages) {
    print_error(messages, response);
    return 1;
  }

  // We should also be able to deal with getting a bunch at once
  response.clear();
  std::string packet = bufferingStrategy->BufferMessage(fullMessage);
  do {
    if (!packet.empty() && packet != "\r\n") {
      response.push_back(trim_newline(packet));
    }
    packet = bufferingStrategy->BufferMessage(fullMessage);
  } while (!packet.empty());

  if (response != messages) {
    print_error(messages, response);
    return 1;
  }

  return 0;
}
