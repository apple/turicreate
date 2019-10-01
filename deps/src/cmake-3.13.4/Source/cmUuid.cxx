/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmUuid.h"

#include "cmCryptoHash.h"

#include <string.h>

cmUuid::cmUuid()
{
  Groups.push_back(4);
  Groups.push_back(2);
  Groups.push_back(2);
  Groups.push_back(2);
  Groups.push_back(6);
}

std::string cmUuid::FromMd5(std::vector<unsigned char> const& uuidNamespace,
                            std::string const& name) const
{
  std::vector<unsigned char> hashInput;
  this->CreateHashInput(uuidNamespace, name, hashInput);

  cmCryptoHash md5(cmCryptoHash::AlgoMD5);
  md5.Initialize();
  md5.Append(&hashInput[0], hashInput.size());
  std::vector<unsigned char> digest = md5.Finalize();

  return this->FromDigest(&digest[0], 3);
}

std::string cmUuid::FromSha1(std::vector<unsigned char> const& uuidNamespace,
                             std::string const& name) const
{
  std::vector<unsigned char> hashInput;
  this->CreateHashInput(uuidNamespace, name, hashInput);

  cmCryptoHash sha1(cmCryptoHash::AlgoSHA1);
  sha1.Initialize();
  sha1.Append(&hashInput[0], hashInput.size());
  std::vector<unsigned char> digest = sha1.Finalize();

  return this->FromDigest(&digest[0], 5);
}

void cmUuid::CreateHashInput(std::vector<unsigned char> const& uuidNamespace,
                             std::string const& name,
                             std::vector<unsigned char>& output) const
{
  output = uuidNamespace;

  if (!name.empty()) {
    output.resize(output.size() + name.size());

    memcpy(&output[0] + uuidNamespace.size(), name.c_str(), name.size());
  }
}

std::string cmUuid::FromDigest(const unsigned char* digest,
                               unsigned char version) const
{
  typedef unsigned char byte_t;

  byte_t uuid[16] = { 0 };
  memcpy(uuid, digest, 16);

  uuid[6] &= 0xF;
  uuid[6] |= byte_t(version << 4);

  uuid[8] &= 0x3F;
  uuid[8] |= 0x80;

  return this->BinaryToString(uuid);
}

bool cmUuid::StringToBinary(std::string const& input,
                            std::vector<unsigned char>& output) const
{
  output.clear();
  output.reserve(16);

  if (input.length() != 36) {
    return false;
  }
  size_t index = 0;
  for (size_t i = 0; i < this->Groups.size(); ++i) {
    if (i != 0 && input[index++] != '-') {
      return false;
    }
    size_t digits = this->Groups[i] * 2;
    if (!StringToBinaryImpl(input.substr(index, digits), output)) {
      return false;
    }

    index += digits;
  }

  return true;
}

std::string cmUuid::BinaryToString(const unsigned char* input) const
{
  std::string output;

  size_t inputIndex = 0;
  for (size_t i = 0; i < this->Groups.size(); ++i) {
    if (i != 0) {
      output += '-';
    }

    size_t bytes = this->Groups[i];
    for (size_t j = 0; j < bytes; ++j) {
      unsigned char byte = input[inputIndex++];
      output += this->ByteToHex(byte);
    }
  }

  return output;
}

std::string cmUuid::ByteToHex(unsigned char byte) const
{
  std::string result;
  for (int i = 0; i < 2; ++i) {
    unsigned char rest = byte % 16;
    byte /= 16;

    char c = (rest < 0xA) ? char('0' + rest) : char('a' + (rest - 0xA));

    result = c + result;
  }

  return result;
}

bool cmUuid::StringToBinaryImpl(std::string const& input,
                                std::vector<unsigned char>& output) const
{
  if (input.size() % 2) {
    return false;
  }

  for (size_t i = 0; i < input.size(); i += 2) {
    char c1 = 0;
    if (!IntFromHexDigit(input[i], c1)) {
      return false;
    }

    char c2 = 0;
    if (!IntFromHexDigit(input[i + 1], c2)) {
      return false;
    }

    output.push_back(char(c1 << 4 | c2));
  }

  return true;
}

bool cmUuid::IntFromHexDigit(char input, char& output) const
{
  if (input >= '0' && input <= '9') {
    output = char(input - '0');
    return true;
  }
  if (input >= 'a' && input <= 'f') {
    output = char(input - 'a' + 0xA);
    return true;
  }
  if (input >= 'A' && input <= 'F') {
    output = char(input - 'A' + 0xA);
    return true;
  }
  return false;
}
