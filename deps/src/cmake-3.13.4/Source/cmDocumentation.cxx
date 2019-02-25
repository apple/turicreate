/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmDocumentation.h"

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmDocumentationSection.h"
#include "cmRST.h"
#include "cmSystemTools.h"
#include "cmVersion.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include <algorithm>
#include <ctype.h>
#include <string.h>
#include <utility>

static const char* cmDocumentationStandardOptions[][2] = {
  { "--help,-help,-usage,-h,-H,/?", "Print usage information and exit." },
  { "--version,-version,/V [<f>]", "Print version number and exit." },
  { "--help-full [<f>]", "Print all help manuals and exit." },
  { "--help-manual <man> [<f>]", "Print one help manual and exit." },
  { "--help-manual-list [<f>]", "List help manuals available and exit." },
  { "--help-command <cmd> [<f>]", "Print help for one command and exit." },
  { "--help-command-list [<f>]",
    "List commands with help available and exit." },
  { "--help-commands [<f>]", "Print cmake-commands manual and exit." },
  { "--help-module <mod> [<f>]", "Print help for one module and exit." },
  { "--help-module-list [<f>]", "List modules with help available and exit." },
  { "--help-modules [<f>]", "Print cmake-modules manual and exit." },
  { "--help-policy <cmp> [<f>]", "Print help for one policy and exit." },
  { "--help-policy-list [<f>]",
    "List policies with help available and exit." },
  { "--help-policies [<f>]", "Print cmake-policies manual and exit." },
  { "--help-property <prop> [<f>]", "Print help for one property and exit." },
  { "--help-property-list [<f>]",
    "List properties with help available and exit." },
  { "--help-properties [<f>]", "Print cmake-properties manual and exit." },
  { "--help-variable var [<f>]", "Print help for one variable and exit." },
  { "--help-variable-list [<f>]",
    "List variables with help available and exit." },
  { "--help-variables [<f>]", "Print cmake-variables manual and exit." },
  { nullptr, nullptr }
};

static const char* cmDocumentationGeneratorsHeader[][2] = {
  { nullptr, "The following generators are available on this platform:" },
  { nullptr, nullptr }
};

cmDocumentation::cmDocumentation()
{
  this->addCommonStandardDocSections();
  this->ShowGenerators = true;
}

cmDocumentation::~cmDocumentation()
{
  cmDeleteAll(this->AllSections);
}

bool cmDocumentation::PrintVersion(std::ostream& os)
{
  /* clang-format off */
  os <<
    this->GetNameString() <<
    " version " << cmVersion::GetCMakeVersion() << "\n"
    "\n"
    "CMake suite maintained and supported by Kitware (kitware.com/cmake).\n"
    ;
  /* clang-format on */
  return true;
}

bool cmDocumentation::PrintDocumentation(Type ht, std::ostream& os)
{
  switch (ht) {
    case cmDocumentation::Usage:
      return this->PrintUsage(os);
    case cmDocumentation::Help:
      return this->PrintHelp(os);
    case cmDocumentation::Full:
      return this->PrintHelpFull(os);
    case cmDocumentation::OneManual:
      return this->PrintHelpOneManual(os);
    case cmDocumentation::OneCommand:
      return this->PrintHelpOneCommand(os);
    case cmDocumentation::OneModule:
      return this->PrintHelpOneModule(os);
    case cmDocumentation::OnePolicy:
      return this->PrintHelpOnePolicy(os);
    case cmDocumentation::OneProperty:
      return this->PrintHelpOneProperty(os);
    case cmDocumentation::OneVariable:
      return this->PrintHelpOneVariable(os);
    case cmDocumentation::ListManuals:
      return this->PrintHelpListManuals(os);
    case cmDocumentation::ListCommands:
      return this->PrintHelpListCommands(os);
    case cmDocumentation::ListModules:
      return this->PrintHelpListModules(os);
    case cmDocumentation::ListProperties:
      return this->PrintHelpListProperties(os);
    case cmDocumentation::ListVariables:
      return this->PrintHelpListVariables(os);
    case cmDocumentation::ListPolicies:
      return this->PrintHelpListPolicies(os);
    case cmDocumentation::ListGenerators:
      return this->PrintHelpListGenerators(os);
    case cmDocumentation::Version:
      return this->PrintVersion(os);
    case cmDocumentation::OldCustomModules:
      return this->PrintOldCustomModules(os);
    default:
      return false;
  }
}

