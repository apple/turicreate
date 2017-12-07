/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCryptoHash.h"

#include "cm_kwiml.h"
#include "cm_rhash.h"
#include "cmsys/FStream.hxx"
#include <string.h>

static unsigned int const cmCryptoHashAlgoToId[] = {
  /* clang-format needs this comment to break after the opening brace */
  RHASH_MD5,      //
  RHASH_SHA1,     //
  RHASH_SHA224,   //
  RHASH_SHA256,   //
  RHASH_SHA384,   //
  RHASH_SHA512,   //
  RHASH_SHA3_224, //
  RHASH_SHA3_256, //
  RHASH_SHA3_384, //
  RHASH_SHA3_512
};

static int cmCryptoHash_rhash_library_initialized;

static rhash cmCryptoHash_rhash_init(unsigned int id)
{
  if (!cmCryptoHash_rhash_library_initialized) {
    cmCryptoHash_rhash_library_initialized = 1;
    rhash_library_init();
  }
  return rhash_init(id);
}

cmCryptoHash::cmCryptoHash(Algo algo)
  : Id(cmCryptoHashAlgoToId[algo])
  , CTX(cmCryptoHash_rhash_init(Id))
{
}

cmCryptoHash::~cmCryptoHash()
{
  rhash_free(this->CTX);
}

CM_AUTO_PTR<cmCryptoHash> cmCryptoHash::New(const char* algo)
{
  if (strcmp(algo, "MD5") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoMD5));
  }
  if (strcmp(algo, "SHA1") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA1));
  }
  if (strcmp(algo, "SHA224") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA224));
  }
  if (strcmp(algo, "SHA256") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA256));
  }
  if (strcmp(algo, "SHA384") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA384));
  }
  if (strcmp(algo, "SHA512") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA512));
  }
  if (strcmp(algo, "SHA3_224") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA3_224));
  }
  if (strcmp(algo, "SHA3_256") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA3_256));
  }
  if (strcmp(algo, "SHA3_384") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA3_384));
  }
  if (strcmp(algo, "SHA3_512") == 0) {
    return CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(AlgoSHA3_512));
  }
  return CM_AUTO_PTR<cmCryptoHash>(CM_NULLPTR);
}

bool cmCryptoHash::IntFromHexDigit(char input, char& output)
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

std::string cmCryptoHash::ByteHashToString(
  const std::vector<unsigned char>& hash)
{
  // Map from 4-bit index to hexadecimal representation.
  static char const hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

  std::string res;
  for (std::vector<unsigned char>::const_iterator vit = hash.begin();
       vit != hash.end(); ++vit) {
    res.push_back(hex[(*vit) >> 4]);
    res.push_back(hex[(*vit) & 0xF]);
  }
  return res;
}

std::vector<unsigned char> cmCryptoHash::ByteHashString(
  const std::string& input)
{
  this->Initialize();
  this->Append(reinterpret_cast<unsigned char const*>(input.c_str()),
               static_cast<int>(input.size()));
  return this->Finalize();
}

std::vector<unsigned char> cmCryptoHash::ByteHashFile(const std::string& file)
{
  cmsys::ifstream fin(file.c_str(), std::ios::in | std::ios::binary);
  if (fin) {
    this->Initialize();
    {
      // Should be efficient enough on most system:
      KWIML_INT_uint64_t buffer[512];
      char* buffer_c = reinterpret_cast<char*>(buffer);
      unsigned char const* buffer_uc =
        reinterpret_cast<unsigned char const*>(buffer);
      // This copy loop is very sensitive on certain platforms with
      // slightly broken stream libraries (like HPUX).  Normally, it is
      // incorrect to not check the error condition on the fin.read()
      // before using the data, but the fin.gcount() will be zero if an
      // error occurred.  Therefore, the loop should be safe everywhere.
      while (fin) {
        fin.read(buffer_c, sizeof(buffer));
        if (int gcount = static_cast<int>(fin.gcount())) {
          this->Append(buffer_uc, gcount);
        }
      }
    }
    if (fin.eof()) {
      // Success
      return this->Finalize();
    }
    // Finalize anyway
    this->Finalize();
  }
  // Return without success
  return std::vector<unsigned char>();
}

std::string cmCryptoHash::HashString(const std::string& input)
{
  return ByteHashToString(this->ByteHashString(input));
}

std::string cmCryptoHash::HashFile(const std::string& file)
{
  return ByteHashToString(this->ByteHashFile(file));
}

void cmCryptoHash::Initialize()
{
  rhash_reset(this->CTX);
}

void cmCryptoHash::Append(void const* buf, size_t sz)
{
  rhash_update(this->CTX, buf, sz);
}

void cmCryptoHash::Append(std::string const& str)
{
  this->Append(str.c_str(), str.size());
}

std::vector<unsigned char> cmCryptoHash::Finalize()
{
  std::vector<unsigned char> hash(rhash_get_digest_size(this->Id), 0);
  rhash_final(this->CTX, &hash[0]);
  return hash;
}

std::string cmCryptoHash::FinalizeHex()
{
  return cmCryptoHash::ByteHashToString(this->Finalize());
}
