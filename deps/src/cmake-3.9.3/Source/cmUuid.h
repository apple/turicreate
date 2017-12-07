/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmUuid_h
#define cmUuid_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

/** \class cmUuid
 * \brief Utility class to generate UUIDs as defined by RFC4122
 *
 */
class cmUuid
{
public:
  cmUuid();

  std::string FromMd5(std::vector<unsigned char> const& uuidNamespace,
                      std::string const& name) const;

  std::string FromSha1(std::vector<unsigned char> const& uuidNamespace,
                       std::string const& name) const;

  bool StringToBinary(std::string const& input,
                      std::vector<unsigned char>& output) const;

private:
  std::string ByteToHex(unsigned char byte) const;

  void CreateHashInput(std::vector<unsigned char> const& uuidNamespace,
                       std::string const& name,
                       std::vector<unsigned char>& output) const;

  std::string FromDigest(const unsigned char* digest,
                         unsigned char version) const;

  bool StringToBinaryImpl(std::string const& input,
                          std::vector<unsigned char>& output) const;

  std::string BinaryToString(const unsigned char* input) const;

  bool IntFromHexDigit(char input, char& output) const;

  std::vector<int> Groups;
};

#endif
