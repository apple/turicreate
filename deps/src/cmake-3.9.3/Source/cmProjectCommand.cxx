/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmProjectCommand.h"

#include "cmsys/RegularExpression.hxx"
#include <sstream>
#include <stdio.h>

#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

// cmProjectCommand
bool cmProjectCommand::InitialPass(std::vector<std::string> const& args,
                                   cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("PROJECT called with incorrect number of arguments");
    return false;
  }

  std::string const& projectName = args[0];

  this->Makefile->SetProjectName(projectName);

  std::string bindir = projectName;
  bindir += "_BINARY_DIR";
  std::string srcdir = projectName;
  srcdir += "_SOURCE_DIR";

  this->Makefile->AddCacheDefinition(
    bindir, this->Makefile->GetCurrentBinaryDirectory(),
    "Value Computed by CMake", cmStateEnums::STATIC);
  this->Makefile->AddCacheDefinition(
    srcdir, this->Makefile->GetCurrentSourceDirectory(),
    "Value Computed by CMake", cmStateEnums::STATIC);

  bindir = "PROJECT_BINARY_DIR";
  srcdir = "PROJECT_SOURCE_DIR";

  this->Makefile->AddDefinition(bindir,
                                this->Makefile->GetCurrentBinaryDirectory());
  this->Makefile->AddDefinition(srcdir,
                                this->Makefile->GetCurrentSourceDirectory());

  this->Makefile->AddDefinition("PROJECT_NAME", projectName.c_str());

  // Set the CMAKE_PROJECT_NAME variable to be the highest-level
  // project name in the tree. If there are two project commands
  // in the same CMakeLists.txt file, and it is the top level
  // CMakeLists.txt file, then go with the last one, so that
  // CMAKE_PROJECT_NAME will match PROJECT_NAME, and cmake --build
  // will work.
  if (!this->Makefile->GetDefinition("CMAKE_PROJECT_NAME") ||
      (this->Makefile->IsRootMakefile())) {
    this->Makefile->AddDefinition("CMAKE_PROJECT_NAME", projectName.c_str());
    this->Makefile->AddCacheDefinition(
      "CMAKE_PROJECT_NAME", projectName.c_str(), "Value Computed by CMake",
      cmStateEnums::STATIC);
  }

  bool haveVersion = false;
  bool haveLanguages = false;
  bool haveDescription = false;
  std::string version;
  std::string description;
  std::vector<std::string> languages;
  enum Doing
  {
    DoingDescription,
    DoingLanguages,
    DoingVersion
  };
  Doing doing = DoingLanguages;
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "LANGUAGES") {
      if (haveLanguages) {
        this->Makefile->IssueMessage(
          cmake::FATAL_ERROR, "LANGUAGES may be specified at most once.");
        cmSystemTools::SetFatalErrorOccured();
        return true;
      }
      haveLanguages = true;
      doing = DoingLanguages;
    } else if (args[i] == "VERSION") {
      if (haveVersion) {
        this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                     "VERSION may be specified at most once.");
        cmSystemTools::SetFatalErrorOccured();
        return true;
      }
      haveVersion = true;
      doing = DoingVersion;
    } else if (args[i] == "DESCRIPTION") {
      if (haveDescription) {
        this->Makefile->IssueMessage(
          cmake::FATAL_ERROR, "DESCRITPION may be specified at most once.");
        cmSystemTools::SetFatalErrorOccured();
        return true;
      }
      haveDescription = true;
      doing = DoingDescription;
    } else if (doing == DoingVersion) {
      doing = DoingLanguages;
      version = args[i];
    } else if (doing == DoingDescription) {
      doing = DoingLanguages;
      description = args[i];
    } else // doing == DoingLanguages
    {
      languages.push_back(args[i]);
    }
  }

  if (haveVersion && !haveLanguages && !languages.empty()) {
    this->Makefile->IssueMessage(
      cmake::FATAL_ERROR,
      "project with VERSION must use LANGUAGES before language names.");
    cmSystemTools::SetFatalErrorOccured();
    return true;
  }
  if (haveLanguages && languages.empty()) {
    languages.push_back("NONE");
  }

  cmPolicies::PolicyStatus cmp0048 =
    this->Makefile->GetPolicyStatus(cmPolicies::CMP0048);
  if (haveVersion) {
    // Set project VERSION variables to given values
    if (cmp0048 == cmPolicies::OLD || cmp0048 == cmPolicies::WARN) {
      this->Makefile->IssueMessage(
        cmake::FATAL_ERROR,
        "VERSION not allowed unless CMP0048 is set to NEW");
      cmSystemTools::SetFatalErrorOccured();
      return true;
    }

    cmsys::RegularExpression vx(
      "^([0-9]+(\\.[0-9]+(\\.[0-9]+(\\.[0-9]+)?)?)?)?$");
    if (!vx.find(version)) {
      std::string e = "VERSION \"" + version + "\" format invalid.";
      this->Makefile->IssueMessage(cmake::FATAL_ERROR, e);
      cmSystemTools::SetFatalErrorOccured();
      return true;
    }

    std::string vs;
    const char* sep = "";
    char vb[4][64];
    unsigned int v[4] = { 0, 0, 0, 0 };
    int vc =
      sscanf(version.c_str(), "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
    for (int i = 0; i < 4; ++i) {
      if (i < vc) {
        sprintf(vb[i], "%u", v[i]);
        vs += sep;
        vs += vb[i];
        sep = ".";
      } else {
        vb[i][0] = 0;
      }
    }

    std::string vv;
    vv = projectName + "_VERSION";
    this->Makefile->AddDefinition("PROJECT_VERSION", vs.c_str());
    this->Makefile->AddDefinition(vv, vs.c_str());
    vv = projectName + "_VERSION_MAJOR";
    this->Makefile->AddDefinition("PROJECT_VERSION_MAJOR", vb[0]);
    this->Makefile->AddDefinition(vv, vb[0]);
    vv = projectName + "_VERSION_MINOR";
    this->Makefile->AddDefinition("PROJECT_VERSION_MINOR", vb[1]);
    this->Makefile->AddDefinition(vv, vb[1]);
    vv = projectName + "_VERSION_PATCH";
    this->Makefile->AddDefinition("PROJECT_VERSION_PATCH", vb[2]);
    this->Makefile->AddDefinition(vv, vb[2]);
    vv = projectName + "_VERSION_TWEAK";
    this->Makefile->AddDefinition("PROJECT_VERSION_TWEAK", vb[3]);
    this->Makefile->AddDefinition(vv, vb[3]);
  } else if (cmp0048 != cmPolicies::OLD) {
    // Set project VERSION variables to empty
    std::vector<std::string> vv;
    vv.push_back("PROJECT_VERSION");
    vv.push_back("PROJECT_VERSION_MAJOR");
    vv.push_back("PROJECT_VERSION_MINOR");
    vv.push_back("PROJECT_VERSION_PATCH");
    vv.push_back("PROJECT_VERSION_TWEAK");
    vv.push_back(projectName + "_VERSION");
    vv.push_back(projectName + "_VERSION_MAJOR");
    vv.push_back(projectName + "_VERSION_MINOR");
    vv.push_back(projectName + "_VERSION_PATCH");
    vv.push_back(projectName + "_VERSION_TWEAK");
    std::string vw;
    for (std::vector<std::string>::iterator i = vv.begin(); i != vv.end();
         ++i) {
      const char* v = this->Makefile->GetDefinition(*i);
      if (v && *v) {
        if (cmp0048 == cmPolicies::WARN) {
          vw += "\n  ";
          vw += *i;
        } else {
          this->Makefile->AddDefinition(*i, "");
        }
      }
    }
    if (!vw.empty()) {
      std::ostringstream w;
      w << cmPolicies::GetPolicyWarning(cmPolicies::CMP0048)
        << "\nThe following variable(s) would be set to empty:" << vw;
      this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, w.str());
    }
  }

  if (haveDescription) {
    this->Makefile->AddDefinition("PROJECT_DESCRIPTION", description.c_str());
    // Set the CMAKE_PROJECT_DESCRIPTION variable to be the highest-level
    // project name in the tree. If there are two project commands
    // in the same CMakeLists.txt file, and it is the top level
    // CMakeLists.txt file, then go with the last one.
    if (!this->Makefile->GetDefinition("CMAKE_PROJECT_DESCRIPTION") ||
        (this->Makefile->IsRootMakefile())) {
      this->Makefile->AddDefinition("CMAKE_PROJECT_DESCRIPTION",
                                    description.c_str());
      this->Makefile->AddCacheDefinition(
        "CMAKE_PROJECT_DESCRIPTION", description.c_str(),
        "Value Computed by CMake", cmStateEnums::STATIC);
    }
  }

  if (languages.empty()) {
    // if no language is specified do c and c++
    languages.push_back("C");
    languages.push_back("CXX");
  }
  this->Makefile->EnableLanguage(languages, false);
  std::string extraInclude = "CMAKE_PROJECT_" + projectName + "_INCLUDE";
  const char* include = this->Makefile->GetDefinition(extraInclude);
  if (include) {
    bool readit = this->Makefile->ReadDependentFile(include);
    if (!readit && !cmSystemTools::GetFatalErrorOccured()) {
      std::string m = "could not find file:\n"
                      "  ";
      m += include;
      this->SetError(m);
      return false;
    }
  }
  return true;
}
