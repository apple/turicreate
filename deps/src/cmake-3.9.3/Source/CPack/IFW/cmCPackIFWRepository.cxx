/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackIFWRepository.h"

#include "cmCPackIFWGenerator.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"
#include "cmXMLWriter.h"

#include <stddef.h>

cmCPackIFWRepository::cmCPackIFWRepository()
  : Update(cmCPackIFWRepository::None)
{
}

bool cmCPackIFWRepository::IsValid() const
{
  bool valid = true;

  switch (this->Update) {
    case cmCPackIFWRepository::None:
      valid = !this->Url.empty();
      break;
    case cmCPackIFWRepository::Add:
      valid = !this->Url.empty();
      break;
    case cmCPackIFWRepository::Remove:
      valid = !this->Url.empty();
      break;
    case cmCPackIFWRepository::Replace:
      valid = !this->OldUrl.empty() && !this->NewUrl.empty();
      break;
  }

  return valid;
}

bool cmCPackIFWRepository::ConfigureFromOptions()
{
  // Name;
  if (this->Name.empty()) {
    return false;
  }

  std::string prefix =
    "CPACK_IFW_REPOSITORY_" + cmsys::SystemTools::UpperCase(this->Name) + "_";

  // Update
  if (this->IsOn(prefix + "ADD")) {
    this->Update = cmCPackIFWRepository::Add;
  } else if (IsOn(prefix + "REMOVE")) {
    this->Update = cmCPackIFWRepository::Remove;
  } else if (IsOn(prefix + "REPLACE")) {
    this->Update = cmCPackIFWRepository::Replace;
  } else {
    this->Update = cmCPackIFWRepository::None;
  }

  // Url
  if (const char* url = this->GetOption(prefix + "URL")) {
    this->Url = url;
  } else {
    this->Url = "";
  }

  // Old url
  if (const char* oldUrl = this->GetOption(prefix + "OLD_URL")) {
    this->OldUrl = oldUrl;
  } else {
    this->OldUrl = "";
  }

  // New url
  if (const char* newUrl = this->GetOption(prefix + "NEW_URL")) {
    this->NewUrl = newUrl;
  } else {
    this->NewUrl = "";
  }

  // Enabled
  if (this->IsOn(prefix + "DISABLED")) {
    this->Enabled = "0";
  } else {
    this->Enabled = "";
  }

  // Username
  if (const char* username = this->GetOption(prefix + "USERNAME")) {
    this->Username = username;
  } else {
    this->Username = "";
  }

  // Password
  if (const char* password = this->GetOption(prefix + "PASSWORD")) {
    this->Password = password;
  } else {
    this->Password = "";
  }

  // DisplayName
  if (const char* displayName = this->GetOption(prefix + "DISPLAY_NAME")) {
    this->DisplayName = displayName;
  } else {
    this->DisplayName = "";
  }

  return this->IsValid();
}

/** \class cmCPackeIFWUpdatesPatcher
 * \brief Helper class that parses and patch Updates.xml file (QtIFW)
 */
class cmCPackeIFWUpdatesPatcher : public cmXMLParser
{
public:
  cmCPackeIFWUpdatesPatcher(cmCPackIFWRepository* r, cmXMLWriter& x)
    : repository(r)
    , xout(x)
    , patched(false)
  {
  }

  cmCPackIFWRepository* repository;
  cmXMLWriter& xout;
  bool patched;

protected:
  void StartElement(const std::string& name, const char** atts) CM_OVERRIDE
  {
    this->xout.StartElement(name);
    this->StartFragment(atts);
  }

  void StartFragment(const char** atts)
  {
    for (size_t i = 0; atts[i]; i += 2) {
      const char* key = atts[i];
      const char* value = atts[i + 1];
      this->xout.Attribute(key, value);
    }
  }

