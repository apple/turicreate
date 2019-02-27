/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmProjectCommand.h"

#include "cmsys/RegularExpression.hxx"
#include <functional>
#include <sstream>
#include <stdio.h>

#include "cmAlgorithms.h"
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
    bindir, this->Makefile->GetCurrentBinaryDirectory().c_str(),
    "Value Computed by CMake", cmStateEnums::STATIC);
  this->Makefile->AddCacheDefinition(
    srcdir, this->Makefile->GetCurrentSourceDirectory().c_str(),
    "Value Computed by CMake", cmStateEnums::STATIC);

  bindir = "PROJECT_BINARY_DIR";
  srcdir = "PROJECT_SOURCE_DIR";

  this->Makefile->AddDefinition(
    bindir, this->Makefile->GetCurrentBinaryDirectory().c_str());
  this->Makefile->AddDefinition(
    srcdir, this->Makefile->GetCurrentSourceDirectory().c_str());

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
  bool haveHomepage = false;
  bool injectedProjectCommand = false;
  std::string version;
  std::string description;
  std::string homepage;
  std::vector<std::string> languages;
  std::function<void()> missedValueReporter;
  auto resetReporter = [&missedValueReporter]() {
    missedValueReporter = std::function<void()>();
  };
  enum Doing
  {
    DoingDescription,
    DoingHomepage,
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
      if (missedValueReporter) {
        missedValueReporter();
      }
      doing = DoingLanguages;
      if (!languages.empty()) {
        std::string msg =
          "the following parameters must be specified after LANGUAGES "
          "keyword: ";
        msg += cmJoin(languages, ", ");
        msg += '.';
        this->Makefile->IssueMessage(cmake::WARNING, msg);
      }
    } else if (args[i] == "VERSION") {
      if (haveVersion) {
        this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                     "VERSION may be specified at most once.");
        cmSystemTools::SetFatalErrorOccured();
        return true;
      }
      haveVersion = true;
      if (missedValueReporter) {
        missedValueReporter();
      }
      doing = DoingVersion;
      missedValueReporter = [this, &resetReporter]() {
        this->Makefile->IssueMessage(
          cmake::WARNING,
          "VERSION keyword not followed by a value or was followed by a "
          "value that expanded to nothing.");
        resetReporter();
      };
    } else if (args[i] == "DESCRIPTION") {
      if (haveDescription) {
        this->Makefile->IssueMessage(
          cmake::FATAL_ERROR, "DESCRIPTION may be specified at most once.");
        cmSystemTools::SetFatalErrorOccured();
        return true;
      }
      haveDescription = true;
      if (missedValueReporter) {
        missedValueReporter();
      }
      doing = DoingDescription;
      missedValueReporter = [this, &resetReporter]() {
        this->Makefile->IssueMessage(
          cmake::WARNING,
          "DESCRIPTION keyword not followed by a value or was followed "
          "by a value that expanded to nothing.");
        resetReporter();
      };
    } else if (args[i] == "HOMEPAGE_URL") {
      if (haveHomepage) {
        this->Makefile->IssueMessage(
          cmake::FATAL_ERROR, "HOMEPAGE_URL may be specified at most once.");
        cmSystemTools::SetFatalErrorOccured();
        return true;
      }
      haveHomepage = true;
      doing = DoingHomepage;
      missedValueReporter = [this, &resetReporter]() {
        this->Makefile->IssueMessage(
          cmake::WARNING,
          "HOMEPAGE_URL keyword not followed by a value or was followed "
          "by a value that expanded to nothing.");
        resetReporter();
      };
    } else if (i == 1 && args[i] == "__CMAKE_INJECTED_PROJECT_COMMAND__") {
      injectedProjectCommand = true;
    } else if (doing == DoingVersion) {
      doing = DoingLanguages;
      version = args[i];
      resetReporter();
    } else if (doing == DoingDescription) {
      doing = DoingLanguages;
      description = args[i];
      resetReporter();
    } else if (doing == DoingHomepage) {
      doing = DoingLanguages;
      homepage = args[i];
      resetReporter();
    } else // doing == DoingLanguages
    {
      languages.push_back(args[i]);
    }
  }

  if (missedValueReporter) {
    missedValueReporter();
  }

  if ((haveVersion || haveDescription || haveHomepage) && !haveLanguages &&
      !languages.empty()) {
    this->Makefile->IssueMessage(
      cmake::FATAL_ERROR,
      "project with VERSION, DESCRIPTION or HOMEPAGE_URL must "
      "use LANGUAGES before language names.");
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
    // Also, try set top level variables
    TopLevelCMakeVarCondSet("CMAKE_PROJECT_VERSION", vs.c_str());
    TopLevelCMakeVarCondSet("CMAKE_PROJECT_VERSION_MAJOR", vb[0]);
    TopLevelCMakeVarCondSet("CMAKE_PROJECT_VERSION_MINOR", vb[1]);
    TopLevelCMakeVarCondSet("CMAKE_PROJECT_VERSION_PATCH", vb[2]);
    TopLevelCMakeVarCondSet("CMAKE_PROJECT_VERSION_TWEAK", vb[3]);
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
    if (this->Makefile->IsRootMakefile()) {
      vv.push_back("CMAKE_PROJECT_VERSION");
      vv.push_back("CMAKE_PROJECT_VERSION_MAJOR");
      vv.push_back("CMAKE_PROJECT_VERSION_MINOR");
      vv.push_back("CMAKE_PROJECT_VERSION_PATCH");
      vv.push_back("CMAKE_PROJECT_VERSION_TWEAK");
    }
    std::string vw;
    for (std::string const& i : vv) {
      const char* v = this->Makefile->GetDefinition(i);
      if (v && *v) {
        if (cmp0048 == cmPolicies::WARN) {
          if (!injectedProjectCommand) {
            vw += "\n  ";
            vw += i;
          }
        } else {
          this->Makefile->AddDefinition(i, "");
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

  this->Makefile->AddDefinition("PROJECT_DESCRIPTION", description.c_str());
  this->Makefile->AddDefinition(projectName + "_DESCRIPTION",
                                description.c_str());
  TopLevelCMakeVarCondSet("CMAKE_PROJECT_DESCRIPTION", description.c_str());

  this->Makefile->AddDefinition("PROJECT_HOMEPAGE_URL", homepage.c_str());
  this->Makefile->AddDefinition(projectName + "_HOMEPAGE_URL",
                                homepage.c_str());
  TopLevelCMakeVarCondSet("CMAKE_PROJECT_HOMEPAGE_URL", homepage.c_str());

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

void cmProjectCommand::TopLevelCMakeVarCondSet(const char* const name,
                                               const char* const value)
{
  // Set the CMAKE_PROJECT_XXX variable to be the highest-level
  // project name in the tree. If there are two project commands
  // in the same CMakeLists.txt file, and it is the top level
  // CMakeLists.txt file, then go with the last one.
  if (!this->Makefile->GetDefinition(name) ||
      (this->Makefile->IsRootMakefile())) {
    this->Makefile->AddDefinition(name, value);
    this->Makefile->AddCacheDefinition(name, value, "Value Computed by CMake",
                                       cmStateEnums::STATIC);
  }
}
