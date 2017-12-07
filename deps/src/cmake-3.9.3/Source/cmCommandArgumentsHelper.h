/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCommandArgumentsHelper_h
#define cmCommandArgumentsHelper_h

#include "cmConfigure.h"

#include <set>
#include <string>
#include <vector>

class cmCommandArgumentGroup;
class cmCommandArgumentsHelper;

/* cmCommandArgumentsHelper, cmCommandArgumentGroup and cmCommandArgument (i.e.
its derived classes cmCAXXX can be used to simplify the processing of
arguments to cmake commands. Maybe they can also be used to generate
documentation.

For every argument supported by a command one cmCommandArgument is created
and added to cmCommandArgumentsHelper. cmCommand has a cmCommandArgumentsHelper
as member variable so this should be used.

The order of the arguments is defined using the Follows(arg) method. It says
that this argument follows immediateley the given argument. It can be used
with multiple arguments if the argument can follow after different arguments.

Arguments can be arranged in groups using cmCommandArgumentGroup. Every
member of a group can follow any other member of the group. These groups
can also be used to define the order.

Once all arguments and groups are set up, cmCommandArgumentsHelper::Parse()
is called and afterwards the values of the arguments can be evaluated.

For an example see cmExportCommand.cxx.
*/
class cmCommandArgument
{
public:
  cmCommandArgument(cmCommandArgumentsHelper* args, const char* key,
                    cmCommandArgumentGroup* group = CM_NULLPTR);
  virtual ~cmCommandArgument() {}

  /// this argument may follow after arg. 0 means it comes first.
  void Follows(const cmCommandArgument* arg);

  /// this argument may follow after any of the arguments in the given group
  void FollowsGroup(const cmCommandArgumentGroup* group);

  /// Returns true if the argument was found in the argument list
  bool WasFound() const { return this->WasActive; }

  // The following methods are only called from
  // cmCommandArgumentsHelper::Parse(), but making this a friend would
  // give it access to everything

  /// Make the current argument the currently active argument
  void Activate();
  /// Consume the current string
  bool Consume(const std::string& arg);

  /// Return true if this argument may follow after the given argument.
  bool MayFollow(const cmCommandArgument* current) const;

  /** Returns true if the given key matches the key for this argument.
  If this argument has an empty key everything matches. */
  bool KeyMatches(const std::string& key) const;

  /// Make this argument follow all members of the own group
  void ApplyOwnGroup();

  /// Reset argument, so it's back to its initial state
  void Reset();

private:
  const char* Key;
  std::set<const cmCommandArgument*> ArgumentsBefore;
  cmCommandArgumentGroup* Group;
  bool WasActive;
  bool ArgumentsBeforeEmpty;
  unsigned int CurrentIndex;

  virtual bool DoConsume(const std::string& arg, unsigned int index) = 0;
  virtual void DoReset() = 0;
};

/** cmCAStringVector is to be used for arguments which can consist of more
than one string, e.g. the FILES argument in INSTALL(FILES f1 f2 f3 ...). */
class cmCAStringVector : public cmCommandArgument
{
public:
  cmCAStringVector(cmCommandArgumentsHelper* args, const char* key,
                   cmCommandArgumentGroup* group = CM_NULLPTR);

  /// Return the vector of strings
  const std::vector<std::string>& GetVector() const { return this->Vector; }

  /** Is there a keyword which should be skipped in
  the arguments (e.g. ARGS for ADD_CUSTOM_COMMAND) ? */
  void SetIgnore(const char* ignore) { this->Ignore = ignore; }
private:
  std::vector<std::string> Vector;
  unsigned int DataStart;
  const char* Ignore;
  cmCAStringVector();
  bool DoConsume(const std::string& arg, unsigned int index) CM_OVERRIDE;
  void DoReset() CM_OVERRIDE;
};

/** cmCAString is to be used for arguments which consist of one value,
e.g. the executable name in ADD_EXECUTABLE(). */
class cmCAString : public cmCommandArgument
{
public:
  cmCAString(cmCommandArgumentsHelper* args, const char* key,
             cmCommandArgumentGroup* group = CM_NULLPTR);

  /// Return the string
  const std::string& GetString() const { return this->String; }
  const char* GetCString() const { return this->String.c_str(); }
private:
  std::string String;
  unsigned int DataStart;
  bool DoConsume(const std::string& arg, unsigned int index) CM_OVERRIDE;
  void DoReset() CM_OVERRIDE;
  cmCAString();
};

/** cmCAEnabler is to be used for options which are off by default and can be
enabled using a special argument, e.g. EXCLUDE_FROM_ALL in ADD_EXECUTABLE(). */
class cmCAEnabler : public cmCommandArgument
{
public:
  cmCAEnabler(cmCommandArgumentsHelper* args, const char* key,
              cmCommandArgumentGroup* group = CM_NULLPTR);

  /// Has it been enabled ?
  bool IsEnabled() const { return this->Enabled; }
private:
  bool Enabled;
  bool DoConsume(const std::string& arg, unsigned int index) CM_OVERRIDE;
  void DoReset() CM_OVERRIDE;
  cmCAEnabler();
};

/** cmCADisable is to be used for options which are on by default and can be
disabled using a special argument.*/
class cmCADisabler : public cmCommandArgument
{
public:
  cmCADisabler(cmCommandArgumentsHelper* args, const char* key,
               cmCommandArgumentGroup* group = CM_NULLPTR);

  /// Is it still enabled ?
  bool IsEnabled() const { return this->Enabled; }
private:
  bool Enabled;
  bool DoConsume(const std::string& arg, unsigned int index) CM_OVERRIDE;
  void DoReset() CM_OVERRIDE;
  cmCADisabler();
};

/** Group of arguments, needed for ordering. E.g. WIN32, EXCLUDE_FROM_ALL and
MACSOX_BUNDLE from ADD_EXECUTABLE() are a group.
*/
class cmCommandArgumentGroup
{
  friend class cmCommandArgument;

public:
  cmCommandArgumentGroup() {}

  /// All members of this group may follow the given argument
  void Follows(const cmCommandArgument* arg);

  /// All members of this group may follow all members of the given group
  void FollowsGroup(const cmCommandArgumentGroup* group);

private:
  std::vector<cmCommandArgument*> ContainedArguments;
};

class cmCommandArgumentsHelper
{
public:
  /// Parse the argument list
  void Parse(const std::vector<std::string>* args,
             std::vector<std::string>* unconsumedArgs);
  /// Add an argument.
  void AddArgument(cmCommandArgument* arg);

private:
  std::vector<cmCommandArgument*> Arguments;
};

#endif