bool cmDocumentation::PrintRequestedDocumentation(std::ostream& os)
{
  int count = 0;
  bool result = true;

  // Loop over requested documentation types.
  for (RequestedHelpItem const& rhi : this->RequestedHelpItems) {
    this->CurrentArgument = rhi.Argument;
    // If a file name was given, use it.  Otherwise, default to the
    // given stream.
    cmsys::ofstream fout;
    std::ostream* s = &os;
    if (!rhi.Filename.empty()) {
      fout.open(rhi.Filename.c_str());
      s = &fout;
    } else if (++count > 1) {
      os << "\n\n";
    }

    // Print this documentation type to the stream.
    if (!this->PrintDocumentation(rhi.HelpType, *s) || s->fail()) {
      result = false;
    }
  }
  return result;
}

#define GET_OPT_ARGUMENT(target)                                              \
  if ((i + 1 < argc) && !this->IsOption(argv[i + 1])) {                       \
    (target) = argv[i + 1];                                                   \
    i = i + 1;                                                                \
  };

void cmDocumentation::WarnFormFromFilename(
  cmDocumentation::RequestedHelpItem& request, bool& result)
{
  std::string ext = cmSystemTools::GetFilenameLastExtension(request.Filename);
  ext = cmSystemTools::UpperCase(ext);
  if ((ext == ".HTM") || (ext == ".HTML")) {
    request.HelpType = cmDocumentation::None;
    result = true;
    cmSystemTools::Message("Warning: HTML help format no longer supported");
  } else if (ext == ".DOCBOOK") {
    request.HelpType = cmDocumentation::None;
    result = true;
    cmSystemTools::Message("Warning: Docbook help format no longer supported");
  }
  // ".1" to ".9" should be manpages
  else if ((ext.length() == 2) && (ext[1] >= '1') && (ext[1] <= '9')) {
    request.HelpType = cmDocumentation::None;
    result = true;
    cmSystemTools::Message("Warning: Man help format no longer supported");
  }
}

void cmDocumentation::addCommonStandardDocSections()
{
  cmDocumentationSection* sec;

  sec = new cmDocumentationSection("Options", "OPTIONS");
  sec->Append(cmDocumentationStandardOptions);
  this->AllSections["Options"] = sec;
}

void cmDocumentation::addCMakeStandardDocSections()
{
  cmDocumentationSection* sec;

  sec = new cmDocumentationSection("Generators", "GENERATORS");
  sec->Append(cmDocumentationGeneratorsHeader);
  this->AllSections["Generators"] = sec;
}

void cmDocumentation::addCTestStandardDocSections()
{
  // This is currently done for backward compatibility reason
  // We may suppress some of these.
  addCMakeStandardDocSections();
}

void cmDocumentation::addCPackStandardDocSections()
{
  cmDocumentationSection* sec;

  sec = new cmDocumentationSection("Generators", "GENERATORS");
  sec->Append(cmDocumentationGeneratorsHeader);
  this->AllSections["Generators"] = sec;
}

