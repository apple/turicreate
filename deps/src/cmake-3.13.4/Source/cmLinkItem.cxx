/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLinkItem.h"

#include "cmGeneratorTarget.h"

#include <utility> // IWYU pragma: keep

cmLinkItem::cmLinkItem()
  : String()
  , Target(nullptr)
{
}

cmLinkItem::cmLinkItem(std::string const& n)
  : String(n)
  , Target(nullptr)
{
}

cmLinkItem::cmLinkItem(cmGeneratorTarget const* t)
  : String()
  , Target(t)
{
}

std::string const& cmLinkItem::AsStr() const
{
  return this->Target ? this->Target->GetName() : this->String;
}

bool operator<(cmLinkItem const& l, cmLinkItem const& r)
{
  // Order among targets.
  if (l.Target && r.Target) {
    return l.Target < r.Target;
  }
  // Order targets before strings.
  if (l.Target) {
    return true;
  }
  if (r.Target) {
    return false;
  }
  // Order among strings.
  return l.String < r.String;
}

bool operator==(cmLinkItem const& l, cmLinkItem const& r)
{
  return l.Target == r.Target && l.String == r.String;
}

std::ostream& operator<<(std::ostream& os, cmLinkItem const& item)
{
  return os << item.AsStr();
}

cmLinkImplItem::cmLinkImplItem()
  : cmLinkItem()
  , Backtrace()
  , FromGenex(false)
{
}

cmLinkImplItem::cmLinkImplItem(cmLinkItem item, cmListFileBacktrace const& bt,
                               bool fromGenex)
  : cmLinkItem(std::move(item))
  , Backtrace(bt)
  , FromGenex(fromGenex)
{
}
