/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSetPropertyCommand.h"

#include <sstream>

#include "cmGlobalGenerator.h"
#include "cmInstalledFile.h"
#include "cmMakefile.h"
#include "cmProperty.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTest.h"
#include "cmake.h"

class cmExecutionStatus;

cmSetPropertyCommand::cmSetPropertyCommand()
{
  this->AppendMode = false;
  this->AppendAsString = false;
  this->Remove = true;
}

bool cmSetPropertyCommand::InitialPass(std::vector<std::string> const& args,
                                       cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // Get the scope on which to set the property.
  std::vector<std::string>::const_iterator arg = args.begin();
  cmProperty::ScopeType scope;
  if (*arg == "GLOBAL") {
    scope = cmProperty::GLOBAL;
  } else if (*arg == "DIRECTORY") {
    scope = cmProperty::DIRECTORY;
  } else if (*arg == "TARGET") {
    scope = cmProperty::TARGET;
  } else if (*arg == "SOURCE") {
    scope = cmProperty::SOURCE_FILE;
  } else if (*arg == "TEST") {
    scope = cmProperty::TEST;
  } else if (*arg == "CACHE") {
    scope = cmProperty::CACHE;
  } else if (*arg == "INSTALL") {
    scope = cmProperty::INSTALL;
  } else {
    std::ostringstream e;
    e << "given invalid scope " << *arg << ".  "
      << "Valid scopes are GLOBAL, DIRECTORY, "
         "TARGET, SOURCE, TEST, CACHE, INSTALL.";
    this->SetError(e.str());
    return false;
  }

  // Parse the rest of the arguments up to the values.
  enum Doing
  {
    DoingNone,
    DoingNames,
    DoingProperty,
    DoingValues
  };
  Doing doing = DoingNames;
  const char* sep = "";
  for (++arg; arg != args.end(); ++arg) {
    if (*arg == "PROPERTY") {
      doing = DoingProperty;
    } else if (*arg == "APPEND") {
      doing = DoingNone;
      this->AppendMode = true;
      this->Remove = false;
      this->AppendAsString = false;
    } else if (*arg == "APPEND_STRING") {
      doing = DoingNone;
      this->AppendMode = true;
      this->Remove = false;
      this->AppendAsString = true;
    } else if (doing == DoingNames) {
      this->Names.insert(*arg);
    } else if (doing == DoingProperty) {
      this->PropertyName = *arg;
      doing = DoingValues;
    } else if (doing == DoingValues) {
      this->PropertyValue += sep;
      sep = ";";
      this->PropertyValue += *arg;
      this->Remove = false;
    } else {
      std::ostringstream e;
      e << "given invalid argument \"" << *arg << "\".";
      this->SetError(e.str());
      return false;
    }
  }

  // Make sure a property name was found.
  if (this->PropertyName.empty()) {
    this->SetError("not given a PROPERTY <name> argument.");
    return false;
  }

  // Dispatch property setting.
  switch (scope) {
    case cmProperty::GLOBAL:
      return this->HandleGlobalMode();
    case cmProperty::DIRECTORY:
      return this->HandleDirectoryMode();
    case cmProperty::TARGET:
      return this->HandleTargetMode();
    case cmProperty::SOURCE_FILE:
      return this->HandleSourceMode();
    case cmProperty::TEST:
      return this->HandleTestMode();
    case cmProperty::CACHE:
      return this->HandleCacheMode();
    case cmProperty::INSTALL:
      return this->HandleInstallMode();

    case cmProperty::VARIABLE:
    case cmProperty::CACHED_VARIABLE:
      break; // should never happen
  }
  return true;
}

bool cmSetPropertyCommand::HandleGlobalMode()
{
  if (!this->Names.empty()) {
    this->SetError("given names for GLOBAL scope.");
    return false;
  }

  // Set or append the property.
  cmake* cm = this->Makefile->GetCMakeInstance();
  std::string const& name = this->PropertyName;
  const char* value = this->PropertyValue.c_str();
  if (this->Remove) {
    value = CM_NULLPTR;
  }
  if (this->AppendMode) {
    cm->AppendProperty(name, value ? value : "", this->AppendAsString);
  } else {
    cm->SetProperty(name, value);
  }

  return true;
}

