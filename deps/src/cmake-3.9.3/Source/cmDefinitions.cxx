/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmDefinitions.h"

#include "cmConfigure.h"
#include <assert.h>
#include <set>
#include <utility>

cmDefinitions::Def cmDefinitions::NoDef;

cmDefinitions::Def const& cmDefinitions::GetInternal(const std::string& key,
                                                     StackIter begin,
                                                     StackIter end, bool raise)
{
  assert(begin != end);
  MapType::iterator i = begin->Map.find(key);
  if (i != begin->Map.end()) {
    i->second.Used = true;
    return i->second;
  }
  StackIter it = begin;
  ++it;
  if (it == end) {
    return cmDefinitions::NoDef;
  }
  Def const& def = cmDefinitions::GetInternal(key, it, end, raise);
  if (!raise) {
    return def;
  }
  return begin->Map.insert(MapType::value_type(key, def)).first->second;
}

const char* cmDefinitions::Get(const std::string& key, StackIter begin,
                               StackIter end)
{
  Def const& def = cmDefinitions::GetInternal(key, begin, end, false);
  return def.Exists ? def.c_str() : CM_NULLPTR;
}

void cmDefinitions::Raise(const std::string& key, StackIter begin,
                          StackIter end)
{
  cmDefinitions::GetInternal(key, begin, end, true);
}

bool cmDefinitions::HasKey(const std::string& key, StackIter begin,
                           StackIter end)
{
  for (StackIter it = begin; it != end; ++it) {
    MapType::const_iterator i = it->Map.find(key);
    if (i != it->Map.end()) {
      return true;
    }
  }
  return false;
}

void cmDefinitions::Set(const std::string& key, const char* value)
{
  Def def(value);
  this->Map[key] = def;
}

std::vector<std::string> cmDefinitions::UnusedKeys() const
{
  std::vector<std::string> keys;
  keys.reserve(this->Map.size());
  // Consider local definitions.
  for (MapType::const_iterator mi = this->Map.begin(); mi != this->Map.end();
       ++mi) {
    if (!mi->second.Used) {
      keys.push_back(mi->first);
    }
  }
  return keys;
}

cmDefinitions cmDefinitions::MakeClosure(StackIter begin, StackIter end)
{
  cmDefinitions closure;
  std::set<std::string> undefined;
  for (StackIter it = begin; it != end; ++it) {
    // Consider local definitions.
    for (MapType::const_iterator mi = it->Map.begin(); mi != it->Map.end();
         ++mi) {
      // Use this key if it is not already set or unset.
      if (closure.Map.find(mi->first) == closure.Map.end() &&
          undefined.find(mi->first) == undefined.end()) {
        if (mi->second.Exists) {
          closure.Map.insert(*mi);
        } else {
          undefined.insert(mi->first);
        }
      }
    }
  }
  return closure;
}

std::vector<std::string> cmDefinitions::ClosureKeys(StackIter begin,
                                                    StackIter end)
{
  std::set<std::string> bound;
  std::vector<std::string> defined;

  for (StackIter it = begin; it != end; ++it) {
    defined.reserve(defined.size() + it->Map.size());
    for (MapType::const_iterator mi = it->Map.begin(); mi != it->Map.end();
         ++mi) {
      // Use this key if it is not already set or unset.
      if (bound.insert(mi->first).second && mi->second.Exists) {
        defined.push_back(mi->first);
      }
    }
  }

  return defined;
}
