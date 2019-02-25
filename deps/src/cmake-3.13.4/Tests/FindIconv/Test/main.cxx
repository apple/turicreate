extern "C" {
#include <iconv.h>
}
#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <system_error>

class iconv_desc
{
private:
  iconv_t iconvd_;

public:
  iconv_desc(const std::string& tocode, const std::string& fromcode)
  {
    iconvd_ = iconv_open(tocode.c_str(), fromcode.c_str());
    if (iconvd_ == reinterpret_cast<iconv_t>(-1))
      throw std::system_error(errno, std::system_category());
  }

  ~iconv_desc() { iconv_close(iconvd_); }

  operator iconv_t() { return this->iconvd_; }
};

int main()
{
  try {
    auto conv_d = iconv_desc{ "ISO-8859-1", "UTF-8" };
    auto from_str = std::array<char, 10>{ u8"a\xC3\xA4o\xC3\xB6u\xC3\xBC" };
    auto to_str = std::array<char, 7>{};

    auto from_str_ptr = from_str.data();
    auto from_len = from_str.size();
    auto to_str_ptr = to_str.data();
    auto to_len = to_str.size();
    const auto iconv_ret =
      iconv(conv_d, &from_str_ptr, &from_len, &to_str_ptr, &to_len);
    if (iconv_ret == static_cast<std::size_t>(-1))
      throw std::system_error(errno, std::system_category());
    std::cout << '\'' << from_str.data() << "\' converted to \'"
              << to_str.data() << '\'' << std::endl;
    return EXIT_SUCCESS;
  } catch (const std::system_error& ex) {
    std::cerr << "ERROR: " << ex.code() << '\n'
              << ex.code().message() << std::endl;
  }
  return EXIT_FAILURE;
}
