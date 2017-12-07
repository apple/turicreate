/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmIncludeDirectoryCommand.h"

#include <algorithm>
#include <set>

#include "cmMakefile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmIncludeDirectoryCommand
bool cmIncludeDirectoryCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.empty()) {
    return true;
  }

  std::vector<std::string>::const_iterator i = args.begin();

  bool before = this->Makefile->IsOn("CMAKE_INCLUDE_DIRECTORIES_BEFORE");
  bool system = false;

  if ((*i) == "BEFORE") {
    before = true;
    ++i;
  } else if ((*i) == "AFTER") {
    before = false;
    ++i;
  }

  std::vector<std::string> beforeIncludes;
  std::vector<std::string> afterIncludes;
  std::set<std::string> systemIncludes;

  for (; i != args.end(); ++i) {
    if (*i == "SYSTEM") {
      system = true;
      continue;
    }
    if (i->empty()) {
      this->SetError("given empty-string as include directory.");
      return false;
    }

    std::vector<std::string> includes;

    this->GetIncludes(*i, includes);

    if (before) {
      beforeIncludes.insert(beforeIncludes.end(), includes.begin(),
                            includes.end());
    } else {
      afterIncludes.insert(afterIncludes.end(), includes.begin(),
                           includes.end());
    }
    if (system) {
      systemIncludes.insert(includes.begin(), includes.end());
    }
  }
  std::reverse(beforeIncludes.begin(), beforeIncludes.end());

  this->Makefile->AddIncludeDirectories(afterIncludes);
  this->Makefile->AddIncludeDirectories(beforeIncludes, before);
  this->Makefile->AddSystemIncludeDirectories(systemIncludes);

  return true;
}

static bool StartsWithGeneratorExpression(const std::string& input)
{
  return input[0] == '$' && input[1] == '<';
}

// do a lot of cleanup on the arguments because this is one place where folks
// sometimes take the output of a program and pass it directly into this
// command not thinking that a single argument could be filled with spaces
// and newlines etc liek below:
//
// "   /foo/bar
//    /boo/hoo /dingle/berry "
//
// ideally that should be three separate arguments but when sucking the
// output from a program and passing it into a command the cleanup doesn't
// always happen
//
void cmIncludeDirectoryCommand::GetIncludes(const std::string& arg,
                                            std::vector<std::string>& incs)
{
  // break apart any line feed arguments
  std::string::size_type pos = 0;
  std::string::size_type lastPos = 0;
  while ((pos = arg.find('\n', lastPos)) != std::string::npos) {
    if (pos) {
      std::string inc = arg.substr(lastPos, pos);
      this->NormalizeInclude(inc);
      if (!inc.empty()) {
        incs.push_back(inc);
      }
    }
    lastPos = pos + 1;
  }
  std::string inc = arg.substr(lastPos);
  this->NormalizeInclude(inc);
  if (!inc.empty()) {
    incs.push_back(inc);
  }
}

void cmIncludeDirectoryCommand::NormalizeInclude(std::string& inc)
{
  std::string::size_type b = inc.find_first_not_of(" \r");
  std::string::size_type e = inc.find_last_not_of(" \r");
  if ((b != std::string::npos) && (e != std::string::npos)) {
    inc.assign(inc, b, 1 + e - b); // copy the remaining substring
  } else {
    inc = "";
    return;
  }

  if (!cmSystemTools::IsOff(inc.c_str())) {
    cmSystemTools::ConvertToUnixSlashes(inc);

    if (!cmSystemTools::FileIsFullPath(inc.c_str())) {
      if (!StartsWithGeneratorExpression(inc)) {
        std::string tmp = this->Makefile->GetCurrentSourceDirectory();
        tmp += "/";
        tmp += inc;
        inc = tmp;
      }
    }
  }
}
