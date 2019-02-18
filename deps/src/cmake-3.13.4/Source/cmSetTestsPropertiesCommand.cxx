/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSetTestsPropertiesCommand.h"

#include <iterator>

#include "cmMakefile.h"
#include "cmTest.h"

class cmExecutionStatus;

// cmSetTestsPropertiesCommand
bool cmSetTestsPropertiesCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // first collect up the list of files
  std::vector<std::string> propertyPairs;
  int numFiles = 0;
  std::vector<std::string>::const_iterator j;
  for (j = args.begin(); j != args.end(); ++j) {
    if (*j == "PROPERTIES") {
      // now loop through the rest of the arguments, new style
      ++j;
      if (std::distance(j, args.end()) % 2 != 0) {
        this->SetError("called with incorrect number of arguments.");
        return false;
      }
      propertyPairs.insert(propertyPairs.end(), j, args.end());
      break;
    }
    numFiles++;
  }
  if (propertyPairs.empty()) {
    this->SetError("called with illegal arguments, maybe "
                   "missing a PROPERTIES specifier?");
    return false;
  }

  // now loop over all the targets
  int i;
  for (i = 0; i < numFiles; ++i) {
    std::string errors;
    bool ret = cmSetTestsPropertiesCommand::SetOneTest(args[i], propertyPairs,
                                                       this->Makefile, errors);
    if (!ret) {
      this->SetError(errors);
      return ret;
    }
  }

  return true;
}

bool cmSetTestsPropertiesCommand::SetOneTest(
  const std::string& tname, std::vector<std::string>& propertyPairs,
  cmMakefile* mf, std::string& errors)
{
  if (cmTest* test = mf->GetTest(tname)) {
    // now loop through all the props and set them
    unsigned int k;
    for (k = 0; k < propertyPairs.size(); k = k + 2) {
      if (!propertyPairs[k].empty()) {
        test->SetProperty(propertyPairs[k], propertyPairs[k + 1].c_str());
      }
    }
  } else {
    errors = "Can not find test to add properties to: ";
    errors += tname;
    return false;
  }

  return true;
}
