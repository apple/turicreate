/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCustomCommand_h
#define cmCustomCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCustomCommandLines.h"
#include "cmListFileCache.h"

#include <string>
#include <utility>
#include <vector>

class cmMakefile;

/** \class cmCustomCommand
 * \brief A class to encapsulate a custom command
 *
 * cmCustomCommand encapsulates the properties of a custom command
 */
class cmCustomCommand
{
public:
  /** Default and copy constructors for STL containers.  */
  cmCustomCommand();

  /** Main constructor specifies all information for the command.  */
  cmCustomCommand(cmMakefile const* mf,
                  const std::vector<std::string>& outputs,
                  const std::vector<std::string>& byproducts,
                  const std::vector<std::string>& depends,
                  const cmCustomCommandLines& commandLines,
                  const char* comment, const char* workingDirectory);

  /** Get the output file produced by the command.  */
  const std::vector<std::string>& GetOutputs() const;

  /** Get the extra files produced by the command.  */
  const std::vector<std::string>& GetByproducts() const;

  /** Get the vector that holds the list of dependencies.  */
  const std::vector<std::string>& GetDepends() const;

  /** Get the working directory.  */
  std::string const& GetWorkingDirectory() const
  {
    return this->WorkingDirectory;
  }

  /** Get the list of command lines.  */
  const cmCustomCommandLines& GetCommandLines() const;

  /** Get the comment string for the command.  */
  const char* GetComment() const;

  /** Append to the list of command lines.  */
  void AppendCommands(const cmCustomCommandLines& commandLines);

  /** Append to the list of dependencies.  */
  void AppendDepends(const std::vector<std::string>& depends);

  /** Set/Get whether old-style escaping should be used.  */
  bool GetEscapeOldStyle() const;
  void SetEscapeOldStyle(bool b);

  /** Set/Get whether the build tool can replace variables in
      arguments to the command.  */
  bool GetEscapeAllowMakeVars() const;
  void SetEscapeAllowMakeVars(bool b);

  /** Backtrace of the command that created this custom command.  */
  cmListFileBacktrace const& GetBacktrace() const;

  typedef std::pair<std::string, std::string> ImplicitDependsPair;
  class ImplicitDependsList : public std::vector<ImplicitDependsPair>
  {
  };
  void SetImplicitDepends(ImplicitDependsList const&);
  void AppendImplicitDepends(ImplicitDependsList const&);
  ImplicitDependsList const& GetImplicitDepends() const;

  /** Set/Get whether this custom command should be given access to the
      real console (if possible).  */
  bool GetUsesTerminal() const;
  void SetUsesTerminal(bool b);

  /** Set/Get whether lists in command lines should be expanded. */
  bool GetCommandExpandLists() const;
  void SetCommandExpandLists(bool b);

  /** Set/Get the depfile (used by the Ninja generator) */
  const std::string& GetDepfile() const;
  void SetDepfile(const std::string& depfile);

private:
  std::vector<std::string> Outputs;
  std::vector<std::string> Byproducts;
  std::vector<std::string> Depends;
  cmCustomCommandLines CommandLines;
  cmListFileBacktrace Backtrace;
  ImplicitDependsList ImplicitDepends;
  std::string Comment;
  std::string WorkingDirectory;
  std::string Depfile;
  bool HaveComment;
  bool EscapeAllowMakeVars;
  bool EscapeOldStyle;
  bool UsesTerminal;
  bool CommandExpandLists;
};

#endif
