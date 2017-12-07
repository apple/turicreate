/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmDefinitions_h
#define cmDefinitions_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmLinkedTree.h"
#include "cm_unordered_map.hxx"

/** \class cmDefinitions
 * \brief Store a scope of variable definitions for CMake language.
 *
 * This stores the state of variable definitions (set or unset) for
 * one scope.  Sets are always local.  Gets search parent scopes
 * transitively and save results locally.
 */
class cmDefinitions
{
  typedef cmLinkedTree<cmDefinitions>::iterator StackIter;

public:
  static const char* Get(const std::string& key, StackIter begin,
                         StackIter end);

  static void Raise(const std::string& key, StackIter begin, StackIter end);

  static bool HasKey(const std::string& key, StackIter begin, StackIter end);

  /** Set (or unset if null) a value associated with a key.  */
  void Set(const std::string& key, const char* value);

  std::vector<std::string> UnusedKeys() const;

  static std::vector<std::string> ClosureKeys(StackIter begin, StackIter end);

  static cmDefinitions MakeClosure(StackIter begin, StackIter end);

private:
  // String with existence boolean.
  struct Def : public std::string
  {
  private:
    typedef std::string std_string;

  public:
    Def()
      : std_string()
      , Exists(false)
      , Used(false)
    {
    }
    Def(const char* v)
      : std_string(v ? v : "")
      , Exists(v ? true : false)
      , Used(false)
    {
    }
    Def(const std_string& v)
      : std_string(v)
      , Exists(true)
      , Used(false)
    {
    }
    bool Exists;
    bool Used;
  };
  static Def NoDef;

  typedef CM_UNORDERED_MAP<std::string, Def> MapType;
  MapType Map;

  static Def const& GetInternal(const std::string& key, StackIter begin,
                                StackIter end, bool raise);
};

#endif
