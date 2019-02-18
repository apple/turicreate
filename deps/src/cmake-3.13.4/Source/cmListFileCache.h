/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmListFileCache_h
#define cmListFileCache_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <memory> // IWYU pragma: keep
#include <stddef.h>
#include <string>
#include <vector>

#include "cmStateSnapshot.h"

/** \class cmListFileCache
 * \brief A class to cache list file contents.
 *
 * cmListFileCache is a class used to cache the contents of parsed
 * cmake list files.
 */

class cmMessenger;

struct cmCommandContext
{
  struct cmCommandName
  {
    std::string Lower;
    std::string Original;
    cmCommandName() {}
    cmCommandName(std::string const& name) { *this = name; }
    cmCommandName& operator=(std::string const& name);
  } Name;
  long Line;
  cmCommandContext()
    : Line(0)
  {
  }
  cmCommandContext(const char* name, int line)
    : Name(name)
    , Line(line)
  {
  }
};

struct cmListFileArgument
{
  enum Delimiter
  {
    Unquoted,
    Quoted,
    Bracket
  };
  cmListFileArgument()
    : Value()
    , Delim(Unquoted)
    , Line(0)
  {
  }
  cmListFileArgument(const std::string& v, Delimiter d, long line)
    : Value(v)
    , Delim(d)
    , Line(line)
  {
  }
  bool operator==(const cmListFileArgument& r) const
  {
    return (this->Value == r.Value) && (this->Delim == r.Delim);
  }
  bool operator!=(const cmListFileArgument& r) const { return !(*this == r); }
  std::string Value;
  Delimiter Delim;
  long Line;
};

class cmListFileContext
{
public:
  std::string Name;
  std::string FilePath;
  long Line;
  cmListFileContext()
    : Name()
    , FilePath()
    , Line(0)
  {
  }

  static cmListFileContext FromCommandContext(cmCommandContext const& lfcc,
                                              std::string const& fileName)
  {
    cmListFileContext lfc;
    lfc.FilePath = fileName;
    lfc.Line = lfcc.Line;
    lfc.Name = lfcc.Name.Original;
    return lfc;
  }
};

std::ostream& operator<<(std::ostream&, cmListFileContext const&);
bool operator<(const cmListFileContext& lhs, const cmListFileContext& rhs);
bool operator==(cmListFileContext const& lhs, cmListFileContext const& rhs);
bool operator!=(cmListFileContext const& lhs, cmListFileContext const& rhs);

struct cmListFileFunction : public cmCommandContext
{
  std::vector<cmListFileArgument> Arguments;
};

// Represent a backtrace (call stack).  Provide value semantics
// but use efficient reference-counting underneath to avoid copies.
class cmListFileBacktrace
{
public:
  // Default-constructed backtrace may not be used until after
  // set via assignment from a backtrace constructed with a
  // valid snapshot.
  cmListFileBacktrace() = default;

  // Construct an empty backtrace whose bottom sits in the directory
  // indicated by the given valid snapshot.
  cmListFileBacktrace(cmStateSnapshot const& snapshot);

  // Backtraces may be copied, moved, and assigned as values.
  cmListFileBacktrace(cmListFileBacktrace const&) = default;
  cmListFileBacktrace(cmListFileBacktrace&&) // NOLINT(clang-tidy)
    noexcept = default;
  cmListFileBacktrace& operator=(cmListFileBacktrace const&) = default;
  cmListFileBacktrace& operator=(cmListFileBacktrace&&) // NOLINT(clang-tidy)
    noexcept = default;
  ~cmListFileBacktrace() = default;

  cmStateSnapshot GetBottom() const;

  // Get a backtrace with the given file scope added to the top.
  // May not be called until after construction with a valid snapshot.
  cmListFileBacktrace Push(std::string const& file) const;

  // Get a backtrace with the given call context added to the top.
  // May not be called until after construction with a valid snapshot.
  cmListFileBacktrace Push(cmListFileContext const& lfc) const;

  // Get a backtrace with the top level removed.
  // May not be called until after a matching Push.
  cmListFileBacktrace Pop() const;

  // Get the context at the top of the backtrace.
  // This may be called only if Empty() would return false.
  cmListFileContext const& Top() const;

  // Print the top of the backtrace.
  void PrintTitle(std::ostream& out) const;

  // Print the call stack below the top of the backtrace.
  void PrintCallStack(std::ostream& out) const;

  // Get the number of 'frames' in this backtrace
  size_t Depth() const;

  // Return true if this backtrace is empty.
  bool Empty() const;

private:
  struct Entry;
  std::shared_ptr<Entry const> TopEntry;
  cmListFileBacktrace(std::shared_ptr<Entry const> parent,
                      cmListFileContext const& lfc);
  cmListFileBacktrace(std::shared_ptr<Entry const> top);
};

struct cmListFile
{
  bool ParseFile(const char* path, cmMessenger* messenger,
                 cmListFileBacktrace const& lfbt);

  std::vector<cmListFileFunction> Functions;
};

#endif
