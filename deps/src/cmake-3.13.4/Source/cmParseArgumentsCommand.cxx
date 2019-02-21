/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmParseArgumentsCommand.h"

#include <map>
#include <set>
#include <sstream>
#include <utility>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

static std::string escape_arg(const std::string& arg)
{
  // replace ";" with "\;" so output argument lists will split correctly
  std::string escapedArg;
  for (char i : arg) {
    if (i == ';') {
      escapedArg += '\\';
    }
    escapedArg += i;
  }
  return escapedArg;
}

bool cmParseArgumentsCommand::InitialPass(std::vector<std::string> const& args,
                                          cmExecutionStatus&)
{
  // cmake_parse_arguments(prefix options single multi <ARGN>)
  //                         1       2      3      4
  // or
  // cmake_parse_arguments(PARSE_ARGV N prefix options single multi)
  if (args.size() < 4) {
    this->SetError("must be called with at least 4 arguments.");
    return false;
  }

  std::vector<std::string>::const_iterator argIter = args.begin(),
                                           argEnd = args.end();
  bool parseFromArgV = false;
  unsigned long argvStart = 0;
  if (*argIter == "PARSE_ARGV") {
    if (args.size() != 6) {
      this->Makefile->IssueMessage(
        cmake::FATAL_ERROR,
        "PARSE_ARGV must be called with exactly 6 arguments.");
      cmSystemTools::SetFatalErrorOccured();
      return true;
    }
    parseFromArgV = true;
    argIter++; // move past PARSE_ARGV
    if (!cmSystemTools::StringToULong(argIter->c_str(), &argvStart)) {
      this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                   "PARSE_ARGV index '" + *argIter +
                                     "' is not an unsigned integer");
      cmSystemTools::SetFatalErrorOccured();
      return true;
    }
    argIter++; // move past N
  }
  // the first argument is the prefix
  const std::string prefix = (*argIter++) + "_";

  // define the result maps holding key/value pairs for
  // options, single values and multi values
  typedef std::map<std::string, bool> options_map;
  typedef std::map<std::string, std::string> single_map;
  typedef std::map<std::string, std::vector<std::string>> multi_map;
  options_map options;
  single_map singleValArgs;
  multi_map multiValArgs;

  // anything else is put into a vector of unparsed strings
  std::vector<std::string> unparsed;

  // remember already defined keywords
  std::set<std::string> used_keywords;
  const std::string dup_warning = "keyword defined more than once: ";

  // the second argument is a (cmake) list of options without argument
  std::vector<std::string> list;
  cmSystemTools::ExpandListArgument(*argIter++, list);
  for (std::string const& iter : list) {
    if (!used_keywords.insert(iter).second) {
      this->GetMakefile()->IssueMessage(cmake::WARNING, dup_warning + iter);
    }
    options[iter]; // default initialize
  }

  // the third argument is a (cmake) list of single argument options
  list.clear();
  cmSystemTools::ExpandListArgument(*argIter++, list);
  for (std::string const& iter : list) {
    if (!used_keywords.insert(iter).second) {
      this->GetMakefile()->IssueMessage(cmake::WARNING, dup_warning + iter);
    }
    singleValArgs[iter]; // default initialize
  }

  // the fourth argument is a (cmake) list of multi argument options
  list.clear();
  cmSystemTools::ExpandListArgument(*argIter++, list);
  for (std::string const& iter : list) {
    if (!used_keywords.insert(iter).second) {
      this->GetMakefile()->IssueMessage(cmake::WARNING, dup_warning + iter);
    }
    multiValArgs[iter]; // default initialize
  }

  enum insideValues
  {
    NONE,
    SINGLE,
    MULTI
  } insideValues = NONE;
  std::string currentArgName;

  list.clear();
  if (!parseFromArgV) {
    // Flatten ;-lists in the arguments into a single list as was done
    // by the original function(CMAKE_PARSE_ARGUMENTS).
    for (; argIter != argEnd; ++argIter) {
      cmSystemTools::ExpandListArgument(*argIter, list);
    }
  } else {
    // in the PARSE_ARGV move read the arguments from ARGC and ARGV#
    std::string argc = this->Makefile->GetSafeDefinition("ARGC");
    unsigned long count;
    if (!cmSystemTools::StringToULong(argc.c_str(), &count)) {
      this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                   "PARSE_ARGV called with ARGC='" + argc +
                                     "' that is not an unsigned integer");
      cmSystemTools::SetFatalErrorOccured();
      return true;
    }
    for (unsigned long i = argvStart; i < count; ++i) {
      std::ostringstream argName;
      argName << "ARGV" << i;
      const char* arg = this->Makefile->GetDefinition(argName.str());
      if (!arg) {
        this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                     "PARSE_ARGV called with " +
                                       argName.str() + " not set");
        cmSystemTools::SetFatalErrorOccured();
        return true;
      }
      list.push_back(arg);
    }
  }

  // iterate over the arguments list and fill in the values where applicable
  for (std::string const& arg : list) {
    const options_map::iterator optIter = options.find(arg);
    if (optIter != options.end()) {
      insideValues = NONE;
      optIter->second = true;
      continue;
    }

    const single_map::iterator singleIter = singleValArgs.find(arg);
    if (singleIter != singleValArgs.end()) {
      insideValues = SINGLE;
      currentArgName = arg;
      continue;
    }

    const multi_map::iterator multiIter = multiValArgs.find(arg);
    if (multiIter != multiValArgs.end()) {
      insideValues = MULTI;
      currentArgName = arg;
      continue;
    }

    switch (insideValues) {
      case SINGLE:
        singleValArgs[currentArgName] = arg;
        insideValues = NONE;
        break;
      case MULTI:
        if (parseFromArgV) {
          multiValArgs[currentArgName].push_back(escape_arg(arg));
        } else {
          multiValArgs[currentArgName].push_back(arg);
        }
        break;
      default:
        if (parseFromArgV) {
          unparsed.push_back(escape_arg(arg));
        } else {
          unparsed.push_back(arg);
        }
        break;
    }
  }

  // now iterate over the collected values and update their definition
  // within the current scope. undefine if necessary.

  for (auto const& iter : options) {
    this->Makefile->AddDefinition(prefix + iter.first,
                                  iter.second ? "TRUE" : "FALSE");
  }
  for (auto const& iter : singleValArgs) {
    if (!iter.second.empty()) {
      this->Makefile->AddDefinition(prefix + iter.first, iter.second.c_str());
    } else {
      this->Makefile->RemoveDefinition(prefix + iter.first);
    }
  }

  for (auto const& iter : multiValArgs) {
    if (!iter.second.empty()) {
      this->Makefile->AddDefinition(
        prefix + iter.first, cmJoin(cmMakeRange(iter.second), ";").c_str());
    } else {
      this->Makefile->RemoveDefinition(prefix + iter.first);
    }
  }

  if (!unparsed.empty()) {
    this->Makefile->AddDefinition(prefix + "UNPARSED_ARGUMENTS",
                                  cmJoin(cmMakeRange(unparsed), ";").c_str());
  } else {
    this->Makefile->RemoveDefinition(prefix + "UNPARSED_ARGUMENTS");
  }

  return true;
}