  void EndElement(const std::string& name) CM_OVERRIDE
  {
    if (name == "Updates" && !this->patched) {
      this->repository->WriteRepositoryUpdates(this->xout);
      this->patched = true;
    }
    this->xout.EndElement();
    if (this->patched) {
      return;
    }
    if (name == "Checksum") {
      this->repository->WriteRepositoryUpdates(this->xout);
      this->patched = true;
    }
  }

  void CharacterDataHandler(const char* data, int length) CM_OVERRIDE
  {
    std::string content(data, data + length);
    if (content == "" || content == " " || content == "  " ||
        content == "\n") {
      return;
    }
    this->xout.Content(content);
  }
};

bool cmCPackIFWRepository::PatchUpdatesXml()
{
  // Lazy directory initialization
  if (this->Directory.empty() && this->Generator) {
    this->Directory = this->Generator->toplevel;
  }

  // Filenames
  std::string updatesXml = this->Directory + "/repository/Updates.xml";
  std::string updatesPatchXml =
    this->Directory + "/repository/UpdatesPatch.xml";

  // Output stream
  cmGeneratedFileStream fout(updatesPatchXml.data());
  cmXMLWriter xout(fout);

  xout.StartDocument();

  this->WriteGeneratedByToStrim(xout);

  // Patch
  {
    cmCPackeIFWUpdatesPatcher patcher(this, xout);
    patcher.ParseFile(updatesXml.data());
  }

  xout.EndDocument();

  fout.Close();

  return cmSystemTools::RenameFile(updatesPatchXml.data(), updatesXml.data());
}

void cmCPackIFWRepository::WriteRepositoryConfig(cmXMLWriter& xout)
{
  xout.StartElement("Repository");

  // Url
  xout.Element("Url", this->Url);
  // Enabled
  if (!this->Enabled.empty()) {
    xout.Element("Enabled", this->Enabled);
  }
  // Username
  if (!this->Username.empty()) {
    xout.Element("Username", this->Username);
  }
  // Password
  if (!this->Password.empty()) {
    xout.Element("Password", this->Password);
  }
  // DisplayName
  if (!this->DisplayName.empty()) {
    xout.Element("DisplayName", this->DisplayName);
  }

  xout.EndElement();
}

void cmCPackIFWRepository::WriteRepositoryUpdate(cmXMLWriter& xout)
{
  xout.StartElement("Repository");

  switch (this->Update) {
    case cmCPackIFWRepository::None:
      break;
    case cmCPackIFWRepository::Add:
      xout.Attribute("action", "add");
      break;
    case cmCPackIFWRepository::Remove:
      xout.Attribute("action", "remove");
      break;
    case cmCPackIFWRepository::Replace:
      xout.Attribute("action", "replace");
      break;
  }

  // Url
  if (this->Update == cmCPackIFWRepository::Add ||
      this->Update == cmCPackIFWRepository::Remove) {
    xout.Attribute("url", this->Url);
  } else if (Update == cmCPackIFWRepository::Replace) {
    xout.Attribute("oldUrl", this->OldUrl);
    xout.Attribute("newUrl", this->NewUrl);
  }
  // Enabled
  if (!this->Enabled.empty()) {
    xout.Attribute("enabled", this->Enabled);
  }
  // Username
  if (!this->Username.empty()) {
    xout.Attribute("username", this->Username);
  }
  // Password
  if (!this->Password.empty()) {
    xout.Attribute("password", this->Password);
  }
  // DisplayName
  if (!this->DisplayName.empty()) {
    xout.Attribute("displayname", this->DisplayName);
  }

  xout.EndElement();
}

void cmCPackIFWRepository::WriteRepositoryUpdates(cmXMLWriter& xout)
{
  if (!this->RepositoryUpdate.empty()) {
    xout.StartElement("RepositoryUpdate");
    for (RepositoriesVector::iterator rit = this->RepositoryUpdate.begin();
         rit != this->RepositoryUpdate.end(); ++rit) {
      (*rit)->WriteRepositoryUpdate(xout);
    }
    xout.EndElement();
  }
}
