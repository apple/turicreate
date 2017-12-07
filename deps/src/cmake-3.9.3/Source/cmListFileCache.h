/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmListFileCache_h
#define cmListFileCache_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
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
  std::string Name;
  long Line;
  cmCommandContext()
    : Name()
    , Line(0)
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
    lfc.Name = lfcc.Name;
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
  cmListFileBacktrace();

  // Construct an empty backtrace whose bottom sits in the directory
  // indicated by the given valid snapshot.
  cmListFileBacktrace(cmStateSnapshot const& snapshot);

  // Backtraces may be copied and assigned as values.
  cmListFileBacktrace(cmListFileBacktrace const& r);
  cmListFileBacktrace& operator=(cmListFileBacktrace const& r);
  ~cmListFileBacktrace();

  cmStateSnapshot GetBottom() const { return this->Bottom; }

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
  // Returns an empty context if the backtrace is empty.
  cmListFileContext const& Top() const;

  // Print the top of the backtrace.
  void PrintTitle(std::ostream& out) const;

  // Print the call stack below the top of the backtrace.
  void PrintCallStack(std::ostream& out) const;

private:
  struct Entry;

  cmStateSnapshot Bottom;
  Entry* Cur;
  cmListFileBacktrace(cmStateSnapshot const& bottom, Entry* up,
                      cmListFileContext const& lfc);
  cmListFileBacktrace(cmStateSnapshot const& bottom, Entry* cur);
};

struct cmListFile
{
  bool ParseFile(const char* path, cmMessenger* messenger,
                 cmListFileBacktrace const& lfbt);

  std::vector<cmListFileFunction> Functions;
};

#endif
