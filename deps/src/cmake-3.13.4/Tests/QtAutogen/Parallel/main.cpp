#include "aaa/bbb/item.hpp"
#include "aaa/item.hpp"
#include "bbb/aaa/item.hpp"
#include "bbb/item.hpp"
#include "ccc/item.hpp"

int main(int argv, char** args)
{
  // Object instances
  ::aaa::Item aaa_item;
  ::aaa::bbb::Item aaa_bbb_item;
  ::bbb::Item bbb_item;
  ::bbb::aaa::Item bbb_aaa_item;
  ::ccc::Item ccc_item;
  return 0;
}