bool cmSetPropertyCommand::HandleDirectoryMode()
{
  if (this->Names.size() > 1) {
    this->SetError("allows at most one name for DIRECTORY scope.");
    return false;
  }

  // Default to the current directory.
  cmMakefile* mf = this->Makefile;

  // Lookup the directory if given.
  if (!this->Names.empty()) {
    // Construct the directory name.  Interpret relative paths with
    // respect to the current directory.
    std::string dir = *this->Names.begin();
    if (!cmSystemTools::FileIsFullPath(dir.c_str())) {
      dir = this->Makefile->GetCurrentSourceDirectory();
      dir += "/";
      dir += *this->Names.begin();
    }

    // The local generators are associated with collapsed paths.
    dir = cmSystemTools::CollapseFullPath(dir);

    mf = this->Makefile->GetGlobalGenerator()->FindMakefile(dir);
    if (!mf) {
      // Could not find the directory.
      this->SetError(
        "DIRECTORY scope provided but requested directory was not found. "
        "This could be because the directory argument was invalid or, "
        "it is valid but has not been processed yet.");
      return false;
    }
  }

  // Set or append the property.
  std::string const& name = this->PropertyName;
  const char* value = this->PropertyValue.c_str();
  if (this->Remove) {
    value = CM_NULLPTR;
  }
  if (this->AppendMode) {
    mf->AppendProperty(name, value ? value : "", this->AppendAsString);
  } else {
    mf->SetProperty(name, value);
  }

  return true;
}

bool cmSetPropertyCommand::HandleTargetMode()
{
  for (std::set<std::string>::const_iterator ni = this->Names.begin();
       ni != this->Names.end(); ++ni) {
    if (this->Makefile->IsAlias(*ni)) {
      this->SetError("can not be used on an ALIAS target.");
      return false;
    }
    if (cmTarget* target = this->Makefile->FindTargetToUse(*ni)) {
      // Handle the current target.
      if (!this->HandleTarget(target)) {
        return false;
      }
    } else {
      std::ostringstream e;
      e << "could not find TARGET " << *ni
        << ".  Perhaps it has not yet been created.";
      this->SetError(e.str());
      return false;
    }
  }
  return true;
}

bool cmSetPropertyCommand::HandleTarget(cmTarget* target)
{
  // Set or append the property.
  std::string const& name = this->PropertyName;
  const char* value = this->PropertyValue.c_str();
  if (this->Remove) {
    value = CM_NULLPTR;
  }
  if (this->AppendMode) {
    target->AppendProperty(name, value, this->AppendAsString);
  } else {
    target->SetProperty(name, value);
  }

  // Check the resulting value.
  target->CheckProperty(name, this->Makefile);

  return true;
}

bool cmSetPropertyCommand::HandleSourceMode()
{
  for (std::set<std::string>::const_iterator ni = this->Names.begin();
       ni != this->Names.end(); ++ni) {
    // Get the source file.
    if (cmSourceFile* sf = this->Makefile->GetOrCreateSource(*ni)) {
      if (!this->HandleSource(sf)) {
        return false;
      }
    } else {
      std::ostringstream e;
      e << "given SOURCE name that could not be found or created: " << *ni;
      this->SetError(e.str());
      return false;
    }
  }
  return true;
}

bool cmSetPropertyCommand::HandleSource(cmSourceFile* sf)
{
  // Set or append the property.
  std::string const& name = this->PropertyName;
  const char* value = this->PropertyValue.c_str();
  if (this->Remove) {
    value = CM_NULLPTR;
  }

  if (this->AppendMode) {
    sf->AppendProperty(name, value, this->AppendAsString);
  } else {
    sf->SetProperty(name, value);
  }
  return true;
}

bool cmSetPropertyCommand::HandleTestMode()
{
  // Look for tests with all names given.
  std::set<std::string>::iterator next;
  for (std::set<std::string>::iterator ni = this->Names.begin();
       ni != this->Names.end(); ni = next) {
    next = ni;
    ++next;
    if (cmTest* test = this->Makefile->GetTest(*ni)) {
      if (this->HandleTest(test)) {
        this->Names.erase(ni);
      } else {
        return false;
      }
    }
  }

  // Names that are still left were not found.
  if (!this->Names.empty()) {
    std::ostringstream e;
    e << "given TEST names that do not exist:\n";
    for (std::set<std::string>::const_iterator ni = this->Names.begin();
         ni != this->Names.end(); ++ni) {
      e << "  " << *ni << "\n";
    }
    this->SetError(e.str());
    return false;
  }
  return true;
}