bool cmDocumentation::CheckOptions(int argc, const char* const* argv,
                                   const char* exitOpt)
{
  // Providing zero arguments gives usage information.
  if (argc == 1) {
    RequestedHelpItem help;
    help.HelpType = cmDocumentation::Usage;
    this->RequestedHelpItems.push_back(std::move(help));
    return true;
  }

  // Search for supported help options.

  bool result = false;
  for (int i = 1; i < argc; ++i) {
    if (exitOpt && strcmp(argv[i], exitOpt) == 0) {
      return result;
    }
    RequestedHelpItem help;
    // Check if this is a supported help option.
    if ((strcmp(argv[i], "-help") == 0) || (strcmp(argv[i], "--help") == 0) ||
        (strcmp(argv[i], "/?") == 0) || (strcmp(argv[i], "-usage") == 0) ||
        (strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-H") == 0)) {
      help.HelpType = cmDocumentation::Help;
      GET_OPT_ARGUMENT(help.Argument);
      help.Argument = cmSystemTools::LowerCase(help.Argument);
      // special case for single command
      if (!help.Argument.empty()) {
        help.HelpType = cmDocumentation::OneCommand;
      }
    } else if (strcmp(argv[i], "--help-properties") == 0) {
      help.HelpType = cmDocumentation::OneManual;
      help.Argument = "cmake-properties.7";
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-policies") == 0) {
      help.HelpType = cmDocumentation::OneManual;
      help.Argument = "cmake-policies.7";
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-variables") == 0) {
      help.HelpType = cmDocumentation::OneManual;
      help.Argument = "cmake-variables.7";
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-modules") == 0) {
      help.HelpType = cmDocumentation::OneManual;
      help.Argument = "cmake-modules.7";
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-custom-modules") == 0) {
      GET_OPT_ARGUMENT(help.Filename);
      cmSystemTools::Message(
        "Warning: --help-custom-modules no longer supported");
      if (help.Filename.empty()) {
        return true;
      }
      // Avoid breaking old project builds completely by at least generating
      // the output file.  Abuse help.Argument to give the file name to
      // PrintOldCustomModules without disrupting our internal API.
      help.HelpType = cmDocumentation::OldCustomModules;
      help.Argument = cmSystemTools::GetFilenameName(help.Filename);
    } else if (strcmp(argv[i], "--help-commands") == 0) {
      help.HelpType = cmDocumentation::OneManual;
      help.Argument = "cmake-commands.7";
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-compatcommands") == 0) {
      GET_OPT_ARGUMENT(help.Filename);
      cmSystemTools::Message(
        "Warning: --help-compatcommands no longer supported");
      return true;
    } else if (strcmp(argv[i], "--help-full") == 0) {
      help.HelpType = cmDocumentation::Full;
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-html") == 0) {
      GET_OPT_ARGUMENT(help.Filename);
      cmSystemTools::Message("Warning: --help-html no longer supported");
      return true;
    } else if (strcmp(argv[i], "--help-man") == 0) {
      GET_OPT_ARGUMENT(help.Filename);
      cmSystemTools::Message("Warning: --help-man no longer supported");
      return true;
    } else if (strcmp(argv[i], "--help-command") == 0) {
      help.HelpType = cmDocumentation::OneCommand;
      GET_OPT_ARGUMENT(help.Argument);
      GET_OPT_ARGUMENT(help.Filename);
      help.Argument = cmSystemTools::LowerCase(help.Argument);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-module") == 0) {
      help.HelpType = cmDocumentation::OneModule;
      GET_OPT_ARGUMENT(help.Argument);
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-property") == 0) {
      help.HelpType = cmDocumentation::OneProperty;
      GET_OPT_ARGUMENT(help.Argument);
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-policy") == 0) {
      help.HelpType = cmDocumentation::OnePolicy;
      GET_OPT_ARGUMENT(help.Argument);
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-variable") == 0) {
      help.HelpType = cmDocumentation::OneVariable;
      GET_OPT_ARGUMENT(help.Argument);
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-manual") == 0) {
      help.HelpType = cmDocumentation::OneManual;
      GET_OPT_ARGUMENT(help.Argument);
      GET_OPT_ARGUMENT(help.Filename);
      this->WarnFormFromFilename(help, result);
    } else if (strcmp(argv[i], "--help-command-list") == 0) {
      help.HelpType = cmDocumentation::ListCommands;
      GET_OPT_ARGUMENT(help.Filename);
    } else if (strcmp(argv[i], "--help-module-list") == 0) {
      help.HelpType = cmDocumentation::ListModules;
      GET_OPT_ARGUMENT(help.Filename);
    } else if (strcmp(argv[i], "--help-property-list") == 0) {
      help.HelpType = cmDocumentation::ListProperties;
      GET_OPT_ARGUMENT(help.Filename);
    } else if (strcmp(argv[i], "--help-variable-list") == 0) {
      help.HelpType = cmDocumentation::ListVariables;
      GET_OPT_ARGUMENT(help.Filename);
    } else if (strcmp(argv[i], "--help-policy-list") == 0) {
      help.HelpType = cmDocumentation::ListPolicies;
      GET_OPT_ARGUMENT(help.Filename);
    } else if (strcmp(argv[i], "--help-manual-list") == 0) {
      help.HelpType = cmDocumentation::ListManuals;
      GET_OPT_ARGUMENT(help.Filename);
    } else if (strcmp(argv[i], "--copyright") == 0) {
      GET_OPT_ARGUMENT(help.Filename);
      cmSystemTools::Message("Warning: --copyright no longer supported");
      return true;
    } else if ((strcmp(argv[i], "--version") == 0) ||
               (strcmp(argv[i], "-version") == 0) ||
               (strcmp(argv[i], "/V") == 0)) {
      help.HelpType = cmDocumentation::Version;
      GET_OPT_ARGUMENT(help.Filename);
    }
    if (help.HelpType != None) {
      // This is a help option.  See if there is a file name given.
      result = true;
      this->RequestedHelpItems.push_back(std::move(help));
    }
  }
  return result;
}

void cmDocumentation::SetName(const std::string& name)
{
  this->NameString = name;
}

void cmDocumentation::SetSection(const char* name,
                                 cmDocumentationSection* section)
{
  if (this->AllSections.find(name) != this->AllSections.end()) {
    delete this->AllSections[name];
  }
  this->AllSections[name] = section;
}

void cmDocumentation::SetSection(const char* name,
                                 std::vector<cmDocumentationEntry>& docs)
{
  cmDocumentationSection* sec =
    new cmDocumentationSection(name, cmSystemTools::UpperCase(name).c_str());
  sec->Append(docs);
  this->SetSection(name, sec);
}

void cmDocumentation::SetSection(const char* name, const char* docs[][2])
{
  cmDocumentationSection* sec =
    new cmDocumentationSection(name, cmSystemTools::UpperCase(name).c_str());
  sec->Append(docs);
  this->SetSection(name, sec);
}

void cmDocumentation::SetSections(
  std::map<std::string, cmDocumentationSection*>& sections)
{
  for (auto const& s : sections) {
    this->SetSection(s.first.c_str(), s.second);
  }
}

void cmDocumentation::PrependSection(const char* name, const char* docs[][2])
{
  cmDocumentationSection* sec = nullptr;
  if (this->AllSections.find(name) == this->AllSections.end()) {
    sec =
      new cmDocumentationSection(name, cmSystemTools::UpperCase(name).c_str());
    this->SetSection(name, sec);
  } else {
    sec = this->AllSections[name];
  }
  sec->Prepend(docs);
}

void cmDocumentation::PrependSection(const char* name,
                                     std::vector<cmDocumentationEntry>& docs)
{
  cmDocumentationSection* sec = nullptr;
  if (this->AllSections.find(name) == this->AllSections.end()) {
    sec =
      new cmDocumentationSection(name, cmSystemTools::UpperCase(name).c_str());
    this->SetSection(name, sec);
  } else {
    sec = this->AllSections[name];
  }
  sec->Prepend(docs);
}

void cmDocumentation::AppendSection(const char* name, const char* docs[][2])
{
  cmDocumentationSection* sec = nullptr;
  if (this->AllSections.find(name) == this->AllSections.end()) {
    sec =
      new cmDocumentationSection(name, cmSystemTools::UpperCase(name).c_str());
    this->SetSection(name, sec);
  } else {
    sec = this->AllSections[name];
  }
  sec->Append(docs);
}

void cmDocumentation::AppendSection(const char* name,
                                    std::vector<cmDocumentationEntry>& docs)
{
  cmDocumentationSection* sec = nullptr;
  if (this->AllSections.find(name) == this->AllSections.end()) {
    sec =
      new cmDocumentationSection(name, cmSystemTools::UpperCase(name).c_str());
    this->SetSection(name, sec);
  } else {
    sec = this->AllSections[name];
  }
  sec->Append(docs);
}

void cmDocumentation::AppendSection(const char* name,
                                    cmDocumentationEntry& docs)
{

  std::vector<cmDocumentationEntry> docsVec;
  docsVec.push_back(docs);
  this->AppendSection(name, docsVec);
}

void cmDocumentation::PrependSection(const char* name,
                                     cmDocumentationEntry& docs)
{

  std::vector<cmDocumentationEntry> docsVec;
  docsVec.push_back(docs);
  this->PrependSection(name, docsVec);
}

void cmDocumentation::GlobHelp(std::vector<std::string>& files,
                               std::string const& pattern)
{
  cmsys::Glob gl;
  std::string findExpr =
    cmSystemTools::GetCMakeRoot() + "/Help/" + pattern + ".rst";
  if (gl.FindFiles(findExpr)) {
    files = gl.GetFiles();
  }
}

void cmDocumentation::PrintNames(std::ostream& os, std::string const& pattern)
{
  std::vector<std::string> files;
  this->GlobHelp(files, pattern);
  std::vector<std::string> names;
  for (std::string const& f : files) {
    std::string line;
    cmsys::ifstream fin(f.c_str());
    while (fin && cmSystemTools::GetLineFromStream(fin, line)) {
      if (!line.empty() && (isalnum(line[0]) || line[0] == '<')) {
        names.push_back(line);
        break;
      }
    }
  }
  std::sort(names.begin(), names.end());
  for (std::string const& n : names) {
    os << n << "\n";
  }
}

bool cmDocumentation::PrintFiles(std::ostream& os, std::string const& pattern)
{
  bool found = false;
  std::vector<std::string> files;
  this->GlobHelp(files, pattern);
  std::sort(files.begin(), files.end());
  cmRST r(os, cmSystemTools::GetCMakeRoot() + "/Help");
  for (std::string const& f : files) {
    found = r.ProcessFile(f) || found;
  }
  return found;
}

bool cmDocumentation::PrintHelpFull(std::ostream& os)
{
  return this->PrintFiles(os, "index");
}

bool cmDocumentation::PrintHelpOneManual(std::ostream& os)
{
  std::string mname = this->CurrentArgument;
  std::string::size_type mlen = mname.length();
  if (mlen > 3 && mname[mlen - 3] == '(' && mname[mlen - 1] == ')') {
    mname = mname.substr(0, mlen - 3) + "." + mname[mlen - 2];
  }
  if (this->PrintFiles(os, "manual/" + mname) ||
      this->PrintFiles(os, "manual/" + mname + ".[0-9]")) {
    return true;
  }
  // Argument was not a manual.  Complain.
  os << "Argument \"" << this->CurrentArgument
     << "\" to --help-manual is not an available manual.  "
     << "Use --help-manual-list to see all available manuals.\n";
  return false;
}

bool cmDocumentation::PrintHelpListManuals(std::ostream& os)
{
  this->PrintNames(os, "manual/*");
  return true;
}

bool cmDocumentation::PrintHelpOneCommand(std::ostream& os)
{
  std::string cname = cmSystemTools::LowerCase(this->CurrentArgument);
  if (this->PrintFiles(os, "command/" + cname)) {
    return true;
  }
  // Argument was not a command.  Complain.
  os << "Argument \"" << this->CurrentArgument
     << "\" to --help-command is not a CMake command.  "
     << "Use --help-command-list to see all commands.\n";
  return false;
}

bool cmDocumentation::PrintHelpListCommands(std::ostream& os)
{
  this->PrintNames(os, "command/*");
  return true;
}

bool cmDocumentation::PrintHelpOneModule(std::ostream& os)
{
  std::string mname = this->CurrentArgument;
  if (this->PrintFiles(os, "module/" + mname)) {
    return true;
  }
  // Argument was not a module.  Complain.
  os << "Argument \"" << this->CurrentArgument
     << "\" to --help-module is not a CMake module.\n";
  return false;
}

bool cmDocumentation::PrintHelpListModules(std::ostream& os)
{
  std::vector<std::string> files;
  this->GlobHelp(files, "module/*");
  std::vector<std::string> modules;
  for (std::string const& f : files) {
    std::string module = cmSystemTools::GetFilenameName(f);
    modules.push_back(module.substr(0, module.size() - 4));
  }
  std::sort(modules.begin(), modules.end());
  for (std::string const& m : modules) {
    os << m << "\n";
  }
  return true;
}

bool cmDocumentation::PrintHelpOneProperty(std::ostream& os)
{
  std::string pname = cmSystemTools::HelpFileName(this->CurrentArgument);
  if (this->PrintFiles(os, "prop_*/" + pname)) {
    return true;
  }
  // Argument was not a property.  Complain.
  os << "Argument \"" << this->CurrentArgument
     << "\" to --help-property is not a CMake property.  "
     << "Use --help-property-list to see all properties.\n";
  return false;
}

bool cmDocumentation::PrintHelpListProperties(std::ostream& os)
{
  this->PrintNames(os, "prop_*/*");
  return true;
}

bool cmDocumentation::PrintHelpOnePolicy(std::ostream& os)
{
  std::string pname = this->CurrentArgument;
  std::vector<std::string> files;
  if (this->PrintFiles(os, "policy/" + pname)) {
    return true;
  }

  // Argument was not a policy.  Complain.
  os << "Argument \"" << this->CurrentArgument
     << "\" to --help-policy is not a CMake policy.\n";
  return false;
}

bool cmDocumentation::PrintHelpListPolicies(std::ostream& os)
{
  this->PrintNames(os, "policy/*");
  return true;
}

bool cmDocumentation::PrintHelpListGenerators(std::ostream& os)
{
  std::map<std::string, cmDocumentationSection*>::iterator si;
  si = this->AllSections.find("Generators");
  if (si != this->AllSections.end()) {
    this->Formatter.SetIndent("  ");
    this->Formatter.PrintSection(os, *si->second);
  }
  return true;
}

bool cmDocumentation::PrintHelpOneVariable(std::ostream& os)
{
  std::string vname = cmSystemTools::HelpFileName(this->CurrentArgument);
  if (this->PrintFiles(os, "variable/" + vname)) {
    return true;
  }
  // Argument was not a variable.  Complain.
  os << "Argument \"" << this->CurrentArgument
     << "\" to --help-variable is not a defined variable.  "
     << "Use --help-variable-list to see all defined variables.\n";
  return false;
}

bool cmDocumentation::PrintHelpListVariables(std::ostream& os)
{
  this->PrintNames(os, "variable/*");
  return true;
}

bool cmDocumentation::PrintUsage(std::ostream& os)
{
  std::map<std::string, cmDocumentationSection*>::iterator si;
  si = this->AllSections.find("Usage");
  if (si != this->AllSections.end()) {
    this->Formatter.PrintSection(os, *si->second);
  }
  return true;
}

bool cmDocumentation::PrintHelp(std::ostream& os)
{
  std::map<std::string, cmDocumentationSection*>::iterator si;
  si = this->AllSections.find("Usage");
  if (si != this->AllSections.end()) {
    this->Formatter.PrintSection(os, *si->second);
  }
  si = this->AllSections.find("Options");
  if (si != this->AllSections.end()) {
    this->Formatter.PrintSection(os, *si->second);
  }
  if (this->ShowGenerators) {
    si = this->AllSections.find("Generators");
    if (si != this->AllSections.end()) {
      this->Formatter.PrintSection(os, *si->second);
    }
  }
  return true;
}

const char* cmDocumentation::GetNameString() const
{
  if (!this->NameString.empty()) {
    return this->NameString.c_str();
  }
  return "CMake";
}

bool cmDocumentation::IsOption(const char* arg) const
{
  return ((arg[0] == '-') || (strcmp(arg, "/V") == 0) ||
          (strcmp(arg, "/?") == 0));
}

bool cmDocumentation::PrintOldCustomModules(std::ostream& os)
{
  // CheckOptions abuses the Argument field to give us the file name.
  std::string filename = this->CurrentArgument;
  std::string ext = cmSystemTools::UpperCase(
    cmSystemTools::GetFilenameLastExtension(filename));
  std::string name = cmSystemTools::GetFilenameWithoutLastExtension(filename);

  const char* summary = "cmake --help-custom-modules no longer supported\n";
  const char* detail =
    "CMake versions prior to 3.0 exposed their internal module help page\n"
    "generation functionality through the --help-custom-modules option.\n"
    "CMake versions 3.0 and above use other means to generate their module\n"
    "help pages so this functionality is no longer available to be exposed.\n"
    "\n"
    "This file was generated as a placeholder to provide this information.\n";
  if ((ext == ".HTM") || (ext == ".HTML")) {
    os << "<html><title>" << name << "</title><body>\n"
       << summary << "<p/>\n"
       << detail << "</body></html>\n";
  } else if ((ext.length() == 2) && (ext[1] >= '1') && (ext[1] <= '9')) {
    /* clang-format off */
    os <<
      ".TH " << name << " " << ext[1] << " \"" <<
      cmSystemTools::GetCurrentDateTime("%B %d, %Y") <<
      "\" \"cmake " << cmVersion::GetCMakeVersion() << "\"\n"
      ".SH NAME\n"
      ".PP\n" <<
      name << " \\- " << summary <<
      "\n"
      ".SH DESCRIPTION\n"
      ".PP\n" <<
      detail
      ;
    /* clang-format on */
  } else {
    os << name << "\n\n" << summary << "\n" << detail;
  }
  return true;
}