bool cmSetPropertyCommand::HandleTest(cmTest* test)
{
  // Set or append the property.
  std::string const& name = this->PropertyName;
  const char* value = this->PropertyValue.c_str();
  if (this->Remove) {
    value = CM_NULLPTR;
  }
  if (this->AppendMode) {
    test->AppendProperty(name, value, this->AppendAsString);
  } else {
    test->SetProperty(name, value);
  }

  return true;
}

bool cmSetPropertyCommand::HandleCacheMode()
{
  if (this->PropertyName == "ADVANCED") {
    if (!this->Remove && !cmSystemTools::IsOn(this->PropertyValue.c_str()) &&
        !cmSystemTools::IsOff(this->PropertyValue.c_str())) {
      std::ostringstream e;
      e << "given non-boolean value \"" << this->PropertyValue
        << "\" for CACHE property \"ADVANCED\".  ";
      this->SetError(e.str());
      return false;
    }
  } else if (this->PropertyName == "TYPE") {
    if (!cmState::IsCacheEntryType(this->PropertyValue)) {
      std::ostringstream e;
      e << "given invalid CACHE entry TYPE \"" << this->PropertyValue << "\"";
      this->SetError(e.str());
      return false;
    }
  } else if (this->PropertyName != "HELPSTRING" &&
             this->PropertyName != "STRINGS" &&
             this->PropertyName != "VALUE") {
    std::ostringstream e;
    e << "given invalid CACHE property " << this->PropertyName << ".  "
      << "Settable CACHE properties are: "
      << "ADVANCED, HELPSTRING, STRINGS, TYPE, and VALUE.";
    this->SetError(e.str());
    return false;
  }

  for (std::set<std::string>::const_iterator ni = this->Names.begin();
       ni != this->Names.end(); ++ni) {
    // Get the source file.
    cmMakefile* mf = this->GetMakefile();
    cmake* cm = mf->GetCMakeInstance();
    const char* existingValue = cm->GetState()->GetCacheEntryValue(*ni);
    if (existingValue) {
      if (!this->HandleCacheEntry(*ni)) {
        return false;
      }
    } else {
      std::ostringstream e;
      e << "could not find CACHE variable " << *ni
        << ".  Perhaps it has not yet been created.";
      this->SetError(e.str());
      return false;
    }
  }
  return true;
}

bool cmSetPropertyCommand::HandleCacheEntry(std::string const& cacheKey)
{
  // Set or append the property.
  std::string const& name = this->PropertyName;
  const char* value = this->PropertyValue.c_str();
  cmState* state = this->Makefile->GetState();
  if (this->Remove) {
    state->RemoveCacheEntryProperty(cacheKey, name);
  }
  if (this->AppendMode) {
    state->AppendCacheEntryProperty(cacheKey, name, value,
                                    this->AppendAsString);
  } else {
    state->SetCacheEntryProperty(cacheKey, name, value);
  }

  return true;
}

bool cmSetPropertyCommand::HandleInstallMode()
{
  cmake* cm = this->Makefile->GetCMakeInstance();

  for (std::set<std::string>::const_iterator i = this->Names.begin();
       i != this->Names.end(); ++i) {
    if (cmInstalledFile* file =
          cm->GetOrCreateInstalledFile(this->Makefile, *i)) {
      if (!this->HandleInstall(file)) {
        return false;
      }
    } else {
      std::ostringstream e;
      e << "given INSTALL name that could not be found or created: " << *i;
      this->SetError(e.str());
      return false;
    }
  }
  return true;
}

bool cmSetPropertyCommand::HandleInstall(cmInstalledFile* file)
{
  // Set or append the property.
  std::string const& name = this->PropertyName;

  cmMakefile* mf = this->Makefile;

  const char* value = this->PropertyValue.c_str();
  if (this->Remove) {
    file->RemoveProperty(name);
  } else if (this->AppendMode) {
    file->AppendProperty(mf, name, value, this->AppendAsString);
  } else {
    file->SetProperty(mf, name, value);
  }
  return true;
}
