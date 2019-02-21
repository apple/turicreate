/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmVisualStudio10TargetGenerator.h"

#include "cmAlgorithms.h"
#include "cmComputeLinkInformation.h"
#include "cmCustomCommandGenerator.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalVisualStudio10Generator.h"
#include "cmLocalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"
#include "cmVisualStudioGeneratorOptions.h"
#include "windows.h"

#include <iterator>
#include <memory> // IWYU pragma: keep

static void ConvertToWindowsSlash(std::string& s);

static std::string cmVS10EscapeXML(std::string arg)
{
  cmSystemTools::ReplaceString(arg, "&", "&amp;");
  cmSystemTools::ReplaceString(arg, "<", "&lt;");
  cmSystemTools::ReplaceString(arg, ">", "&gt;");
  return arg;
}

static std::string cmVS10EscapeAttr(std::string arg)
{
  cmSystemTools::ReplaceString(arg, "&", "&amp;");
  cmSystemTools::ReplaceString(arg, "<", "&lt;");
  cmSystemTools::ReplaceString(arg, ">", "&gt;");
  cmSystemTools::ReplaceString(arg, "\"", "&quot;");
  return arg;
}

struct cmVisualStudio10TargetGenerator::Elem
{
  std::ostream& S;
  const int Indent;
  bool HasElements = false;
  bool HasContent = false;
  std::string Tag;

  Elem(std::ostream& s)
    : S(s)
    , Indent(0)
  {
  }
  Elem(const Elem&) = delete;
  Elem(Elem& par)
    : S(par.S)
    , Indent(par.Indent + 1)
  {
    par.SetHasElements();
  }
  Elem(Elem& par, const char* tag)
    : S(par.S)
    , Indent(par.Indent + 1)
  {
    par.SetHasElements();
    this->StartElement(tag);
  }
  void SetHasElements()
  {
    if (!HasElements) {
      this->S << ">\n";
      HasElements = true;
    }
  }
  std::ostream& WriteString(const char* line);
  Elem& StartElement(const std::string& tag)
  {
    this->Tag = tag;
    this->WriteString("<") << tag;
    return *this;
  }
  void Element(const char* tag, const std::string& val)
  {
    Elem(*this, tag).Content(val);
  }
  Elem& Attribute(const char* an, const std::string& av)
  {
    this->S << " " << an << "=\"" << cmVS10EscapeAttr(av) << "\"";
    return *this;
  }
  // This method for now assumes that this->Tag has been set, e.g. by calling
  // StartElement().
  void Content(const std::string& val)
  {
    if (!this->HasContent) {
      this->S << ">";
      this->HasContent = true;
    }
    this->S << cmVS10EscapeXML(val);
  }
  ~Elem()
  {
    // Do not emit element which has not been started
    if (Tag.empty()) {
      return;
    }

    if (HasElements) {
      this->WriteString("</") << this->Tag << ">";
      if (this->Indent > 0) {
        this->S << '\n';
      } else {
        // special case: don't print EOL at EOF
      }
    } else if (HasContent) {
      this->S << "</" << this->Tag << ">\n";
    } else {
      this->S << " />\n";
    }
  }

  void WritePlatformConfigTag(const char* tag, const std::string& cond,
                              const std::string& content);
};

class cmVS10GeneratorOptions : public cmVisualStudioGeneratorOptions
{
public:
  typedef cmVisualStudio10TargetGenerator::Elem Elem;
  cmVS10GeneratorOptions(cmLocalVisualStudioGenerator* lg, Tool tool,
                         cmVS7FlagTable const* table,
                         cmVisualStudio10TargetGenerator* g = nullptr)
    : cmVisualStudioGeneratorOptions(lg, tool, table)
    , TargetGenerator(g)
  {
  }

  void OutputFlag(std::ostream& /*fout*/, int /*indent*/, const char* tag,
                  const std::string& content) override
  {
    if (!this->GetConfiguration().empty()) {
      // if there are configuration specific flags, then
      // use the configuration specific tag for PreprocessorDefinitions
      const std::string cond =
        this->TargetGenerator->CalcCondition(this->GetConfiguration());
      this->Parent->WritePlatformConfigTag(tag, cond, content);
    } else {
      this->Parent->Element(tag, content);
    }
  }

private:
  cmVisualStudio10TargetGenerator* const TargetGenerator;
  Elem* Parent = nullptr;
  friend cmVisualStudio10TargetGenerator::OptionsHelper;
};

struct cmVisualStudio10TargetGenerator::OptionsHelper
{
  cmVS10GeneratorOptions& O;
  OptionsHelper(cmVS10GeneratorOptions& o, Elem& e)
    : O(o)
  {
    O.Parent = &e;
  }
  ~OptionsHelper() { O.Parent = nullptr; }

  void OutputPreprocessorDefinitions(const std::string& lang)
  {
    O.OutputPreprocessorDefinitions(O.Parent->S, O.Parent->Indent + 1, lang);
  }
  void OutputAdditionalIncludeDirectories(const std::string& lang)
  {
    O.OutputAdditionalIncludeDirectories(O.Parent->S, O.Parent->Indent + 1,
                                         lang);
  }
  void OutputFlagMap() { O.OutputFlagMap(O.Parent->S, O.Parent->Indent + 1); }
  void PrependInheritedString(std::string const& key)
  {
    O.PrependInheritedString(key);
  }
};

static std::string cmVS10EscapeComment(std::string comment)
{
  // MSBuild takes the CDATA of a <Message></Message> element and just
  // does "echo $CDATA" with no escapes.  We must encode the string.
  // http://technet.microsoft.com/en-us/library/cc772462%28WS.10%29.aspx
  std::string echoable;
  for (char c : comment) {
    switch (c) {
      case '\r':
        break;
      case '\n':
        echoable += '\t';
        break;
      case '"': /* no break */
      case '|': /* no break */
      case '&': /* no break */
      case '<': /* no break */
      case '>': /* no break */
      case '^':
        echoable += '^'; /* no break */
        CM_FALLTHROUGH;
      default:
        echoable += c;
        break;
    }
  }
  return echoable;
}

static bool cmVS10IsTargetsFile(std::string const& path)
{
  std::string const ext = cmSystemTools::GetFilenameLastExtension(path);
  return cmSystemTools::Strucmp(ext.c_str(), ".targets") == 0;
}

static std::string computeProjectFileExtension(cmGeneratorTarget const* t)
{
  std::string res;
  res = ".vcxproj";
  if (t->IsCSharpOnly()) {
    res = ".csproj";
  }
  return res;
}

cmVisualStudio10TargetGenerator::cmVisualStudio10TargetGenerator(
  cmGeneratorTarget* target, cmGlobalVisualStudio10Generator* gg)
  : GeneratorTarget(target)
  , Makefile(target->Target->GetMakefile())
  , Platform(gg->GetPlatformName())
  , Name(target->GetName())
  , GUID(gg->GetGUID(this->Name))
  , GlobalGenerator(gg)
  , LocalGenerator(
      (cmLocalVisualStudio10Generator*)target->GetLocalGenerator())
{
  this->Makefile->GetConfigurations(this->Configurations);
  this->NsightTegra = gg->IsNsightTegra();
  for (int i = 0; i < 4; ++i) {
    this->NsightTegraVersion[i] = 0;
  }
  sscanf(gg->GetNsightTegraVersion().c_str(), "%u.%u.%u.%u",
         &this->NsightTegraVersion[0], &this->NsightTegraVersion[1],
         &this->NsightTegraVersion[2], &this->NsightTegraVersion[3]);
  this->MSTools = !this->NsightTegra;
  this->Managed = false;
  this->TargetCompileAsWinRT = false;
  this->IsMissingFiles = false;
  this->DefaultArtifactDir =
    this->LocalGenerator->GetCurrentBinaryDirectory() + "/" +
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  this->InSourceBuild = (this->Makefile->GetCurrentSourceDirectory() ==
                         this->Makefile->GetCurrentBinaryDirectory());
}

cmVisualStudio10TargetGenerator::~cmVisualStudio10TargetGenerator()
{
}

std::string cmVisualStudio10TargetGenerator::CalcCondition(
  const std::string& config) const
{
  std::ostringstream oss;
  oss << "'$(Configuration)|$(Platform)'=='";
  oss << config << "|" << this->Platform;
  oss << "'";
  // handle special case for 32 bit C# targets
  if (this->ProjectType == csproj && this->Platform == "Win32") {
    oss << " Or ";
    oss << "'$(Configuration)|$(Platform)'=='";
    oss << config << "|x86";
    oss << "'";
  }
  return oss.str();
}

void cmVisualStudio10TargetGenerator::Elem::WritePlatformConfigTag(
  const char* tag, const std::string& cond, const std::string& content)
{
  Elem(*this, tag).Attribute("Condition", cond).Content(content);
}

std::ostream& cmVisualStudio10TargetGenerator::Elem::WriteString(
  const char* line)
{
  this->S.fill(' ');
  this->S.width(this->Indent * 2);
  // write an empty string to get the fill level indent to print
  this->S << "";
  this->S << line;
  return this->S;
}

#define VS10_CXX_DEFAULT_PROPS "$(VCTargetsPath)\\Microsoft.Cpp.Default.props"
#define VS10_CXX_PROPS "$(VCTargetsPath)\\Microsoft.Cpp.props"
#define VS10_CXX_USER_PROPS                                                   \
  "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props"
#define VS10_CXX_TARGETS "$(VCTargetsPath)\\Microsoft.Cpp.targets"

#define VS10_CSharp_DEFAULT_PROPS                                             \
  "$(MSBuildExtensionsPath)\\$(MSBuildToolsVersion)\\Microsoft.Common.props"
// This does not seem to exist by default, it's just provided for consistency
// in case users want to have default custom props for C# targets
#define VS10_CSharp_USER_PROPS                                                \
  "$(UserRootDir)\\Microsoft.CSharp.$(Platform).user.props"
#define VS10_CSharp_TARGETS "$(MSBuildToolsPath)\\Microsoft.CSharp.targets"

void cmVisualStudio10TargetGenerator::Generate()
{
  // do not generate external ms projects
  if (this->GeneratorTarget->GetType() == cmStateEnums::INTERFACE_LIBRARY ||
      this->GeneratorTarget->GetProperty("EXTERNAL_MSPROJECT")) {
    return;
  }
  const std::string ProjectFileExtension =
    computeProjectFileExtension(this->GeneratorTarget);
  if (ProjectFileExtension == ".vcxproj") {
    this->ProjectType = vcxproj;
    this->Managed = false;
  } else if (ProjectFileExtension == ".csproj") {
    if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY) {
      std::string message = "The C# target \"" +
        this->GeneratorTarget->GetName() +
        "\" is of type STATIC_LIBRARY. This is discouraged (and may be "
        "disabled in future). Make it a SHARED library instead.";
      this->Makefile->IssueMessage(cmake::MessageType::DEPRECATION_WARNING,
                                   message);
    }
    this->ProjectType = csproj;
    this->Managed = true;
  }
  // Tell the global generator the name of the project file
  this->GeneratorTarget->Target->SetProperty("GENERATOR_FILE_NAME",
                                             this->Name.c_str());
  this->GeneratorTarget->Target->SetProperty("GENERATOR_FILE_NAME_EXT",
                                             ProjectFileExtension.c_str());
  this->DotNetHintReferences.clear();
  this->AdditionalUsingDirectories.clear();
  if (this->GeneratorTarget->GetType() <= cmStateEnums::OBJECT_LIBRARY) {
    if (!this->ComputeClOptions()) {
      return;
    }
    if (!this->ComputeRcOptions()) {
      return;
    }
    if (!this->ComputeCudaOptions()) {
      return;
    }
    if (!this->ComputeCudaLinkOptions()) {
      return;
    }
    if (!this->ComputeMasmOptions()) {
      return;
    }
    if (!this->ComputeNasmOptions()) {
      return;
    }
    if (!this->ComputeLinkOptions()) {
      return;
    }
    if (!this->ComputeLibOptions()) {
      return;
    }
  }
  std::string path = this->LocalGenerator->GetCurrentBinaryDirectory();
  path += "/";
  path += this->Name;
  path += ProjectFileExtension;
  cmGeneratedFileStream BuildFileStream(path);
  const std::string PathToProjectFile = path;
  BuildFileStream.SetCopyIfDifferent(true);

  // Write the encoding header into the file
  char magic[] = { char(0xEF), char(0xBB), char(0xBF) };
  BuildFileStream.write(magic, 3);
  BuildFileStream << "<?xml version=\"1.0\" encoding=\""
                  << this->GlobalGenerator->Encoding() << "\"?>"
                  << "\n";
  {
    Elem e0(BuildFileStream);
    e0.StartElement("Project");
    e0.Attribute("DefaultTargets", "Build");
    e0.Attribute("ToolsVersion", this->GlobalGenerator->GetToolsVersion());
    e0.Attribute("xmlns",
                 "http://schemas.microsoft.com/developer/msbuild/2003");

    if (this->NsightTegra) {
      Elem e1(e0, "PropertyGroup");
      e1.Attribute("Label", "NsightTegraProject");
      const unsigned int nsightTegraMajorVersion = this->NsightTegraVersion[0];
      const unsigned int nsightTegraMinorVersion = this->NsightTegraVersion[1];
      if (nsightTegraMajorVersion >= 2) {
        if (nsightTegraMajorVersion > 3 ||
            (nsightTegraMajorVersion == 3 && nsightTegraMinorVersion >= 1)) {
          e1.Element("NsightTegraProjectRevisionNumber", "11");
        } else {
          // Nsight Tegra 2.0 uses project revision 9.
          e1.Element("NsightTegraProjectRevisionNumber", "9");
        }
        // Tell newer versions to upgrade silently when loading.
        e1.Element("NsightTegraUpgradeOnceWithoutPrompt", "true");
      } else {
        // Require Nsight Tegra 1.6 for JCompile support.
        e1.Element("NsightTegraProjectRevisionNumber", "7");
      }
    }

    if (const char* hostArch =
          this->GlobalGenerator->GetPlatformToolsetHostArchitecture()) {
      Elem e1(e0, "PropertyGroup");
      e1.Element("PreferredToolArchitecture", hostArch);
    }

    if (this->ProjectType != csproj) {
      this->WriteProjectConfigurations(e0);
    }

    {
      Elem e1(e0, "PropertyGroup");
      e1.Attribute("Label", "Globals");
      e1.Element("ProjectGuid", "{" + this->GUID + "}");

      if (this->MSTools &&
          this->GeneratorTarget->GetType() <= cmStateEnums::GLOBAL_TARGET) {
        this->WriteApplicationTypeSettings(e1);
        this->VerifyNecessaryFiles();
      }

      const char* vsProjectTypes =
        this->GeneratorTarget->GetProperty("VS_GLOBAL_PROJECT_TYPES");
      if (vsProjectTypes) {
        const char* tagName = "ProjectTypes";
        if (this->ProjectType == csproj) {
          tagName = "ProjectTypeGuids";
        }
        e1.Element(tagName, vsProjectTypes);
      }

      const char* vsProjectName =
        this->GeneratorTarget->GetProperty("VS_SCC_PROJECTNAME");
      const char* vsLocalPath =
        this->GeneratorTarget->GetProperty("VS_SCC_LOCALPATH");
      const char* vsProvider =
        this->GeneratorTarget->GetProperty("VS_SCC_PROVIDER");

      if (vsProjectName && vsLocalPath && vsProvider) {
        e1.Element("SccProjectName", vsProjectName);
        e1.Element("SccLocalPath", vsLocalPath);
        e1.Element("SccProvider", vsProvider);

        const char* vsAuxPath =
          this->GeneratorTarget->GetProperty("VS_SCC_AUXPATH");
        if (vsAuxPath) {
          e1.Element("SccAuxPath", vsAuxPath);
        }
      }

      if (this->GeneratorTarget->GetPropertyAsBool("VS_WINRT_COMPONENT")) {
        e1.Element("WinMDAssembly", "true");
      }

      const char* vsGlobalKeyword =
        this->GeneratorTarget->GetProperty("VS_GLOBAL_KEYWORD");
      if (!vsGlobalKeyword) {
        e1.Element("Keyword", "Win32Proj");
      } else {
        e1.Element("Keyword", vsGlobalKeyword);
      }

      const char* vsGlobalRootNamespace =
        this->GeneratorTarget->GetProperty("VS_GLOBAL_ROOTNAMESPACE");
      if (vsGlobalRootNamespace) {
        e1.Element("RootNamespace", vsGlobalRootNamespace);
      }

      e1.Element("Platform", this->Platform);
      const char* projLabel =
        this->GeneratorTarget->GetProperty("PROJECT_LABEL");
      if (!projLabel) {
        projLabel = this->Name.c_str();
      }
      e1.Element("ProjectName", projLabel);
      {
        // TODO: add deprecation warning for VS_* property?
        const char* targetFrameworkVersion =
          this->GeneratorTarget->GetProperty(
            "VS_DOTNET_TARGET_FRAMEWORK_VERSION");
        if (!targetFrameworkVersion) {
          targetFrameworkVersion = this->GeneratorTarget->GetProperty(
            "DOTNET_TARGET_FRAMEWORK_VERSION");
        }
        if (targetFrameworkVersion) {
          e1.Element("TargetFrameworkVersion", targetFrameworkVersion);
        }
      }

      // Disable the project upgrade prompt that is displayed the first time a
      // project using an older toolset version is opened in a newer version of
      // the IDE (respected by VS 2013 and above).
      if (this->GlobalGenerator->GetVersion() >=
          cmGlobalVisualStudioGenerator::VS12) {
        e1.Element("VCProjectUpgraderObjectName", "NoUpgrade");
      }

      std::vector<std::string> keys = this->GeneratorTarget->GetPropertyKeys();
      for (std::string const& keyIt : keys) {
        static const char* prefix = "VS_GLOBAL_";
        if (keyIt.find(prefix) != 0)
          continue;
        std::string globalKey = keyIt.substr(strlen(prefix));
        // Skip invalid or separately-handled properties.
        if (globalKey.empty() || globalKey == "PROJECT_TYPES" ||
            globalKey == "ROOTNAMESPACE" || globalKey == "KEYWORD") {
          continue;
        }
        const char* value = this->GeneratorTarget->GetProperty(keyIt);
        if (!value)
          continue;
        e1.Element(globalKey.c_str(), value);
      }

      if (this->Managed) {
        std::string outputType;
        switch (this->GeneratorTarget->GetType()) {
          case cmStateEnums::OBJECT_LIBRARY:
          case cmStateEnums::STATIC_LIBRARY:
          case cmStateEnums::SHARED_LIBRARY:
            outputType = "Library";
            break;
          case cmStateEnums::MODULE_LIBRARY:
            outputType = "Module";
            break;
          case cmStateEnums::EXECUTABLE:
            if (this->GeneratorTarget->Target->GetPropertyAsBool(
                  "WIN32_EXECUTABLE")) {
              outputType = "WinExe";
            } else {
              outputType = "Exe";
            }
            break;
          case cmStateEnums::UTILITY:
          case cmStateEnums::GLOBAL_TARGET:
            outputType = "Utility";
            break;
          case cmStateEnums::UNKNOWN_LIBRARY:
          case cmStateEnums::INTERFACE_LIBRARY:
            break;
        }
        e1.Element("OutputType", outputType);
        e1.Element("AppDesignerFolder", "Properties");
      }
    }

    switch (this->ProjectType) {
      case vcxproj:
        if (this->GlobalGenerator->GetPlatformToolsetVersion()) {
          Elem(e0, "Import")
            .Attribute("Project",
                       this->GlobalGenerator->GetAuxiliaryToolset());
        }
        Elem(e0, "Import").Attribute("Project", VS10_CXX_DEFAULT_PROPS);
        break;
      case csproj:
        Elem(e0, "Import")
          .Attribute("Project", VS10_CSharp_DEFAULT_PROPS)
          .Attribute("Condition", "Exists('" VS10_CSharp_DEFAULT_PROPS "')");
        break;
    }

    this->WriteProjectConfigurationValues(e0);

    if (this->ProjectType == vcxproj) {
      Elem(e0, "Import").Attribute("Project", VS10_CXX_PROPS);
    }
    {
      Elem e1(e0, "ImportGroup");
      e1.Attribute("Label", "ExtensionSettings");
      e1.SetHasElements();

      if (this->GlobalGenerator->IsCudaEnabled()) {
        Elem(e1, "Import")
          .Attribute("Project",
                     "$(VCTargetsPath)\\BuildCustomizations\\CUDA " +
                       this->GlobalGenerator->GetPlatformToolsetCudaString() +
                       ".props");
      }
      if (this->GlobalGenerator->IsMasmEnabled()) {
        Elem(e1, "Import")
          .Attribute("Project",
                     "$(VCTargetsPath)\\BuildCustomizations\\masm.props");
      }
      if (this->GlobalGenerator->IsNasmEnabled()) {
        // Always search in the standard modules location.
        std::string propsTemplate =
          GetCMakeFilePath("Templates/MSBuild/nasm.props.in");

        std::string propsLocal;
        propsLocal += this->DefaultArtifactDir;
        propsLocal += "\\nasm.props";
        ConvertToWindowsSlash(propsLocal);
        this->Makefile->ConfigureFile(propsTemplate.c_str(),
                                      propsLocal.c_str(), false, true, true);
        Elem(e1, "Import").Attribute("Project", propsLocal);
      }
    }
    {
      Elem e1(e0, "ImportGroup");
      e1.Attribute("Label", "PropertySheets");
      std::string props;
      switch (this->ProjectType) {
        case vcxproj:
          props = VS10_CXX_USER_PROPS;
          break;
        case csproj:
          props = VS10_CSharp_USER_PROPS;
          break;
      }
      if (const char* p =
            this->GeneratorTarget->GetProperty("VS_USER_PROPS")) {
        props = p;
      }
      if (!props.empty()) {
        ConvertToWindowsSlash(props);
        Elem(e1, "Import")
          .Attribute("Project", props)
          .Attribute("Condition", "exists('" + props + "')")
          .Attribute("Label", "LocalAppDataPlatform");
      }

      this->WritePlatformExtensions(e1);
    }
    Elem(e0, "PropertyGroup").Attribute("Label", "UserMacros");
    this->WriteWinRTPackageCertificateKeyFile(e0);
    this->WritePathAndIncrementalLinkOptions(e0);
    this->WriteItemDefinitionGroups(e0);
    this->WriteCustomCommands(e0);
    this->WriteAllSources(e0);
    this->WriteDotNetReferences(e0);
    this->WriteEmbeddedResourceGroup(e0);
    this->WriteXamlFilesGroup(e0);
    this->WriteWinRTReferences(e0);
    this->WriteProjectReferences(e0);
    this->WriteSDKReferences(e0);
    switch (this->ProjectType) {
      case vcxproj:
        Elem(e0, "Import").Attribute("Project", VS10_CXX_TARGETS);
        break;
      case csproj:
        Elem(e0, "Import").Attribute("Project", VS10_CSharp_TARGETS);
        break;
    }

    this->WriteTargetSpecificReferences(e0);
    {
      Elem e1(e0, "ImportGroup");
      e1.Attribute("Label", "ExtensionTargets");
      e1.SetHasElements();
      this->WriteTargetsFileReferences(e1);
      if (this->GlobalGenerator->IsCudaEnabled()) {
        Elem(e1, "Import")
          .Attribute("Project",
                     "$(VCTargetsPath)\\BuildCustomizations\\CUDA " +
                       this->GlobalGenerator->GetPlatformToolsetCudaString() +
                       ".targets");
      }
      if (this->GlobalGenerator->IsMasmEnabled()) {
        Elem(e1, "Import")
          .Attribute("Project",
                     "$(VCTargetsPath)\\BuildCustomizations\\masm.targets");
      }
      if (this->GlobalGenerator->IsNasmEnabled()) {
        std::string nasmTargets =
          GetCMakeFilePath("Templates/MSBuild/nasm.targets");
        Elem(e1, "Import").Attribute("Project", nasmTargets);
      }
    }
    if (this->ProjectType == csproj) {
      for (std::string const& c : this->Configurations) {
        Elem e1(e0, "PropertyGroup");
        e1.Attribute("Condition", "'$(Configuration)' == '" + c + "'");
        e1.SetHasElements();
        this->WriteEvents(e1, c);
      }
      // make sure custom commands are executed before build (if necessary)
      {
        Elem e1(e0, "PropertyGroup");
        std::ostringstream oss;
        oss << "\n";
        for (std::string const& i : this->CSharpCustomCommandNames) {
          oss << "      " << i << ";\n";
        }
        oss << "      "
            << "$(BuildDependsOn)\n";
        e1.Element("BuildDependsOn", oss.str());
      }
    }
  }

  if (BuildFileStream.Close()) {
    this->GlobalGenerator->FileReplacedDuringGenerate(PathToProjectFile);
  }

  // The groups are stored in a separate file for VS 10
  this->WriteGroups();
}

void cmVisualStudio10TargetGenerator::WriteDotNetReferences(Elem& e0)
{
  std::vector<std::string> references;
  if (const char* vsDotNetReferences =
        this->GeneratorTarget->GetProperty("VS_DOTNET_REFERENCES")) {
    cmSystemTools::ExpandListArgument(vsDotNetReferences, references);
  }
  cmPropertyMap const& props = this->GeneratorTarget->Target->GetProperties();
  for (auto const& i : props) {
    if (i.first.find("VS_DOTNET_REFERENCE_") == 0) {
      std::string name = i.first.substr(20);
      if (!name.empty()) {
        std::string path = i.second.GetValue();
        if (!cmsys::SystemTools::FileIsFullPath(path)) {
          path = this->Makefile->GetCurrentSourceDirectory() + "/" + path;
        }
        ConvertToWindowsSlash(path);
        this->DotNetHintReferences[""].push_back(
          DotNetHintReference(name, path));
      }
    }
  }
  if (!references.empty() || !this->DotNetHintReferences.empty()) {
    Elem e1(e0, "ItemGroup");
    for (std::string const& ri : references) {
      // if the entry from VS_DOTNET_REFERENCES is an existing file, generate
      // a new hint-reference and name it from the filename
      if (cmsys::SystemTools::FileExists(ri, true)) {
        std::string name = cmsys::SystemTools::GetFilenameWithoutExtension(ri);
        std::string path = ri;
        ConvertToWindowsSlash(path);
        this->DotNetHintReferences[""].push_back(
          DotNetHintReference(name, path));
      } else {
        this->WriteDotNetReference(e1, ri, "", "");
      }
    }
    for (const auto& h : this->DotNetHintReferences) {
      // DotNetHintReferences is also populated from AddLibraries().
      // The configuration specific hint references are added there.
      for (const auto& i : h.second) {
        this->WriteDotNetReference(e1, i.first, i.second, h.first);
      }
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteDotNetReference(
  Elem& e1, std::string const& ref, std::string const& hint,
  std::string const& config)
{
  Elem e2(e1, "Reference");
  // If 'config' is not empty, the reference is only added for the given
  // configuration. This is used when referencing imported managed assemblies.
  // See also cmVisualStudio10TargetGenerator::AddLibraries().
  if (!config.empty()) {
    e2.Attribute("Condition", this->CalcCondition(config));
  }
  e2.Attribute("Include", ref);
  e2.Element("CopyLocalSatelliteAssemblies", "true");
  e2.Element("ReferenceOutputAssembly", "true");
  if (!hint.empty()) {
    const char* privateReference = "True";
    if (const char* value = this->GeneratorTarget->GetProperty(
          "VS_DOTNET_REFERENCES_COPY_LOCAL")) {
      if (cmSystemTools::IsOff(value)) {
        privateReference = "False";
      }
    }
    e2.Element("Private", privateReference);
    e2.Element("HintPath", hint);
  }
  this->WriteDotNetReferenceCustomTags(e2, ref);
}

void cmVisualStudio10TargetGenerator::WriteDotNetReferenceCustomTags(
  Elem& e2, std::string const& ref)
{

  static const std::string refpropPrefix = "VS_DOTNET_REFERENCEPROP_";
  static const std::string refpropInfix = "_TAG_";
  const std::string refPropFullPrefix = refpropPrefix + ref + refpropInfix;
  typedef std::map<std::string, std::string> CustomTags;
  CustomTags tags;
  cmPropertyMap const& props = this->GeneratorTarget->Target->GetProperties();
  for (const auto& i : props) {
    if (i.first.find(refPropFullPrefix) == 0) {
      std::string refTag = i.first.substr(refPropFullPrefix.length());
      std::string refVal = i.second.GetValue();
      if (!refTag.empty() && !refVal.empty()) {
        tags[refTag] = refVal;
      }
    }
  }
  for (auto const& tag : tags) {
    e2.Element(tag.first.c_str(), tag.second);
  }
}

void cmVisualStudio10TargetGenerator::WriteEmbeddedResourceGroup(Elem& e0)
{
  std::vector<cmSourceFile const*> resxObjs;
  this->GeneratorTarget->GetResxSources(resxObjs, "");
  if (!resxObjs.empty()) {
    Elem e1(e0, "ItemGroup");
    std::string srcDir = this->Makefile->GetCurrentSourceDirectory();
    ConvertToWindowsSlash(srcDir);
    for (cmSourceFile const* oi : resxObjs) {
      std::string obj = oi->GetFullPath();
      ConvertToWindowsSlash(obj);
      bool useRelativePath = false;
      if (this->ProjectType == csproj && this->InSourceBuild) {
        // If we do an in-source build and the resource file is in a
        // subdirectory
        // of the .csproj file, we have to use relative pathnames, otherwise
        // visual studio does not show the file in the IDE. Sorry.
        if (obj.find(srcDir) == 0) {
          obj = this->ConvertPath(obj, true);
          ConvertToWindowsSlash(obj);
          useRelativePath = true;
        }
      }
      Elem e2(e1, "EmbeddedResource");
      e2.Attribute("Include", obj);

      if (this->ProjectType != csproj) {
        std::string hFileName = obj.substr(0, obj.find_last_of(".")) + ".h";
        e2.Element("DependentUpon", hFileName);

        for (std::string const& c : this->Configurations) {
          std::string s;
          if (this->GeneratorTarget->GetProperty("VS_GLOBAL_ROOTNAMESPACE") ||
              // Handle variant of VS_GLOBAL_<variable> for RootNamespace.
              this->GeneratorTarget->GetProperty("VS_GLOBAL_RootNamespace")) {
            s = "$(RootNamespace).";
          }
          s += "%(Filename).resources";
          e2.WritePlatformConfigTag("LogicalName", this->CalcCondition(c), s);
        }
      } else {
        std::string binDir = this->Makefile->GetCurrentBinaryDirectory();
        ConvertToWindowsSlash(binDir);
        // If the resource was NOT added using a relative path (which should
        // be the default), we have to provide a link here
        if (!useRelativePath) {
          std::string link;
          if (obj.find(srcDir) == 0) {
            link = obj.substr(srcDir.length() + 1);
          } else if (obj.find(binDir) == 0) {
            link = obj.substr(binDir.length() + 1);
          } else {
            link = cmsys::SystemTools::GetFilenameName(obj);
          }
          if (!link.empty()) {
            e2.Element("Link", link);
          }
        }
        // Determine if this is a generated resource from a .Designer.cs file
        std::string designerResource =
          cmSystemTools::GetFilenamePath(oi->GetFullPath()) + "/" +
          cmSystemTools::GetFilenameWithoutLastExtension(oi->GetFullPath()) +
          ".Designer.cs";
        if (cmsys::SystemTools::FileExists(designerResource)) {
          std::string generator = "PublicResXFileCodeGenerator";
          if (const char* g = oi->GetProperty("VS_RESOURCE_GENERATOR")) {
            generator = g;
          }
          if (!generator.empty()) {
            e2.Element("Generator", generator);
            if (designerResource.find(srcDir) == 0) {
              designerResource = designerResource.substr(srcDir.length() + 1);
            } else if (designerResource.find(binDir) == 0) {
              designerResource = designerResource.substr(binDir.length() + 1);
            } else {
              designerResource =
                cmsys::SystemTools::GetFilenameName(designerResource);
            }
            ConvertToWindowsSlash(designerResource);
            e2.Element("LastGenOutput", designerResource);
          }
        }
        const cmPropertyMap& props = oi->GetProperties();
        for (const auto& p : props) {
          static const std::string propNamePrefix = "VS_CSHARP_";
          if (p.first.find(propNamePrefix) == 0) {
            std::string tagName = p.first.substr(propNamePrefix.length());
            if (!tagName.empty()) {
              std::string value = props.GetPropertyValue(p.first);
              if (!value.empty()) {
                e2.Element(tagName.c_str(), value);
              }
            }
          }
        }
      }
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteXamlFilesGroup(Elem& e0)
{
  std::vector<cmSourceFile const*> xamlObjs;
  this->GeneratorTarget->GetXamlSources(xamlObjs, "");
  if (!xamlObjs.empty()) {
    Elem e1(e0, "ItemGroup");
    for (cmSourceFile const* oi : xamlObjs) {
      std::string obj = oi->GetFullPath();
      const char* xamlType;
      const char* xamlTypeProperty = oi->GetProperty("VS_XAML_TYPE");
      if (xamlTypeProperty) {
        xamlType = xamlTypeProperty;
      } else {
        xamlType = "Page";
      }

      Elem e2(e1);
      this->WriteSource(e2, xamlType, oi);
      e2.SetHasElements();
      if (this->ProjectType == csproj && !this->InSourceBuild) {
        // add <Link> tag to written XAML source if necessary
        const std::string& srcDir =
          this->Makefile->GetCurrentSourceDirectory();
        const std::string& binDir =
          this->Makefile->GetCurrentBinaryDirectory();
        std::string link;
        if (obj.find(srcDir) == 0) {
          link = obj.substr(srcDir.length() + 1);
        } else if (obj.find(binDir) == 0) {
          link = obj.substr(binDir.length() + 1);
        } else {
          link = cmsys::SystemTools::GetFilenameName(obj);
        }
        if (!link.empty()) {
          ConvertToWindowsSlash(link);
          e2.Element("Link", link);
        }
      }
      e2.Element("SubType", "Designer");
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteTargetSpecificReferences(Elem& e0)
{
  if (this->MSTools) {
    if (this->GlobalGenerator->TargetsWindowsPhone() &&
        this->GlobalGenerator->GetSystemVersion() == "8.0") {
      Elem(e0, "Import")
        .Attribute("Project",
                   "$(MSBuildExtensionsPath)\\Microsoft\\WindowsPhone\\v"
                   "$(TargetPlatformVersion)\\Microsoft.Cpp.WindowsPhone."
                   "$(TargetPlatformVersion).targets");
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteTargetsFileReferences(Elem& e1)
{
  for (TargetsFileAndConfigs const& tac : this->TargetsFileAndConfigsVec) {
    std::ostringstream oss;
    oss << "Exists('" << tac.File << "')";
    if (!tac.Configs.empty()) {
      oss << " And (";
      for (size_t j = 0; j < tac.Configs.size(); ++j) {
        if (j > 0) {
          oss << " Or ";
        }
        oss << "'$(Configuration)'=='" << tac.Configs[j] << "'";
      }
      oss << ")";
    }

    Elem(e1, "Import")
      .Attribute("Project", tac.File)
      .Attribute("Condition", oss.str());
  }
}

void cmVisualStudio10TargetGenerator::WriteWinRTReferences(Elem& e0)
{
  std::vector<std::string> references;
  if (const char* vsWinRTReferences =
        this->GeneratorTarget->GetProperty("VS_WINRT_REFERENCES")) {
    cmSystemTools::ExpandListArgument(vsWinRTReferences, references);
  }

  if (this->GlobalGenerator->TargetsWindowsPhone() &&
      this->GlobalGenerator->GetSystemVersion() == "8.0" &&
      references.empty()) {
    references.push_back("platform.winmd");
  }
  if (!references.empty()) {
    Elem e1(e0, "ItemGroup");
    for (std::string const& ri : references) {
      Elem e2(e1, "Reference");
      e2.Attribute("Include", ri);
      e2.Element("IsWinMDFile", "true");
    }
  }
}

// ConfigurationType Application, Utility StaticLibrary DynamicLibrary

void cmVisualStudio10TargetGenerator::WriteProjectConfigurations(Elem& e0)
{
  Elem e1(e0, "ItemGroup");
  e1.Attribute("Label", "ProjectConfigurations");
  for (std::string const& c : this->Configurations) {
    Elem e2(e1, "ProjectConfiguration");
    e2.Attribute("Include", c + "|" + this->Platform);
    e2.Element("Configuration", c);
    e2.Element("Platform", this->Platform);
  }
}

void cmVisualStudio10TargetGenerator::WriteProjectConfigurationValues(Elem& e0)
{
  for (std::string const& c : this->Configurations) {
    Elem e1(e0, "PropertyGroup");
    e1.Attribute("Condition", this->CalcCondition(c));
    e1.Attribute("Label", "Configuration");

    if (this->ProjectType != csproj) {
      std::string configType;
      if (const char* vsConfigurationType =
            this->GeneratorTarget->GetProperty("VS_CONFIGURATION_TYPE")) {
        configType = vsConfigurationType;
      } else {
        switch (this->GeneratorTarget->GetType()) {
          case cmStateEnums::SHARED_LIBRARY:
          case cmStateEnums::MODULE_LIBRARY:
            configType = "DynamicLibrary";
            break;
          case cmStateEnums::OBJECT_LIBRARY:
          case cmStateEnums::STATIC_LIBRARY:
            configType = "StaticLibrary";
            break;
          case cmStateEnums::EXECUTABLE:
            if (this->NsightTegra &&
                !this->GeneratorTarget->GetPropertyAsBool("ANDROID_GUI")) {
              // Android executables are .so too.
              configType = "DynamicLibrary";
            } else {
              configType = "Application";
            }
            break;
          case cmStateEnums::UTILITY:
          case cmStateEnums::GLOBAL_TARGET:
            if (this->NsightTegra) {
              // Tegra-Android platform does not understand "Utility".
              configType = "StaticLibrary";
            } else {
              configType = "Utility";
            }
            break;
          case cmStateEnums::UNKNOWN_LIBRARY:
          case cmStateEnums::INTERFACE_LIBRARY:
            break;
        }
      }
      e1.Element("ConfigurationType", configType);
    }

    if (this->MSTools) {
      if (!this->Managed) {
        this->WriteMSToolConfigurationValues(e1, c);
      } else {
        this->WriteMSToolConfigurationValuesManaged(e1, c);
      }
    } else if (this->NsightTegra) {
      this->WriteNsightTegraConfigurationValues(e1, c);
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteMSToolConfigurationValues(
  Elem& e1, std::string const& config)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  const char* mfcFlag = this->Makefile->GetDefinition("CMAKE_MFC_FLAG");
  if (mfcFlag) {
    std::string const mfcFlagValue = mfcFlag;

    std::string useOfMfcValue = "false";
    if (this->GeneratorTarget->GetType() <= cmStateEnums::OBJECT_LIBRARY) {
      if (mfcFlagValue == "1") {
        useOfMfcValue = "Static";
      } else if (mfcFlagValue == "2") {
        useOfMfcValue = "Dynamic";
      }
    }
    e1.Element("UseOfMfc", useOfMfcValue);
  }

  if ((this->GeneratorTarget->GetType() <= cmStateEnums::OBJECT_LIBRARY &&
       this->ClOptions[config]->UsingUnicode()) ||
      this->GeneratorTarget->GetPropertyAsBool("VS_WINRT_COMPONENT") ||
      this->GlobalGenerator->TargetsWindowsPhone() ||
      this->GlobalGenerator->TargetsWindowsStore() ||
      this->GeneratorTarget->GetPropertyAsBool("VS_WINRT_EXTENSIONS")) {
    e1.Element("CharacterSet", "Unicode");
  } else if (this->GeneratorTarget->GetType() <=
               cmStateEnums::MODULE_LIBRARY &&
             this->ClOptions[config]->UsingSBCS()) {
    e1.Element("CharacterSet", "NotSet");
  } else {
    e1.Element("CharacterSet", "MultiByte");
  }
  if (const char* toolset = gg->GetPlatformToolset()) {
    e1.Element("PlatformToolset", toolset);
  }
  if (this->GeneratorTarget->GetPropertyAsBool("VS_WINRT_COMPONENT") ||
      this->GeneratorTarget->GetPropertyAsBool("VS_WINRT_EXTENSIONS")) {
    e1.Element("WindowsAppContainer", "true");
  }
  if (this->IPOEnabledConfigurations.count(config) > 0) {
    e1.Element("WholeProgramOptimization", "true");
  }
  if (this->SpectreMitigationConfigurations.count(config) > 0) {
    e1.Element("SpectreMitigation", "Spectre");
  }
}

void cmVisualStudio10TargetGenerator::WriteMSToolConfigurationValuesManaged(
  Elem& e1, std::string const& config)
{
  if (this->GeneratorTarget->GetType() > cmStateEnums::OBJECT_LIBRARY) {
    return;
  }

  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;

  Options& o = *(this->ClOptions[config]);

  if (o.IsDebug()) {
    e1.Element("DebugSymbols", "true");
    e1.Element("DefineDebug", "true");
  }

  std::string outDir = this->GeneratorTarget->GetDirectory(config) + "/";
  ConvertToWindowsSlash(outDir);
  e1.Element("OutputPath", outDir);

  if (o.HasFlag("Platform")) {
    e1.Element("PlatformTarget", o.GetFlag("Platform"));
    o.RemoveFlag("Platform");
  }

  if (const char* toolset = gg->GetPlatformToolset()) {
    e1.Element("PlatformToolset", toolset);
  }

  std::string postfixName = cmSystemTools::UpperCase(config);
  postfixName += "_POSTFIX";
  std::string assemblyName = this->GeneratorTarget->GetOutputName(
    config, cmStateEnums::RuntimeBinaryArtifact);
  if (const char* postfix = this->GeneratorTarget->GetProperty(postfixName)) {
    assemblyName += postfix;
  }
  e1.Element("AssemblyName", assemblyName);

  if (cmStateEnums::EXECUTABLE == this->GeneratorTarget->GetType()) {
    e1.Element("StartAction", "Program");
    e1.Element("StartProgram", outDir + assemblyName + ".exe");
  }

  OptionsHelper oh(o, e1);
  oh.OutputFlagMap();
}

//----------------------------------------------------------------------------
void cmVisualStudio10TargetGenerator::WriteNsightTegraConfigurationValues(
  Elem& e1, std::string const&)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  const char* toolset = gg->GetPlatformToolset();
  e1.Element("NdkToolchainVersion", toolset ? toolset : "Default");
  if (const char* minApi =
        this->GeneratorTarget->GetProperty("ANDROID_API_MIN")) {
    e1.Element("AndroidMinAPI", "android-" + std::string(minApi));
  }
  if (const char* api = this->GeneratorTarget->GetProperty("ANDROID_API")) {
    e1.Element("AndroidTargetAPI", "android-" + std::string(api));
  }

  if (const char* cpuArch =
        this->GeneratorTarget->GetProperty("ANDROID_ARCH")) {
    e1.Element("AndroidArch", cpuArch);
  }

  if (const char* stlType =
        this->GeneratorTarget->GetProperty("ANDROID_STL_TYPE")) {
    e1.Element("AndroidStlType", stlType);
  }
}

void cmVisualStudio10TargetGenerator::WriteCustomCommands(Elem& e0)
{
  this->CSharpCustomCommandNames.clear();
  std::vector<cmSourceFile const*> customCommands;
  this->GeneratorTarget->GetCustomCommands(customCommands, "");
  for (cmSourceFile const* si : customCommands) {
    this->WriteCustomCommand(e0, si);
  }

  // Add CMakeLists.txt file with rule to re-run CMake for user convenience.
  if (this->GeneratorTarget->GetType() != cmStateEnums::GLOBAL_TARGET &&
      this->GeneratorTarget->GetName() != CMAKE_CHECK_BUILD_SYSTEM_TARGET) {
    if (cmSourceFile const* sf =
          this->LocalGenerator->CreateVCProjBuildRule()) {
      // Write directly rather than through WriteCustomCommand because
      // we do not want the de-duplication and it has no dependencies.
      if (cmCustomCommand const* command = sf->GetCustomCommand()) {
        this->WriteCustomRule(e0, sf, *command);
      }
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteCustomCommand(
  Elem& e0, cmSourceFile const* sf)
{
  if (this->LocalGenerator->GetSourcesVisited(this->GeneratorTarget)
        .insert(sf)
        .second) {
    if (std::vector<cmSourceFile*> const* depends =
          this->GeneratorTarget->GetSourceDepends(sf)) {
      for (cmSourceFile const* di : *depends) {
        this->WriteCustomCommand(e0, di);
      }
    }
    if (cmCustomCommand const* command = sf->GetCustomCommand()) {
      // C# projects write their <Target> within WriteCustomRule()
      this->WriteCustomRule(e0, sf, *command);
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteCustomRule(
  Elem& e0, cmSourceFile const* source, cmCustomCommand const& command)
{
  std::string sourcePath = source->GetFullPath();
  // VS 10 will always rebuild a custom command attached to a .rule
  // file that doesn't exist so create the file explicitly.
  if (source->GetPropertyAsBool("__CMAKE_RULE")) {
    if (!cmSystemTools::FileExists(sourcePath)) {
      // Make sure the path exists for the file
      std::string path = cmSystemTools::GetFilenamePath(sourcePath);
      cmSystemTools::MakeDirectory(path);
      cmsys::ofstream fout(sourcePath.c_str());
      if (fout) {
        fout << "# generated from CMake\n";
        fout.flush();
        fout.close();
        // Force given file to have a very old timestamp, thus
        // preventing dependent rebuilds.
        this->ForceOld(sourcePath);
      } else {
        std::string error = "Could not create file: [";
        error += sourcePath;
        error += "]  ";
        cmSystemTools::Error(error.c_str(),
                             cmSystemTools::GetLastSystemError().c_str());
      }
    }
  }
  cmLocalVisualStudio7Generator* lg = this->LocalGenerator;

  std::unique_ptr<Elem> spe1;
  std::unique_ptr<Elem> spe2;
  if (this->ProjectType != csproj) {
    spe1 = cm::make_unique<Elem>(e0, "ItemGroup");
    spe2 = cm::make_unique<Elem>(*spe1);
    this->WriteSource(*spe2, "CustomBuild", source);
    spe2->SetHasElements();
  } else {
    Elem e1(e0, "ItemGroup");
    Elem e2(e1);
    std::string link;
    this->GetCSharpSourceLink(source, link);
    this->WriteSource(e2, "None", source);
    e2.SetHasElements();
    if (!link.empty()) {
      e2.Element("Link", link);
    }
  }
  for (std::string const& c : this->Configurations) {
    cmCustomCommandGenerator ccg(command, c, lg);
    std::string comment = lg->ConstructComment(ccg);
    comment = cmVS10EscapeComment(comment);
    std::string script = lg->ConstructScript(ccg);
    // input files for custom command
    std::stringstream inputs;
    inputs << source->GetFullPath();
    for (std::string const& d : ccg.GetDepends()) {
      std::string dep;
      if (lg->GetRealDependency(d, c, dep)) {
        ConvertToWindowsSlash(dep);
        inputs << ";" << dep;
      }
    }
    // output files for custom command
    std::stringstream outputs;
    const char* sep = "";
    for (std::string const& o : ccg.GetOutputs()) {
      std::string out = o;
      ConvertToWindowsSlash(out);
      outputs << sep << out;
      sep = ";";
    }
    if (this->ProjectType == csproj) {
      std::string name = "CustomCommand_" + c + "_" +
        cmSystemTools::ComputeStringMD5(sourcePath);
      this->WriteCustomRuleCSharp(e0, c, name, script, inputs.str(),
                                  outputs.str(), comment);
    } else {
      this->WriteCustomRuleCpp(*spe2, c, script, inputs.str(), outputs.str(),
                               comment);
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteCustomRuleCpp(
  Elem& e2, std::string const& config, std::string const& script,
  std::string const& inputs, std::string const& outputs,
  std::string const& comment)
{
  const std::string cond = this->CalcCondition(config);
  e2.WritePlatformConfigTag("Message", cond, comment);
  e2.WritePlatformConfigTag("Command", cond, script);
  e2.WritePlatformConfigTag("AdditionalInputs", cond,
                            inputs + ";%(AdditionalInputs)");
  e2.WritePlatformConfigTag("Outputs", cond, outputs);
  if (this->LocalGenerator->GetVersion() >
      cmGlobalVisualStudioGenerator::VS10) {
    // VS >= 11 let us turn off linking of custom command outputs.
    e2.WritePlatformConfigTag("LinkObjects", cond, "false");
  }
}

void cmVisualStudio10TargetGenerator::WriteCustomRuleCSharp(
  Elem& e0, std::string const& config, std::string const& name,
  std::string const& script, std::string const& inputs,
  std::string const& outputs, std::string const& comment)
{
  this->CSharpCustomCommandNames.insert(name);
  Elem e1(e0, "Target");
  e1.Attribute("Condition", this->CalcCondition(config));
  e1.S << "\n    Name=\"" << name << "\"";
  e1.S << "\n    Inputs=\"" << cmVS10EscapeAttr(inputs) << "\"";
  e1.S << "\n    Outputs=\"" << cmVS10EscapeAttr(outputs) << "\"";
  if (!comment.empty()) {
    Elem(e1, "Exec").Attribute("Command", "echo " + comment);
  }
  Elem(e1, "Exec").Attribute("Command", script);
}

std::string cmVisualStudio10TargetGenerator::ConvertPath(
  std::string const& path, bool forceRelative)
{
  return forceRelative
    ? cmSystemTools::RelativePath(
        this->LocalGenerator->GetCurrentBinaryDirectory(), path)
    : path;
}

static void ConvertToWindowsSlash(std::string& s)
{
  // first convert all of the slashes
  std::string::size_type pos = 0;
  while ((pos = s.find('/', pos)) != std::string::npos) {
    s[pos] = '\\';
    pos++;
  }
}

void cmVisualStudio10TargetGenerator::WriteGroups()
{
  if (this->ProjectType == csproj) {
    return;
  }

  // collect up group information
  std::vector<cmSourceGroup> sourceGroups = this->Makefile->GetSourceGroups();

  std::vector<cmGeneratorTarget::AllConfigSource> const& sources =
    this->GeneratorTarget->GetAllConfigSources();

  std::set<cmSourceGroup*> groupsUsed;
  for (cmGeneratorTarget::AllConfigSource const& si : sources) {
    std::string const& source = si.Source->GetFullPath();
    cmSourceGroup* sourceGroup =
      this->Makefile->FindSourceGroup(source, sourceGroups);
    groupsUsed.insert(sourceGroup);
  }

  this->AddMissingSourceGroups(groupsUsed, sourceGroups);

  // Write out group file
  std::string path = this->LocalGenerator->GetCurrentBinaryDirectory();
  path += "/";
  path += this->Name;
  path += computeProjectFileExtension(this->GeneratorTarget);
  path += ".filters";
  cmGeneratedFileStream fout(path);
  fout.SetCopyIfDifferent(true);
  char magic[] = { char(0xEF), char(0xBB), char(0xBF) };
  fout.write(magic, 3);

  fout << "<?xml version=\"1.0\" encoding=\""
       << this->GlobalGenerator->Encoding() << "\"?>"
       << "\n";
  {
    Elem e0(fout);
    e0.StartElement("Project");
    e0.Attribute("ToolsVersion", this->GlobalGenerator->GetToolsVersion());
    e0.Attribute("xmlns",
                 "http://schemas.microsoft.com/developer/msbuild/2003");

    for (auto const& ti : this->Tools) {
      this->WriteGroupSources(e0, ti.first, ti.second, sourceGroups);
    }

    // Added files are images and the manifest.
    if (!this->AddedFiles.empty()) {
      Elem e1(e0, "ItemGroup");
      e1.SetHasElements();
      for (std::string const& oi : this->AddedFiles) {
        std::string fileName =
          cmSystemTools::LowerCase(cmSystemTools::GetFilenameName(oi));
        if (fileName == "wmappmanifest.xml") {
          Elem e2(e1, "XML");
          e2.Attribute("Include", oi);
          e2.Element("Filter", "Resource Files");
        } else if (cmSystemTools::GetFilenameExtension(fileName) ==
                   ".appxmanifest") {
          Elem e2(e1, "AppxManifest");
          e2.Attribute("Include", oi);
          e2.Element("Filter", "Resource Files");
        } else if (cmSystemTools::GetFilenameExtension(fileName) == ".pfx") {
          Elem e2(e1, "None");
          e2.Attribute("Include", oi);
          e2.Element("Filter", "Resource Files");
        } else {
          Elem e2(e1, "Image");
          e2.Attribute("Include", oi);
          e2.Element("Filter", "Resource Files");
        }
      }
    }

    std::vector<cmSourceFile const*> resxObjs;
    this->GeneratorTarget->GetResxSources(resxObjs, "");
    if (!resxObjs.empty()) {
      Elem e1(e0, "ItemGroup");
      for (cmSourceFile const* oi : resxObjs) {
        std::string obj = oi->GetFullPath();
        ConvertToWindowsSlash(obj);
        Elem e2(e1, "EmbeddedResource");
        e2.Attribute("Include", obj);
        e2.Element("Filter", "Resource Files");
      }
    }
    {
      Elem e1(e0, "ItemGroup");
      e1.SetHasElements();
      std::vector<cmSourceGroup*> groupsVec(groupsUsed.begin(),
                                            groupsUsed.end());
      std::sort(groupsVec.begin(), groupsVec.end(),
                [](cmSourceGroup* l, cmSourceGroup* r) {
                  return l->GetFullName() < r->GetFullName();
                });
      for (cmSourceGroup* sg : groupsVec) {
        std::string const& name = sg->GetFullName();
        if (!name.empty()) {
          std::string guidName = "SG_Filter_" + name;
          std::string guid = this->GlobalGenerator->GetGUID(guidName);
          Elem e2(e1, "Filter");
          e2.Attribute("Include", name);
          e2.Element("UniqueIdentifier", "{" + guid + "}");
        }
      }

      if (!resxObjs.empty() || !this->AddedFiles.empty()) {
        std::string guidName = "SG_Filter_Resource Files";
        std::string guid = this->GlobalGenerator->GetGUID(guidName);
        Elem e2(e1, "Filter");
        e2.Attribute("Include", "Resource Files");
        e2.Element("UniqueIdentifier", "{" + guid + "}");
        e2.Element("Extensions",
                   "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;"
                   "gif;jpg;jpeg;jpe;resx;tiff;tif;png;wav;mfcribbon-ms");
      }
    }
  }
  fout << '\n';

  if (fout.Close()) {
    this->GlobalGenerator->FileReplacedDuringGenerate(path);
  }
}

// Add to groupsUsed empty source groups that have non-empty children.
void cmVisualStudio10TargetGenerator::AddMissingSourceGroups(
  std::set<cmSourceGroup*>& groupsUsed,
  const std::vector<cmSourceGroup>& allGroups)
{
  for (cmSourceGroup const& current : allGroups) {
    std::vector<cmSourceGroup> const& children = current.GetGroupChildren();
    if (children.empty()) {
      continue; // the group is really empty
    }

    this->AddMissingSourceGroups(groupsUsed, children);

    cmSourceGroup* current_ptr = const_cast<cmSourceGroup*>(&current);
    if (groupsUsed.find(current_ptr) != groupsUsed.end()) {
      continue; // group has already been added to set
    }

    // check if it least one of the group's descendants is not empty
    // (at least one child must already have been added)
    std::vector<cmSourceGroup>::const_iterator child_it = children.begin();
    while (child_it != children.end()) {
      cmSourceGroup* child_ptr = const_cast<cmSourceGroup*>(&(*child_it));
      if (groupsUsed.find(child_ptr) != groupsUsed.end()) {
        break; // found a child that was already added => add current group too
      }
      child_it++;
    }

    if (child_it == children.end()) {
      continue; // no descendants have source files => ignore this group
    }

    groupsUsed.insert(current_ptr);
  }
}

void cmVisualStudio10TargetGenerator::WriteGroupSources(
  Elem& e0, std::string const& name, ToolSources const& sources,
  std::vector<cmSourceGroup>& sourceGroups)
{
  Elem e1(e0, "ItemGroup");
  e1.SetHasElements();
  for (ToolSource const& s : sources) {
    cmSourceFile const* sf = s.SourceFile;
    std::string const& source = sf->GetFullPath();
    cmSourceGroup* sourceGroup =
      this->Makefile->FindSourceGroup(source, sourceGroups);
    std::string const& filter = sourceGroup->GetFullName();
    std::string path = this->ConvertPath(source, s.RelativePath);
    ConvertToWindowsSlash(path);
    Elem e2(e1, name.c_str());
    e2.Attribute("Include", path);
    if (!filter.empty()) {
      e2.Element("Filter", filter);
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteHeaderSource(Elem& e1,
                                                        cmSourceFile const* sf)
{
  std::string const& fileName = sf->GetFullPath();
  Elem e2(e1);
  this->WriteSource(e2, "ClInclude", sf);
  if (this->IsResxHeader(fileName)) {
    e2.Element("FileType", "CppForm");
  } else if (this->IsXamlHeader(fileName)) {
    std::string xamlFileName = fileName.substr(0, fileName.find_last_of("."));
    e2.Element("DependentUpon", xamlFileName);
  }
}

void cmVisualStudio10TargetGenerator::WriteExtraSource(Elem& e1,
                                                       cmSourceFile const* sf)
{
  bool toolHasSettings = false;
  const char* tool = "None";
  std::string shaderType;
  std::string shaderEntryPoint;
  std::string shaderModel;
  std::string shaderAdditionalFlags;
  std::string shaderDisableOptimizations;
  std::string shaderEnableDebug;
  std::string shaderObjectFileName;
  std::string outputHeaderFile;
  std::string variableName;
  std::string settingsGenerator;
  std::string settingsLastGenOutput;
  std::string sourceLink;
  std::string subType;
  std::string copyToOutDir;
  std::string includeInVsix;
  std::string ext = cmSystemTools::LowerCase(sf->GetExtension());
  if (this->ProjectType == csproj) {
    // EVERY extra source file must have a <Link>, otherwise it might not
    // be visible in Visual Studio at all. The path relative to current
    // source- or binary-dir is used within the link, if the file is
    // in none of these paths, it is added with the plain filename without
    // any path. This means the file will show up at root-level of the csproj
    // (where CMakeLists.txt etc. are).
    if (!this->InSourceBuild) {
      toolHasSettings = true;
      std::string fullFileName = sf->GetFullPath();
      std::string srcDir = this->Makefile->GetCurrentSourceDirectory();
      std::string binDir = this->Makefile->GetCurrentBinaryDirectory();
      if (fullFileName.find(binDir) != std::string::npos) {
        sourceLink.clear();
      } else if (fullFileName.find(srcDir) != std::string::npos) {
        sourceLink = fullFileName.substr(srcDir.length() + 1);
      } else {
        // fallback: add plain filename without any path
        sourceLink = cmsys::SystemTools::GetFilenameName(fullFileName);
      }
      if (!sourceLink.empty()) {
        ConvertToWindowsSlash(sourceLink);
      }
    }
  }
  if (ext == "hlsl") {
    tool = "FXCompile";
    // Figure out the type of shader compiler to use.
    if (const char* st = sf->GetProperty("VS_SHADER_TYPE")) {
      shaderType = st;
      toolHasSettings = true;
    }
    // Figure out which entry point to use if any
    if (const char* se = sf->GetProperty("VS_SHADER_ENTRYPOINT")) {
      shaderEntryPoint = se;
      toolHasSettings = true;
    }
    // Figure out which shader model to use if any
    if (const char* sm = sf->GetProperty("VS_SHADER_MODEL")) {
      shaderModel = sm;
      toolHasSettings = true;
    }
    // Figure out which output header file to use if any
    if (const char* ohf = sf->GetProperty("VS_SHADER_OUTPUT_HEADER_FILE")) {
      outputHeaderFile = ohf;
      toolHasSettings = true;
    }
    // Figure out which variable name to use if any
    if (const char* vn = sf->GetProperty("VS_SHADER_VARIABLE_NAME")) {
      variableName = vn;
      toolHasSettings = true;
    }
    // Figure out if there's any additional flags to use
    if (const char* saf = sf->GetProperty("VS_SHADER_FLAGS")) {
      shaderAdditionalFlags = saf;
      toolHasSettings = true;
    }
    // Figure out if debug information should be generated
    if (const char* sed = sf->GetProperty("VS_SHADER_ENABLE_DEBUG")) {
      shaderEnableDebug = sed;
      toolHasSettings = true;
    }
    // Figure out if optimizations should be disabled
    if (const char* sdo = sf->GetProperty("VS_SHADER_DISABLE_OPTIMIZATIONS")) {
      shaderDisableOptimizations = sdo;
      toolHasSettings = true;
    }
    if (const char* sofn = sf->GetProperty("VS_SHADER_OBJECT_FILE_NAME")) {
      shaderObjectFileName = sofn;
      toolHasSettings = true;
    }
  } else if (ext == "jpg" || ext == "png") {
    tool = "Image";
  } else if (ext == "resw") {
    tool = "PRIResource";
  } else if (ext == "xml") {
    tool = "XML";
  } else if (ext == "natvis") {
    tool = "Natvis";
  } else if (ext == "settings") {
    settingsLastGenOutput =
      cmsys::SystemTools::GetFilenameName(sf->GetFullPath());
    std::size_t pos = settingsLastGenOutput.find(".settings");
    settingsLastGenOutput.replace(pos, 9, ".Designer.cs");
    settingsGenerator = "SettingsSingleFileGenerator";
    toolHasSettings = true;
  } else if (ext == "vsixmanifest") {
    subType = "Designer";
  }
  if (const char* c = sf->GetProperty("VS_COPY_TO_OUT_DIR")) {
    copyToOutDir = c;
    toolHasSettings = true;
  }
  if (sf->GetPropertyAsBool("VS_INCLUDE_IN_VSIX")) {
    includeInVsix = "True";
    tool = "Content";
    toolHasSettings = true;
  }

  // Collect VS_CSHARP_* property values (if some are set)
  std::map<std::string, std::string> sourceFileTags;
  this->GetCSharpSourceProperties(sf, sourceFileTags);

  if (this->NsightTegra) {
    // Nsight Tegra needs specific file types to check up-to-dateness.
    std::string name = cmSystemTools::LowerCase(sf->GetLocation().GetName());
    if (name == "androidmanifest.xml" || name == "build.xml" ||
        name == "proguard.cfg" || name == "proguard-project.txt" ||
        ext == "properties") {
      tool = "AndroidBuild";
    } else if (ext == "java") {
      tool = "JCompile";
    } else if (ext == "asm" || ext == "s") {
      tool = "ClCompile";
    }
  }

  const char* toolOverride = sf->GetProperty("VS_TOOL_OVERRIDE");
  if (toolOverride && *toolOverride) {
    tool = toolOverride;
  }

  std::string deployContent;
  std::string deployLocation;
  if (this->GlobalGenerator->TargetsWindowsPhone() ||
      this->GlobalGenerator->TargetsWindowsStore()) {
    const char* content = sf->GetProperty("VS_DEPLOYMENT_CONTENT");
    if (content && *content) {
      toolHasSettings = true;
      deployContent = content;

      const char* location = sf->GetProperty("VS_DEPLOYMENT_LOCATION");
      if (location && *location) {
        deployLocation = location;
      }
    }
  }

  Elem e2(e1);
  this->WriteSource(e2, tool, sf);
  if (toolHasSettings) {
    e2.SetHasElements();

    if (!deployContent.empty()) {
      cmGeneratorExpression ge;
      std::unique_ptr<cmCompiledGeneratorExpression> cge =
        ge.Parse(deployContent);
      // Deployment location cannot be set on a configuration basis
      if (!deployLocation.empty()) {
        e2.Element("Link", deployLocation + "\\%(FileName)%(Extension)");
      }
      for (size_t i = 0; i != this->Configurations.size(); ++i) {
        if (cge->Evaluate(this->LocalGenerator, this->Configurations[i]) ==
            "1") {
          e2.WritePlatformConfigTag("DeploymentContent",
                                    "'$(Configuration)|$(Platform)'=='" +
                                      this->Configurations[i] + "|" +
                                      this->Platform + "'",
                                    "true");
        } else {
          e2.WritePlatformConfigTag("ExcludedFromBuild",
                                    "'$(Configuration)|$(Platform)'=='" +
                                      this->Configurations[i] + "|" +
                                      this->Platform + "'",
                                    "true");
        }
      }
    }
    if (!shaderType.empty()) {
      e2.Element("ShaderType", shaderType);
    }
    if (!shaderEntryPoint.empty()) {
      e2.Element("EntryPointName", shaderEntryPoint);
    }
    if (!shaderModel.empty()) {
      e2.Element("ShaderModel", shaderModel);
    }
    if (!outputHeaderFile.empty()) {
      for (size_t i = 0; i != this->Configurations.size(); ++i) {
        e2.WritePlatformConfigTag("HeaderFileOutput",
                                  "'$(Configuration)|$(Platform)'=='" +
                                    this->Configurations[i] + "|" +
                                    this->Platform + "'",
                                  outputHeaderFile);
      }
    }
    if (!variableName.empty()) {
      for (size_t i = 0; i != this->Configurations.size(); ++i) {
        e2.WritePlatformConfigTag("VariableName",
                                  "'$(Configuration)|$(Platform)'=='" +
                                    this->Configurations[i] + "|" +
                                    this->Platform + "'",
                                  variableName);
      }
    }
    if (!shaderEnableDebug.empty()) {
      cmGeneratorExpression ge;
      std::unique_ptr<cmCompiledGeneratorExpression> cge =
        ge.Parse(shaderEnableDebug);

      for (size_t i = 0; i != this->Configurations.size(); ++i) {
        const std::string& enableDebug =
          cge->Evaluate(this->LocalGenerator, this->Configurations[i]);
        if (!enableDebug.empty()) {
          e2.WritePlatformConfigTag(
            "EnableDebuggingInformation",
            "'$(Configuration)|$(Platform)'=='" + this->Configurations[i] +
              "|" + this->Platform + "'",
            cmSystemTools::IsOn(enableDebug) ? "true" : "false");
        }
      }
    }
    if (!shaderDisableOptimizations.empty()) {
      cmGeneratorExpression ge;
      std::unique_ptr<cmCompiledGeneratorExpression> cge =
        ge.Parse(shaderDisableOptimizations);

      for (size_t i = 0; i != this->Configurations.size(); ++i) {
        const std::string& disableOptimizations =
          cge->Evaluate(this->LocalGenerator, this->Configurations[i]);
        if (!disableOptimizations.empty()) {
          e2.WritePlatformConfigTag(
            "DisableOptimizations",
            "'$(Configuration)|$(Platform)'=='" + this->Configurations[i] +
              "|" + this->Platform + "'",
            (cmSystemTools::IsOn(disableOptimizations) ? "true" : "false"));
        }
      }
    }
    if (!shaderObjectFileName.empty()) {
      e2.Element("ObjectFileOutput", shaderObjectFileName);
    }
    if (!shaderAdditionalFlags.empty()) {
      e2.Element("AdditionalOptions", shaderAdditionalFlags);
    }
    if (!settingsGenerator.empty()) {
      e2.Element("Generator", settingsGenerator);
    }
    if (!settingsLastGenOutput.empty()) {
      e2.Element("LastGenOutput", settingsLastGenOutput);
    }
    if (!sourceLink.empty()) {
      e2.Element("Link", sourceLink);
    }
    if (!subType.empty()) {
      e2.Element("SubType", subType);
    }
    if (!copyToOutDir.empty()) {
      e2.Element("CopyToOutputDirectory", copyToOutDir);
    }
    if (!includeInVsix.empty()) {
      e2.Element("IncludeInVSIX", includeInVsix);
    }
    // write source file specific tags
    this->WriteCSharpSourceProperties(e2, sourceFileTags);
  }
}

void cmVisualStudio10TargetGenerator::WriteSource(Elem& e2,
                                                  std::string const& tool,
                                                  cmSourceFile const* sf)
{
  // Visual Studio tools append relative paths to the current dir, as in:
  //
  //  c:\path\to\current\dir\..\..\..\relative\path\to\source.c
  //
  // and fail if this exceeds the maximum allowed path length.  Our path
  // conversion uses full paths when possible to allow deeper trees.
  // However, CUDA 8.0 msbuild rules fail on absolute paths so for CUDA
  // we must use relative paths.
  bool forceRelative = sf->GetLanguage() == "CUDA";
  std::string sourceFile = this->ConvertPath(sf->GetFullPath(), forceRelative);
  if (this->LocalGenerator->GetVersion() ==
        cmGlobalVisualStudioGenerator::VS10 &&
      cmSystemTools::FileIsFullPath(sourceFile)) {
    // Normal path conversion resulted in a full path.  VS 10 (but not 11)
    // refuses to show the property page in the IDE for a source file with a
    // full path (not starting in a '.' or '/' AFAICT).  CMake <= 2.8.4 used a
    // relative path but to allow deeper build trees CMake 2.8.[5678] used a
    // full path except for custom commands.  Custom commands do not work
    // without a relative path, but they do not seem to be involved in tools
    // with the above behavior.  For other sources we now use a relative path
    // when the combined path will not be too long so property pages appear.
    std::string sourceRel = this->ConvertPath(sf->GetFullPath(), true);
    size_t const maxLen = 250;
    if (sf->GetCustomCommand() ||
        ((this->LocalGenerator->GetCurrentBinaryDirectory().length() + 1 +
          sourceRel.length()) <= maxLen)) {
      forceRelative = true;
      sourceFile = sourceRel;
    } else {
      this->GlobalGenerator->PathTooLong(this->GeneratorTarget, sf, sourceRel);
    }
  }
  ConvertToWindowsSlash(sourceFile);
  e2.StartElement(tool);
  e2.Attribute("Include", sourceFile);

  ToolSource toolSource = { sf, forceRelative };
  this->Tools[tool].push_back(toolSource);
}

void cmVisualStudio10TargetGenerator::WriteAllSources(Elem& e0)
{
  if (this->GeneratorTarget->GetType() > cmStateEnums::UTILITY) {
    return;
  }
  Elem e1(e0, "ItemGroup");
  e1.SetHasElements();

  std::vector<size_t> all_configs;
  for (size_t ci = 0; ci < this->Configurations.size(); ++ci) {
    all_configs.push_back(ci);
  }

  std::vector<cmGeneratorTarget::AllConfigSource> const& sources =
    this->GeneratorTarget->GetAllConfigSources();

  cmSourceFile const* srcCMakeLists =
    this->LocalGenerator->CreateVCProjBuildRule();

  for (cmGeneratorTarget::AllConfigSource const& si : sources) {
    if (si.Source == srcCMakeLists) {
      // Skip explicit reference to CMakeLists.txt source.
      continue;
    }
    const char* tool = nullptr;
    switch (si.Kind) {
      case cmGeneratorTarget::SourceKindAppManifest:
        tool = "AppxManifest";
        break;
      case cmGeneratorTarget::SourceKindCertificate:
        tool = "None";
        break;
      case cmGeneratorTarget::SourceKindCustomCommand:
        // Handled elsewhere.
        break;
      case cmGeneratorTarget::SourceKindExternalObject:
        tool = "Object";
        if (this->LocalGenerator->GetVersion() <
            cmGlobalVisualStudioGenerator::VS11) {
          // For VS == 10 we cannot use LinkObjects to avoid linking custom
          // command outputs.  If an object file is generated in this target,
          // then vs10 will use it in the build, and we have to list it as
          // None instead of Object.
          std::vector<cmSourceFile*> const* d =
            this->GeneratorTarget->GetSourceDepends(si.Source);
          if (d && !d->empty()) {
            tool = "None";
          }
        }
        break;
      case cmGeneratorTarget::SourceKindExtra:
        this->WriteExtraSource(e1, si.Source);
        break;
      case cmGeneratorTarget::SourceKindHeader:
        this->WriteHeaderSource(e1, si.Source);
        break;
      case cmGeneratorTarget::SourceKindIDL:
        tool = "Midl";
        break;
      case cmGeneratorTarget::SourceKindManifest:
        // Handled elsewhere.
        break;
      case cmGeneratorTarget::SourceKindModuleDefinition:
        tool = "None";
        break;
      case cmGeneratorTarget::SourceKindObjectSource: {
        const std::string& lang = si.Source->GetLanguage();
        if (lang == "C" || lang == "CXX") {
          tool = "ClCompile";
        } else if (lang == "ASM_MASM" &&
                   this->GlobalGenerator->IsMasmEnabled()) {
          tool = "MASM";
        } else if (lang == "ASM_NASM" &&
                   this->GlobalGenerator->IsNasmEnabled()) {
          tool = "NASM";
        } else if (lang == "RC") {
          tool = "ResourceCompile";
        } else if (lang == "CSharp") {
          tool = "Compile";
        } else if (lang == "CUDA" && this->GlobalGenerator->IsCudaEnabled()) {
          tool = "CudaCompile";
        } else {
          tool = "None";
        }
      } break;
      case cmGeneratorTarget::SourceKindResx:
        // Handled elsewhere.
        break;
      case cmGeneratorTarget::SourceKindXaml:
        // Handled elsewhere.
        break;
    }

    if (tool) {
      // Compute set of configurations to exclude, if any.
      std::vector<size_t> const& include_configs = si.Configs;
      std::vector<size_t> exclude_configs;
      std::set_difference(all_configs.begin(), all_configs.end(),
                          include_configs.begin(), include_configs.end(),
                          std::back_inserter(exclude_configs));

      Elem e2(e1);
      this->WriteSource(e2, tool, si.Source);
      if (si.Kind == cmGeneratorTarget::SourceKindObjectSource) {
        this->OutputSourceSpecificFlags(e2, si.Source);
      }
      if (!exclude_configs.empty()) {
        this->WriteExcludeFromBuild(e2, exclude_configs);
      }
    }
  }

  if (this->IsMissingFiles) {
    this->WriteMissingFiles(e1);
  }
}

void cmVisualStudio10TargetGenerator::OutputSourceSpecificFlags(
  Elem& e2, cmSourceFile const* source)
{
  cmSourceFile const& sf = *source;

  std::string objectName;
  if (this->GeneratorTarget->HasExplicitObjectName(&sf)) {
    objectName = this->GeneratorTarget->GetObjectName(&sf);
  }
  std::string flags;
  bool configDependentFlags = false;
  std::string options;
  bool configDependentOptions = false;
  std::string defines;
  bool configDependentDefines = false;
  std::string includes;
  bool configDependentIncludes = false;
  if (const char* cflags = sf.GetProperty("COMPILE_FLAGS")) {
    configDependentFlags =
      cmGeneratorExpression::Find(cflags) != std::string::npos;
    flags += cflags;
  }
  if (const char* coptions = sf.GetProperty("COMPILE_OPTIONS")) {
    configDependentOptions =
      cmGeneratorExpression::Find(coptions) != std::string::npos;
    options += coptions;
  }
  if (const char* cdefs = sf.GetProperty("COMPILE_DEFINITIONS")) {
    configDependentDefines =
      cmGeneratorExpression::Find(cdefs) != std::string::npos;
    defines += cdefs;
  }
  if (const char* cincludes = sf.GetProperty("INCLUDE_DIRECTORIES")) {
    configDependentIncludes =
      cmGeneratorExpression::Find(cincludes) != std::string::npos;
    includes += cincludes;
  }
  std::string lang =
    this->GlobalGenerator->GetLanguageFromExtension(sf.GetExtension().c_str());
  std::string sourceLang = this->LocalGenerator->GetSourceFileLanguage(sf);
  const std::string& linkLanguage =
    this->GeneratorTarget->GetLinkerLanguage("");
  bool needForceLang = false;
  // source file does not match its extension language
  if (lang != sourceLang) {
    needForceLang = true;
    lang = sourceLang;
  }
  // if the source file does not match the linker language
  // then force c or c++
  const char* compileAs = 0;
  if (needForceLang || (linkLanguage != lang)) {
    if (lang == "CXX") {
      // force a C++ file type
      compileAs = "CompileAsCpp";
    } else if (lang == "C") {
      // force to c
      compileAs = "CompileAsC";
    }
  }
  bool noWinRT = this->TargetCompileAsWinRT && lang == "C";
  // for the first time we need a new line if there is something
  // produced here.
  if (!objectName.empty()) {
    if (lang == "CUDA") {
      e2.Element("CompileOut", "$(IntDir)/" + objectName);
    } else {
      e2.Element("ObjectFileName", "$(IntDir)/" + objectName);
    }
  }
  for (std::string const& config : this->Configurations) {
    std::string configUpper = cmSystemTools::UpperCase(config);
    std::string configDefines = defines;
    std::string defPropName = "COMPILE_DEFINITIONS_";
    defPropName += configUpper;
    if (const char* ccdefs = sf.GetProperty(defPropName)) {
      if (!configDefines.empty()) {
        configDefines += ";";
      }
      configDependentDefines |=
        cmGeneratorExpression::Find(ccdefs) != std::string::npos;
      configDefines += ccdefs;
    }
    // if we have flags or defines for this config then
    // use them
    if (!flags.empty() || !options.empty() || !configDefines.empty() ||
        !includes.empty() || compileAs || noWinRT) {
      cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
      cmIDEFlagTable const* flagtable = nullptr;
      const std::string& srclang = source->GetLanguage();
      if (srclang == "C" || srclang == "CXX") {
        flagtable = gg->GetClFlagTable();
      } else if (srclang == "ASM_MASM" &&
                 this->GlobalGenerator->IsMasmEnabled()) {
        flagtable = gg->GetMasmFlagTable();
      } else if (lang == "ASM_NASM" &&
                 this->GlobalGenerator->IsNasmEnabled()) {
        flagtable = gg->GetNasmFlagTable();
      } else if (srclang == "RC") {
        flagtable = gg->GetRcFlagTable();
      } else if (srclang == "CSharp") {
        flagtable = gg->GetCSharpFlagTable();
      }
      cmGeneratorExpressionInterpreter genexInterpreter(
        this->LocalGenerator, config, this->GeneratorTarget, lang);
      cmVS10GeneratorOptions clOptions(
        this->LocalGenerator, cmVisualStudioGeneratorOptions::Compiler,
        flagtable, this);
      if (compileAs) {
        clOptions.AddFlag("CompileAs", compileAs);
      }
      if (noWinRT) {
        clOptions.AddFlag("CompileAsWinRT", "false");
      }
      if (configDependentFlags) {
        clOptions.Parse(genexInterpreter.Evaluate(flags, "COMPILE_FLAGS"));
      } else {
        clOptions.Parse(flags);
      }
      if (!options.empty()) {
        std::string expandedOptions;
        if (configDependentOptions) {
          this->LocalGenerator->AppendCompileOptions(
            expandedOptions,
            genexInterpreter.Evaluate(options, "COMPILE_OPTIONS"));
        } else {
          this->LocalGenerator->AppendCompileOptions(expandedOptions, options);
        }
        clOptions.Parse(expandedOptions);
      }
      if (clOptions.HasFlag("DisableSpecificWarnings")) {
        clOptions.AppendFlag("DisableSpecificWarnings",
                             "%(DisableSpecificWarnings)");
      }
      if (configDependentDefines) {
        clOptions.AddDefines(
          genexInterpreter.Evaluate(configDefines, "COMPILE_DEFINITIONS"));
      } else {
        clOptions.AddDefines(configDefines);
      }
      std::vector<std::string> includeList;
      if (configDependentIncludes) {
        this->LocalGenerator->AppendIncludeDirectories(
          includeList,
          genexInterpreter.Evaluate(includes, "INCLUDE_DIRECTORIES"), *source);
      } else {
        this->LocalGenerator->AppendIncludeDirectories(includeList, includes,
                                                       *source);
      }
      clOptions.AddIncludes(includeList);
      clOptions.SetConfiguration(config);
      OptionsHelper oh(clOptions, e2);
      oh.PrependInheritedString("AdditionalOptions");
      oh.OutputAdditionalIncludeDirectories(lang);
      oh.OutputFlagMap();
      oh.OutputPreprocessorDefinitions(lang);
    }
  }
  if (this->IsXamlSource(source->GetFullPath())) {
    const std::string& fileName = source->GetFullPath();
    std::string xamlFileName = fileName.substr(0, fileName.find_last_of("."));
    e2.Element("DependentUpon", xamlFileName);
  }
  if (this->ProjectType == csproj) {
    std::string f = source->GetFullPath();
    typedef std::map<std::string, std::string> CsPropMap;
    CsPropMap sourceFileTags;
    // set <Link> tag if necessary
    std::string link;
    this->GetCSharpSourceLink(source, link);
    if (!link.empty()) {
      sourceFileTags["Link"] = link;
    }
    this->GetCSharpSourceProperties(&sf, sourceFileTags);
    // write source file specific tags
    if (!sourceFileTags.empty()) {
      this->WriteCSharpSourceProperties(e2, sourceFileTags);
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteExcludeFromBuild(
  Elem& e2, std::vector<size_t> const& exclude_configs)
{
  for (size_t ci : exclude_configs) {
    e2.WritePlatformConfigTag("ExcludedFromBuild",
                              "'$(Configuration)|$(Platform)'=='" +
                                this->Configurations[ci] + "|" +
                                this->Platform + "'",
                              "true");
  }
}

void cmVisualStudio10TargetGenerator::WritePathAndIncrementalLinkOptions(
  Elem& e0)
{
  cmStateEnums::TargetType ttype = this->GeneratorTarget->GetType();
  if (ttype > cmStateEnums::GLOBAL_TARGET) {
    return;
  }
  if (this->ProjectType == csproj) {
    return;
  }

  Elem e1(e0, "PropertyGroup");
  e1.Element("_ProjectFileVersion", "10.0.20506.1");
  for (std::string const& config : this->Configurations) {
    const std::string cond = this->CalcCondition(config);
    if (ttype >= cmStateEnums::UTILITY) {
      e1.WritePlatformConfigTag(
        "IntDir", cond, "$(Platform)\\$(Configuration)\\$(ProjectName)\\");
    } else {
      std::string intermediateDir =
        this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
      intermediateDir += "/";
      intermediateDir += config;
      intermediateDir += "/";
      std::string outDir;
      std::string targetNameFull;
      if (ttype == cmStateEnums::OBJECT_LIBRARY) {
        outDir = intermediateDir;
        targetNameFull = this->GeneratorTarget->GetName();
        targetNameFull += ".lib";
      } else {
        outDir = this->GeneratorTarget->GetDirectory(config) + "/";
        targetNameFull = this->GeneratorTarget->GetFullName(config);
      }
      ConvertToWindowsSlash(intermediateDir);
      ConvertToWindowsSlash(outDir);

      e1.WritePlatformConfigTag("OutDir", cond, outDir);

      e1.WritePlatformConfigTag("IntDir", cond, intermediateDir);

      if (const char* sdkExecutableDirectories = this->Makefile->GetDefinition(
            "CMAKE_VS_SDK_EXECUTABLE_DIRECTORIES")) {
        e1.WritePlatformConfigTag("ExecutablePath", cond,
                                  sdkExecutableDirectories);
      }

      if (const char* sdkIncludeDirectories = this->Makefile->GetDefinition(
            "CMAKE_VS_SDK_INCLUDE_DIRECTORIES")) {
        e1.WritePlatformConfigTag("IncludePath", cond, sdkIncludeDirectories);
      }

      if (const char* sdkReferenceDirectories = this->Makefile->GetDefinition(
            "CMAKE_VS_SDK_REFERENCE_DIRECTORIES")) {
        e1.WritePlatformConfigTag("ReferencePath", cond,
                                  sdkReferenceDirectories);
      }

      if (const char* sdkLibraryDirectories = this->Makefile->GetDefinition(
            "CMAKE_VS_SDK_LIBRARY_DIRECTORIES")) {
        e1.WritePlatformConfigTag("LibraryPath", cond, sdkLibraryDirectories);
      }

      if (const char* sdkLibraryWDirectories = this->Makefile->GetDefinition(
            "CMAKE_VS_SDK_LIBRARY_WINRT_DIRECTORIES")) {
        e1.WritePlatformConfigTag("LibraryWPath", cond,
                                  sdkLibraryWDirectories);
      }

      if (const char* sdkSourceDirectories =
            this->Makefile->GetDefinition("CMAKE_VS_SDK_SOURCE_DIRECTORIES")) {
        e1.WritePlatformConfigTag("SourcePath", cond, sdkSourceDirectories);
      }

      if (const char* sdkExcludeDirectories = this->Makefile->GetDefinition(
            "CMAKE_VS_SDK_EXCLUDE_DIRECTORIES")) {
        e1.WritePlatformConfigTag("ExcludePath", cond, sdkExcludeDirectories);
      }

      if (const char* workingDir = this->GeneratorTarget->GetProperty(
            "VS_DEBUGGER_WORKING_DIRECTORY")) {
        cmGeneratorExpression ge;
        std::unique_ptr<cmCompiledGeneratorExpression> cge =
          ge.Parse(workingDir);
        std::string genWorkingDir =
          cge->Evaluate(this->LocalGenerator, config);

        e1.WritePlatformConfigTag("LocalDebuggerWorkingDirectory", cond,
                                  genWorkingDir);
      }

      if (const char* environment =
            this->GeneratorTarget->GetProperty("VS_DEBUGGER_ENVIRONMENT")) {
        cmGeneratorExpression ge;
        std::unique_ptr<cmCompiledGeneratorExpression> cge =
          ge.Parse(environment);
        std::string genEnvironment =
          cge->Evaluate(this->LocalGenerator, config);

        e1.WritePlatformConfigTag("LocalDebuggerEnvironment", cond,
                                  genEnvironment);
      }

      if (const char* debuggerCommand =
            this->GeneratorTarget->GetProperty("VS_DEBUGGER_COMMAND")) {

        cmGeneratorExpression ge;
        std::unique_ptr<cmCompiledGeneratorExpression> cge =
          ge.Parse(debuggerCommand);
        std::string genDebuggerCommand =
          cge->Evaluate(this->LocalGenerator, config);

        e1.WritePlatformConfigTag("LocalDebuggerCommand", cond,
                                  genDebuggerCommand);
      }

      if (const char* commandArguments = this->GeneratorTarget->GetProperty(
            "VS_DEBUGGER_COMMAND_ARGUMENTS")) {
        cmGeneratorExpression ge;
        std::unique_ptr<cmCompiledGeneratorExpression> cge =
          ge.Parse(commandArguments);
        std::string genCommandArguments =
          cge->Evaluate(this->LocalGenerator, config);

        e1.WritePlatformConfigTag("LocalDebuggerCommandArguments", cond,
                                  genCommandArguments);
      }

      std::string name =
        cmSystemTools::GetFilenameWithoutLastExtension(targetNameFull);
      e1.WritePlatformConfigTag("TargetName", cond, name);

      std::string ext =
        cmSystemTools::GetFilenameLastExtension(targetNameFull);
      if (ext.empty()) {
        // An empty TargetExt causes a default extension to be used.
        // A single "." appears to be treated as an empty extension.
        ext = ".";
      }
      e1.WritePlatformConfigTag("TargetExt", cond, ext);

      this->OutputLinkIncremental(e1, config);
    }
  }
}

void cmVisualStudio10TargetGenerator::OutputLinkIncremental(
  Elem& e1, std::string const& configName)
{
  if (!this->MSTools) {
    return;
  }
  if (this->ProjectType == csproj) {
    return;
  }
  // static libraries and things greater than modules do not need
  // to set this option
  if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY ||
      this->GeneratorTarget->GetType() > cmStateEnums::MODULE_LIBRARY) {
    return;
  }
  Options& linkOptions = *(this->LinkOptions[configName]);
  const std::string cond = this->CalcCondition(configName);

  if (this->IPOEnabledConfigurations.count(configName) == 0) {
    const char* incremental = linkOptions.GetFlag("LinkIncremental");
    e1.WritePlatformConfigTag("LinkIncremental", cond,
                              (incremental ? incremental : "true"));
  }
  linkOptions.RemoveFlag("LinkIncremental");

  const char* manifest = linkOptions.GetFlag("GenerateManifest");
  e1.WritePlatformConfigTag("GenerateManifest", cond,
                            (manifest ? manifest : "true"));
  linkOptions.RemoveFlag("GenerateManifest");

  // Some link options belong here.  Use them now and remove them so that
  // WriteLinkOptions does not use them.
  const char* flags[] = { "LinkDelaySign", "LinkKeyFile", 0 };
  for (const char** f = flags; *f; ++f) {
    const char* flag = *f;
    if (const char* value = linkOptions.GetFlag(flag)) {
      e1.WritePlatformConfigTag(flag, cond, value);
      linkOptions.RemoveFlag(flag);
    }
  }
}

std::vector<std::string> cmVisualStudio10TargetGenerator::GetIncludes(
  std::string const& config, std::string const& lang) const
{
  std::vector<std::string> includes;
  this->LocalGenerator->GetIncludeDirectories(includes, this->GeneratorTarget,
                                              lang, config);
  for (std::string& i : includes) {
    ConvertToWindowsSlash(i);
  }
  return includes;
}

bool cmVisualStudio10TargetGenerator::ComputeClOptions()
{
  for (std::string const& c : this->Configurations) {
    if (!this->ComputeClOptions(c)) {
      return false;
    }
  }
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeClOptions(
  std::string const& configName)
{
  // much of this was copied from here:
  // copied from cmLocalVisualStudio7Generator.cxx 805
  // TODO: Integrate code below with cmLocalVisualStudio7Generator.

  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  std::unique_ptr<Options> pOptions;
  switch (this->ProjectType) {
    case vcxproj:
      pOptions = cm::make_unique<Options>(
        this->LocalGenerator, Options::Compiler, gg->GetClFlagTable());
      break;
    case csproj:
      pOptions =
        cm::make_unique<Options>(this->LocalGenerator, Options::CSharpCompiler,
                                 gg->GetCSharpFlagTable());
      break;
  }
  Options& clOptions = *pOptions;

  std::string flags;
  const std::string& linkLanguage =
    this->GeneratorTarget->GetLinkerLanguage(configName);
  if (linkLanguage.empty()) {
    cmSystemTools::Error(
      "CMake can not determine linker language for target: ",
      this->Name.c_str());
    return false;
  }

  // Choose a language whose flags to use for ClCompile.
  static const char* clLangs[] = { "CXX", "C", "Fortran" };
  std::string langForClCompile;
  if (this->ProjectType == csproj) {
    langForClCompile = "CSharp";
  } else if (std::find(cm::cbegin(clLangs), cm::cend(clLangs), linkLanguage) !=
             cm::cend(clLangs)) {
    langForClCompile = linkLanguage;
  } else {
    std::set<std::string> languages;
    this->GeneratorTarget->GetLanguages(languages, configName);
    for (const char* const* l = cm::cbegin(clLangs); l != cm::cend(clLangs);
         ++l) {
      if (languages.find(*l) != languages.end()) {
        langForClCompile = *l;
        break;
      }
    }
  }
  this->LangForClCompile = langForClCompile;
  if (!langForClCompile.empty()) {
    std::string baseFlagVar = "CMAKE_";
    baseFlagVar += langForClCompile;
    baseFlagVar += "_FLAGS";
    flags = this->Makefile->GetRequiredDefinition(baseFlagVar);
    std::string flagVar =
      baseFlagVar + "_" + cmSystemTools::UpperCase(configName);
    flags += " ";
    flags += this->Makefile->GetRequiredDefinition(flagVar);
    this->LocalGenerator->AddCompileOptions(flags, this->GeneratorTarget,
                                            langForClCompile, configName);
  }
  // set the correct language
  if (linkLanguage == "C") {
    clOptions.AddFlag("CompileAs", "CompileAsC");
  }
  if (linkLanguage == "CXX") {
    clOptions.AddFlag("CompileAs", "CompileAsCpp");
  }

  // Put the IPO enabled configurations into a set.
  if (this->GeneratorTarget->IsIPOEnabled(linkLanguage, configName)) {
    this->IPOEnabledConfigurations.insert(configName);
  }

  // Get preprocessor definitions for this directory.
  std::string defineFlags = this->Makefile->GetDefineFlags();
  if (this->MSTools) {
    if (this->ProjectType == vcxproj) {
      clOptions.FixExceptionHandlingDefault();
      if (this->GlobalGenerator->GetVersion() >=
          cmGlobalVisualStudioGenerator::VS15) {
        // Toolsets that come with VS 2017 may now enable UseFullPaths
        // by default and there is no negative /FC option that projects
        // can use to switch it back.  Older toolsets disable this by
        // default anyway so this will not hurt them.  If the project
        // is using an explicit /FC option then parsing flags will
        // replace this setting with "true" below.
        clOptions.AddFlag("UseFullPaths", "false");
      }
      clOptions.AddFlag("PrecompiledHeader", "NotUsing");
      std::string asmLocation = configName + "/";
      clOptions.AddFlag("AssemblerListingLocation", asmLocation);
    }
  }

  // check for managed C++ assembly compiler flag. This overrides any
  // /clr* compiler flags which may be defined in the flags variable(s).
  if (this->ProjectType != csproj) {
    // Warn if /clr was added manually. This should not be done
    // anymore, because cmGeneratorTarget may not be aware that the
    // target uses C++/CLI.
    if (flags.find("/clr") != std::string::npos ||
        defineFlags.find("/clr") != std::string::npos) {
      if (configName == this->Configurations[0]) {
        std::string message = "For the target \"" +
          this->GeneratorTarget->GetName() +
          "\" the /clr compiler flag was added manually. " +
          "Set usage of C++/CLI by setting COMMON_LANGUAGE_RUNTIME "
          "target property.";
        this->Makefile->IssueMessage(cmake::MessageType::WARNING, message);
      }
    }
    if (auto* clr =
          this->GeneratorTarget->GetProperty("COMMON_LANGUAGE_RUNTIME")) {
      std::string clrString = clr;
      if (!clrString.empty()) {
        clrString = ":" + clrString;
      }
      flags += " /clr" + clrString;
    }
  }

  clOptions.Parse(flags);
  clOptions.Parse(defineFlags);
  std::vector<std::string> targetDefines;
  switch (this->ProjectType) {
    case vcxproj:
      if (!langForClCompile.empty()) {
        this->GeneratorTarget->GetCompileDefinitions(targetDefines, configName,
                                                     langForClCompile);
      }
      break;
    case csproj:
      this->GeneratorTarget->GetCompileDefinitions(targetDefines, configName,
                                                   "CSharp");
      break;
  }
  clOptions.AddDefines(targetDefines);

  // Get includes for this target
  if (!this->LangForClCompile.empty()) {
    clOptions.AddIncludes(
      this->GetIncludes(configName, this->LangForClCompile));
  }

  if (this->MSTools) {
    clOptions.SetVerboseMakefile(
      this->Makefile->IsOn("CMAKE_VERBOSE_MAKEFILE"));
  }

  // Add a definition for the configuration name.
  std::string configDefine = "CMAKE_INTDIR=\"";
  configDefine += configName;
  configDefine += "\"";
  clOptions.AddDefine(configDefine);
  if (const char* exportMacro = this->GeneratorTarget->GetExportMacro()) {
    clOptions.AddDefine(exportMacro);
  }

  if (this->MSTools) {
    // If we have the VS_WINRT_COMPONENT or CMAKE_VS_WINRT_BY_DEFAULT
    // set then force Compile as WinRT.
    if (this->GeneratorTarget->GetPropertyAsBool("VS_WINRT_COMPONENT") ||
        this->Makefile->IsOn("CMAKE_VS_WINRT_BY_DEFAULT")) {
      clOptions.AddFlag("CompileAsWinRT", "true");
      // For WinRT components, add the _WINRT_DLL define to produce a lib
      if (this->GeneratorTarget->GetType() == cmStateEnums::SHARED_LIBRARY ||
          this->GeneratorTarget->GetType() == cmStateEnums::MODULE_LIBRARY) {
        clOptions.AddDefine("_WINRT_DLL");
      }
    } else if (this->GlobalGenerator->TargetsWindowsStore() ||
               this->GlobalGenerator->TargetsWindowsPhone()) {
      if (!clOptions.IsWinRt()) {
        clOptions.AddFlag("CompileAsWinRT", "false");
      }
    }
    if (const char* winRT = clOptions.GetFlag("CompileAsWinRT")) {
      if (cmSystemTools::IsOn(winRT)) {
        this->TargetCompileAsWinRT = true;
      }
    }
  }

  if (this->ProjectType != csproj && clOptions.IsManaged()) {
    this->Managed = true;
    std::string managedType = clOptions.GetFlag("CompileAsManaged");
    if (managedType == "Safe" || managedType == "Pure") {
      // force empty calling convention if safe clr is used
      clOptions.AddFlag("CallingConvention", "");
    }
    // The default values of these flags are incompatible to
    // managed assemblies. We have to force valid values if
    // the target is a managed C++ target.
    clOptions.AddFlag("ExceptionHandling", "Async");
    clOptions.AddFlag("BasicRuntimeChecks", "Default");
  }
  if (this->ProjectType == csproj) {
    // /nowin32manifest overrides /win32manifest: parameter
    if (clOptions.HasFlag("NoWin32Manifest")) {
      clOptions.RemoveFlag("ApplicationManifest");
    }
  }

  if (clOptions.HasFlag("SpectreMitigation")) {
    this->SpectreMitigationConfigurations.insert(configName);
    clOptions.RemoveFlag("SpectreMitigation");
  }

  this->ClOptions[configName] = std::move(pOptions);
  return true;
}

void cmVisualStudio10TargetGenerator::WriteClOptions(
  Elem& e1, std::string const& configName)
{
  Options& clOptions = *(this->ClOptions[configName]);
  if (this->ProjectType == csproj) {
    return;
  }
  Elem e2(e1, "ClCompile");
  OptionsHelper oh(clOptions, e2);
  oh.PrependInheritedString("AdditionalOptions");
  oh.OutputAdditionalIncludeDirectories(this->LangForClCompile);
  oh.OutputFlagMap();
  oh.OutputPreprocessorDefinitions(this->LangForClCompile);

  if (this->NsightTegra) {
    if (const char* processMax =
          this->GeneratorTarget->GetProperty("ANDROID_PROCESS_MAX")) {
      e2.Element("ProcessMax", processMax);
    }
  }

  if (this->MSTools) {
    cmsys::RegularExpression clangToolset("v[0-9]+_clang_.*");
    const char* toolset = this->GlobalGenerator->GetPlatformToolset();
    if (toolset && clangToolset.find(toolset)) {
      e2.Element("ObjectFileName", "$(IntDir)%(filename).obj");
    } else {
      e2.Element("ObjectFileName", "$(IntDir)");
    }

    // If not in debug mode, write the DebugInformationFormat field
    // without value so PDBs don't get generated uselessly. Each tag
    // goes on its own line because Visual Studio corrects it this
    // way when saving the project after CMake generates it.
    if (!clOptions.IsDebug()) {
      Elem e3(e2, "DebugInformationFormat");
      e3.SetHasElements();
    }

    // Specify the compiler program database file if configured.
    std::string pdb = this->GeneratorTarget->GetCompilePDBPath(configName);
    if (!pdb.empty()) {
      if (this->GlobalGenerator->IsCudaEnabled()) {
        // CUDA does not quote paths with spaces correctly when forwarding
        // this to the host compiler.  Use a relative path to avoid spaces.
        // FIXME: We can likely do this even when CUDA is not involved,
        // but for now we will make a minimal change.
        pdb = this->ConvertPath(pdb, true);
      }
      ConvertToWindowsSlash(pdb);
      e2.Element("ProgramDataBaseFileName", pdb);
    }

    // add AdditionalUsingDirectories
    if (this->AdditionalUsingDirectories.count(configName) > 0) {
      std::string dirs;
      for (auto u : this->AdditionalUsingDirectories[configName]) {
        if (!dirs.empty()) {
          dirs.append(";");
        }
        dirs.append(u);
      }
      e2.Element("AdditionalUsingDirectories", dirs);
    }
  }
}

bool cmVisualStudio10TargetGenerator::ComputeRcOptions()
{
  for (std::string const& c : this->Configurations) {
    if (!this->ComputeRcOptions(c)) {
      return false;
    }
  }
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeRcOptions(
  std::string const& configName)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  auto pOptions = cm::make_unique<Options>(
    this->LocalGenerator, Options::ResourceCompiler, gg->GetRcFlagTable());
  Options& rcOptions = *pOptions;

  std::string CONFIG = cmSystemTools::UpperCase(configName);
  std::string rcConfigFlagsVar = "CMAKE_RC_FLAGS_" + CONFIG;
  std::string flags = this->Makefile->GetSafeDefinition("CMAKE_RC_FLAGS") +
    " " + this->Makefile->GetSafeDefinition(rcConfigFlagsVar);

  rcOptions.Parse(flags);

  // For historical reasons, add the C preprocessor defines to RC.
  Options& clOptions = *(this->ClOptions[configName]);
  rcOptions.AddDefines(clOptions.GetDefines());

  // Get includes for this target
  rcOptions.AddIncludes(this->GetIncludes(configName, "RC"));

  this->RcOptions[configName] = std::move(pOptions);
  return true;
}

void cmVisualStudio10TargetGenerator::WriteRCOptions(
  Elem& e1, std::string const& configName)
{
  if (!this->MSTools) {
    return;
  }
  Elem e2(e1, "ResourceCompile");

  OptionsHelper rcOptions(*(this->RcOptions[configName]), e2);
  rcOptions.OutputPreprocessorDefinitions("RC");
  rcOptions.OutputAdditionalIncludeDirectories("RC");
  rcOptions.PrependInheritedString("AdditionalOptions");
  rcOptions.OutputFlagMap();
}

bool cmVisualStudio10TargetGenerator::ComputeCudaOptions()
{
  if (!this->GlobalGenerator->IsCudaEnabled()) {
    return true;
  }
  for (std::string const& c : this->Configurations) {
    if (!this->ComputeCudaOptions(c)) {
      return false;
    }
  }
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeCudaOptions(
  std::string const& configName)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  auto pOptions = cm::make_unique<Options>(
    this->LocalGenerator, Options::CudaCompiler, gg->GetCudaFlagTable());
  Options& cudaOptions = *pOptions;

  // Get compile flags for CUDA in this directory.
  std::string CONFIG = cmSystemTools::UpperCase(configName);
  std::string configFlagsVar = std::string("CMAKE_CUDA_FLAGS_") + CONFIG;
  std::string flags = this->Makefile->GetSafeDefinition("CMAKE_CUDA_FLAGS") +
    " " + this->Makefile->GetSafeDefinition(configFlagsVar);
  this->LocalGenerator->AddCompileOptions(flags, this->GeneratorTarget, "CUDA",
                                          configName);

  // Get preprocessor definitions for this directory.
  std::string defineFlags = this->Makefile->GetDefineFlags();

  cudaOptions.Parse(flags);
  cudaOptions.Parse(defineFlags);
  cudaOptions.ParseFinish();

  // If we haven't explicitly enabled GPU debug information
  // explicitly disable it
  if (!cudaOptions.HasFlag("GPUDebugInfo")) {
    cudaOptions.AddFlag("GPUDebugInfo", "false");
  }

  // The extension on object libraries the CUDA gives isn't
  // consistent with how MSVC generates object libraries for C+, so set
  // the default to not have any extension
  cudaOptions.AddFlag("CompileOut", "$(IntDir)%(Filename).obj");

  bool notPtx = true;
  if (this->GeneratorTarget->GetPropertyAsBool("CUDA_SEPARABLE_COMPILATION")) {
    cudaOptions.AddFlag("GenerateRelocatableDeviceCode", "true");
  } else if (this->GeneratorTarget->GetPropertyAsBool(
               "CUDA_PTX_COMPILATION")) {
    cudaOptions.AddFlag("NvccCompilation", "ptx");
    // We drop the %(Extension) component as CMake expects all PTX files
    // to not have the source file extension at all
    cudaOptions.AddFlag("CompileOut", "$(IntDir)%(Filename).ptx");
    notPtx = false;
  }

  if (notPtx &&
      cmSystemTools::VersionCompareGreaterEq(
        "8.0", this->GlobalGenerator->GetPlatformToolsetCudaString())) {
    // Explicitly state that we want this file to be treated as a
    // CUDA file no matter what the file extensions is
    // This is only needed for < CUDA 9
    cudaOptions.AppendFlagString("AdditionalOptions", "-x cu");
  }

  // Specify the compiler program database file if configured.
  std::string pdb = this->GeneratorTarget->GetCompilePDBPath(configName);
  if (!pdb.empty()) {
    // CUDA does not make the directory if it is non-standard.
    std::string const pdbDir = cmSystemTools::GetFilenamePath(pdb);
    cmSystemTools::MakeDirectory(pdbDir);
    if (cmSystemTools::VersionCompareGreaterEq(
          "9.2", this->GlobalGenerator->GetPlatformToolsetCudaString())) {
      // CUDA does not have a field for this and does not honor the
      // ProgramDataBaseFileName field in ClCompile.  Work around this
      // limitation by creating the directory and passing the flag ourselves.
      pdb = this->ConvertPath(pdb, true);
      ConvertToWindowsSlash(pdb);
      std::string const clFd = "-Xcompiler=\"-Fd\\\"" + pdb + "\\\"\"";
      cudaOptions.AppendFlagString("AdditionalOptions", clFd);
    }
  }

  // CUDA automatically passes the proper '--machine' flag to nvcc
  // for the current architecture, but does not reflect this default
  // in the user-visible IDE settings.  Set it explicitly.
  if (this->Platform == "x64") {
    cudaOptions.AddFlag("TargetMachinePlatform", "64");
  }

  // Convert the host compiler options to the toolset's abstractions
  // using a secondary flag table.
  cudaOptions.ClearTables();
  cudaOptions.AddTable(gg->GetCudaHostFlagTable());
  cudaOptions.Reparse("AdditionalCompilerOptions");

  // `CUDA 8.0.targets` places AdditionalCompilerOptions before nvcc!
  // Pass them through -Xcompiler in AdditionalOptions instead.
  if (const char* acoPtr = cudaOptions.GetFlag("AdditionalCompilerOptions")) {
    std::string aco = acoPtr;
    cudaOptions.RemoveFlag("AdditionalCompilerOptions");
    if (!aco.empty()) {
      aco = this->LocalGenerator->EscapeForShell(aco, false);
      cudaOptions.AppendFlagString("AdditionalOptions", "-Xcompiler=" + aco);
    }
  }

  cudaOptions.FixCudaCodeGeneration();

  std::vector<std::string> targetDefines;
  this->GeneratorTarget->GetCompileDefinitions(targetDefines, configName,
                                               "CUDA");
  cudaOptions.AddDefines(targetDefines);

  // Add a definition for the configuration name.
  std::string configDefine = "CMAKE_INTDIR=\"";
  configDefine += configName;
  configDefine += "\"";
  cudaOptions.AddDefine(configDefine);
  if (const char* exportMacro = this->GeneratorTarget->GetExportMacro()) {
    cudaOptions.AddDefine(exportMacro);
  }

  // Get includes for this target
  cudaOptions.AddIncludes(this->GetIncludes(configName, "CUDA"));
  cudaOptions.AddFlag("UseHostInclude", "false");

  this->CudaOptions[configName] = std::move(pOptions);
  return true;
}

void cmVisualStudio10TargetGenerator::WriteCudaOptions(
  Elem& e1, std::string const& configName)
{
  if (!this->MSTools || !this->GlobalGenerator->IsCudaEnabled()) {
    return;
  }
  Elem e2(e1, "CudaCompile");

  OptionsHelper cudaOptions(*(this->CudaOptions[configName]), e2);
  cudaOptions.OutputAdditionalIncludeDirectories("CUDA");
  cudaOptions.OutputPreprocessorDefinitions("CUDA");
  cudaOptions.PrependInheritedString("AdditionalOptions");
  cudaOptions.OutputFlagMap();
}

bool cmVisualStudio10TargetGenerator::ComputeCudaLinkOptions()
{
  if (!this->GlobalGenerator->IsCudaEnabled()) {
    return true;
  }
  for (std::string const& c : this->Configurations) {
    if (!this->ComputeCudaLinkOptions(c)) {
      return false;
    }
  }
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeCudaLinkOptions(
  std::string const& configName)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  auto pOptions = cm::make_unique<Options>(
    this->LocalGenerator, Options::CudaCompiler, gg->GetCudaFlagTable());
  Options& cudaLinkOptions = *pOptions;

  // Determine if we need to do a device link
  bool doDeviceLinking = false;
  switch (this->GeneratorTarget->GetType()) {
    case cmStateEnums::SHARED_LIBRARY:
    case cmStateEnums::MODULE_LIBRARY:
    case cmStateEnums::EXECUTABLE:
      doDeviceLinking = true;
      break;
    case cmStateEnums::STATIC_LIBRARY:
      doDeviceLinking = this->GeneratorTarget->GetPropertyAsBool(
        "CUDA_RESOLVE_DEVICE_SYMBOLS");
      break;
    default:
      break;
  }

  cudaLinkOptions.AddFlag("PerformDeviceLink",
                          doDeviceLinking ? "true" : "false");

  // Suppress deprecation warnings for default GPU targets during device link.
  if (cmSystemTools::VersionCompareGreaterEq(
        this->GlobalGenerator->GetPlatformToolsetCudaString(), "8.0")) {
    cudaLinkOptions.AppendFlagString("AdditionalOptions",
                                     "-Wno-deprecated-gpu-targets");
  }

  this->CudaLinkOptions[configName] = std::move(pOptions);
  return true;
}

void cmVisualStudio10TargetGenerator::WriteCudaLinkOptions(
  Elem& e1, std::string const& configName)
{
  if (this->GeneratorTarget->GetType() > cmStateEnums::MODULE_LIBRARY) {
    return;
  }

  if (!this->MSTools || !this->GlobalGenerator->IsCudaEnabled()) {
    return;
  }

  Elem e2(e1, "CudaLink");
  OptionsHelper cudaLinkOptions(*(this->CudaLinkOptions[configName]), e2);
  cudaLinkOptions.OutputFlagMap();
}

bool cmVisualStudio10TargetGenerator::ComputeMasmOptions()
{
  if (!this->GlobalGenerator->IsMasmEnabled()) {
    return true;
  }
  for (std::string const& c : this->Configurations) {
    if (!this->ComputeMasmOptions(c)) {
      return false;
    }
  }
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeMasmOptions(
  std::string const& configName)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  auto pOptions = cm::make_unique<Options>(
    this->LocalGenerator, Options::MasmCompiler, gg->GetMasmFlagTable());
  Options& masmOptions = *pOptions;

  std::string CONFIG = cmSystemTools::UpperCase(configName);
  std::string configFlagsVar = std::string("CMAKE_ASM_MASM_FLAGS_") + CONFIG;
  std::string flags =
    this->Makefile->GetSafeDefinition("CMAKE_ASM_MASM_FLAGS") + " " +
    this->Makefile->GetSafeDefinition(configFlagsVar);

  masmOptions.Parse(flags);

  // Get includes for this target
  masmOptions.AddIncludes(this->GetIncludes(configName, "ASM_MASM"));

  this->MasmOptions[configName] = std::move(pOptions);
  return true;
}

void cmVisualStudio10TargetGenerator::WriteMasmOptions(
  Elem& e1, std::string const& configName)
{
  if (!this->MSTools || !this->GlobalGenerator->IsMasmEnabled()) {
    return;
  }
  Elem e2(e1, "MASM");

  // Preprocessor definitions and includes are shared with clOptions.
  OptionsHelper clOptions(*(this->ClOptions[configName]), e2);
  clOptions.OutputPreprocessorDefinitions("ASM_MASM");

  OptionsHelper masmOptions(*(this->MasmOptions[configName]), e2);
  masmOptions.OutputAdditionalIncludeDirectories("ASM_MASM");
  masmOptions.PrependInheritedString("AdditionalOptions");
  masmOptions.OutputFlagMap();
}

bool cmVisualStudio10TargetGenerator::ComputeNasmOptions()
{
  if (!this->GlobalGenerator->IsNasmEnabled()) {
    return true;
  }
  for (std::string const& c : this->Configurations) {
    if (!this->ComputeNasmOptions(c)) {
      return false;
    }
  }
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeNasmOptions(
  std::string const& configName)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  auto pOptions = cm::make_unique<Options>(
    this->LocalGenerator, Options::NasmCompiler, gg->GetNasmFlagTable());
  Options& nasmOptions = *pOptions;

  std::string CONFIG = cmSystemTools::UpperCase(configName);
  std::string configFlagsVar = "CMAKE_ASM_NASM_FLAGS_" + CONFIG;
  std::string flags =
    this->Makefile->GetSafeDefinition("CMAKE_ASM_NASM_FLAGS") + " -f" +
    this->Makefile->GetSafeDefinition("CMAKE_ASM_NASM_OBJECT_FORMAT") + " " +
    this->Makefile->GetSafeDefinition(configFlagsVar);
  nasmOptions.Parse(flags);

  // Get includes for this target
  nasmOptions.AddIncludes(this->GetIncludes(configName, "ASM_NASM"));

  this->NasmOptions[configName] = std::move(pOptions);
  return true;
}

void cmVisualStudio10TargetGenerator::WriteNasmOptions(
  Elem& e1, std::string const& configName)
{
  if (!this->GlobalGenerator->IsNasmEnabled()) {
    return;
  }
  Elem e2(e1, "NASM");

  std::vector<std::string> includes =
    this->GetIncludes(configName, "ASM_NASM");
  OptionsHelper nasmOptions(*(this->NasmOptions[configName]), e2);
  nasmOptions.OutputAdditionalIncludeDirectories("ASM_NASM");
  nasmOptions.OutputFlagMap();
  nasmOptions.PrependInheritedString("AdditionalOptions");
  nasmOptions.OutputPreprocessorDefinitions("ASM_NASM");

  // Preprocessor definitions and includes are shared with clOptions.
  OptionsHelper clOptions(*(this->ClOptions[configName]), e2);
  clOptions.OutputPreprocessorDefinitions("ASM_NASM");
}

void cmVisualStudio10TargetGenerator::WriteLibOptions(
  Elem& e1, std::string const& config)
{
  if (this->GeneratorTarget->GetType() != cmStateEnums::STATIC_LIBRARY &&
      this->GeneratorTarget->GetType() != cmStateEnums::OBJECT_LIBRARY) {
    return;
  }

  const std::string& linkLanguage =
    this->GeneratorTarget->GetLinkClosure(config)->LinkerLanguage;

  std::string libflags;
  this->LocalGenerator->GetStaticLibraryFlags(
    libflags, cmSystemTools::UpperCase(config), linkLanguage,
    this->GeneratorTarget);
  if (!libflags.empty()) {
    Elem e2(e1, "Lib");
    cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
    cmVS10GeneratorOptions libOptions(this->LocalGenerator,
                                      cmVisualStudioGeneratorOptions::Linker,
                                      gg->GetLibFlagTable(), this);
    libOptions.Parse(libflags);
    OptionsHelper oh(libOptions, e2);
    oh.PrependInheritedString("AdditionalOptions");
    oh.OutputFlagMap();
  }

  // We cannot generate metadata for static libraries.  WindowsPhone
  // and WindowsStore tools look at GenerateWindowsMetadata in the
  // Link tool options even for static libraries.
  if (this->GlobalGenerator->TargetsWindowsPhone() ||
      this->GlobalGenerator->TargetsWindowsStore()) {
    Elem e2(e1, "Link");
    e2.Element("GenerateWindowsMetadata", "false");
  }
}

void cmVisualStudio10TargetGenerator::WriteManifestOptions(
  Elem& e1, std::string const& config)
{
  if (this->GeneratorTarget->GetType() != cmStateEnums::EXECUTABLE &&
      this->GeneratorTarget->GetType() != cmStateEnums::SHARED_LIBRARY &&
      this->GeneratorTarget->GetType() != cmStateEnums::MODULE_LIBRARY) {
    return;
  }

  std::vector<cmSourceFile const*> manifest_srcs;
  this->GeneratorTarget->GetManifests(manifest_srcs, config);
  if (!manifest_srcs.empty()) {
    std::ostringstream oss;
    for (cmSourceFile const* mi : manifest_srcs) {
      std::string m = this->ConvertPath(mi->GetFullPath(), false);
      ConvertToWindowsSlash(m);
      oss << m << ";";
    }
    Elem e2(e1, "Manifest");
    e2.Element("AdditionalManifestFiles", oss.str());
  }
}

void cmVisualStudio10TargetGenerator::WriteAntBuildOptions(
  Elem& e1, std::string const& configName)
{
  // Look through the sources for AndroidManifest.xml and use
  // its location as the root source directory.
  std::string rootDir = this->LocalGenerator->GetCurrentSourceDirectory();
  {
    std::vector<cmSourceFile const*> extraSources;
    this->GeneratorTarget->GetExtraSources(extraSources, "");
    for (cmSourceFile const* si : extraSources) {
      if ("androidmanifest.xml" ==
          cmSystemTools::LowerCase(si->GetLocation().GetName())) {
        rootDir = si->GetLocation().GetDirectory();
        break;
      }
    }
  }

  // Tell MSBuild to launch Ant.
  Elem e2(e1, "AntBuild");
  {
    std::string antBuildPath = rootDir;
    ConvertToWindowsSlash(antBuildPath);
    e2.Element("AntBuildPath", antBuildPath);
  }

  if (this->GeneratorTarget->GetPropertyAsBool("ANDROID_SKIP_ANT_STEP")) {
    e2.Element("SkipAntStep", "true");
  }

  if (this->GeneratorTarget->GetPropertyAsBool("ANDROID_PROGUARD")) {
    e2.Element("EnableProGuard", "true");
  }

  if (const char* proGuardConfigLocation =
        this->GeneratorTarget->GetProperty("ANDROID_PROGUARD_CONFIG_PATH")) {
    e2.Element("ProGuardConfigLocation", proGuardConfigLocation);
  }

  if (const char* securePropertiesLocation =
        this->GeneratorTarget->GetProperty("ANDROID_SECURE_PROPS_PATH")) {
    e2.Element("SecurePropertiesLocation", securePropertiesLocation);
  }

  if (const char* nativeLibDirectoriesExpression =
        this->GeneratorTarget->GetProperty("ANDROID_NATIVE_LIB_DIRECTORIES")) {
    cmGeneratorExpression ge;
    std::unique_ptr<cmCompiledGeneratorExpression> cge =
      ge.Parse(nativeLibDirectoriesExpression);
    std::string nativeLibDirs =
      cge->Evaluate(this->LocalGenerator, configName);
    e2.Element("NativeLibDirectories", nativeLibDirs);
  }

  if (const char* nativeLibDependenciesExpression =
        this->GeneratorTarget->GetProperty(
          "ANDROID_NATIVE_LIB_DEPENDENCIES")) {
    cmGeneratorExpression ge;
    std::unique_ptr<cmCompiledGeneratorExpression> cge =
      ge.Parse(nativeLibDependenciesExpression);
    std::string nativeLibDeps =
      cge->Evaluate(this->LocalGenerator, configName);
    e2.Element("NativeLibDependencies", nativeLibDeps);
  }

  if (const char* javaSourceDir =
        this->GeneratorTarget->GetProperty("ANDROID_JAVA_SOURCE_DIR")) {
    e2.Element("JavaSourceDir", javaSourceDir);
  }

  if (const char* jarDirectoriesExpression =
        this->GeneratorTarget->GetProperty("ANDROID_JAR_DIRECTORIES")) {
    cmGeneratorExpression ge;
    std::unique_ptr<cmCompiledGeneratorExpression> cge =
      ge.Parse(jarDirectoriesExpression);
    std::string jarDirectories =
      cge->Evaluate(this->LocalGenerator, configName);
    e2.Element("JarDirectories", jarDirectories);
  }

  if (const char* jarDeps =
        this->GeneratorTarget->GetProperty("ANDROID_JAR_DEPENDENCIES")) {
    e2.Element("JarDependencies", jarDeps);
  }

  if (const char* assetsDirectories =
        this->GeneratorTarget->GetProperty("ANDROID_ASSETS_DIRECTORIES")) {
    e2.Element("AssetsDirectories", assetsDirectories);
  }

  {
    std::string manifest_xml = rootDir + "/AndroidManifest.xml";
    ConvertToWindowsSlash(manifest_xml);
    e2.Element("AndroidManifestLocation", manifest_xml);
  }

  if (const char* antAdditionalOptions =
        this->GeneratorTarget->GetProperty("ANDROID_ANT_ADDITIONAL_OPTIONS")) {
    e2.Element("AdditionalOptions",
               std::string(antAdditionalOptions) + " %(AdditionalOptions)");
  }
}

bool cmVisualStudio10TargetGenerator::ComputeLinkOptions()
{
  if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE ||
      this->GeneratorTarget->GetType() == cmStateEnums::SHARED_LIBRARY ||
      this->GeneratorTarget->GetType() == cmStateEnums::MODULE_LIBRARY) {
    for (std::string const& c : this->Configurations) {
      if (!this->ComputeLinkOptions(c)) {
        return false;
      }
    }
  }
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeLinkOptions(
  std::string const& config)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  auto pOptions = cm::make_unique<Options>(
    this->LocalGenerator, Options::Linker, gg->GetLinkFlagTable(), this);
  Options& linkOptions = *pOptions;

  cmGeneratorTarget::LinkClosure const* linkClosure =
    this->GeneratorTarget->GetLinkClosure(config);

  const std::string& linkLanguage = linkClosure->LinkerLanguage;
  if (linkLanguage.empty()) {
    cmSystemTools::Error(
      "CMake can not determine linker language for target: ",
      this->Name.c_str());
    return false;
  }

  std::string CONFIG = cmSystemTools::UpperCase(config);

  const char* linkType = "SHARED";
  if (this->GeneratorTarget->GetType() == cmStateEnums::MODULE_LIBRARY) {
    linkType = "MODULE";
  }
  if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE) {
    linkType = "EXE";
  }
  std::string flags;
  std::string linkFlagVarBase = "CMAKE_";
  linkFlagVarBase += linkType;
  linkFlagVarBase += "_LINKER_FLAGS";
  flags += " ";
  flags += this->Makefile->GetRequiredDefinition(linkFlagVarBase);
  std::string linkFlagVar = linkFlagVarBase + "_" + CONFIG;
  flags += " ";
  flags += this->Makefile->GetRequiredDefinition(linkFlagVar);
  const char* targetLinkFlags =
    this->GeneratorTarget->GetProperty("LINK_FLAGS");
  if (targetLinkFlags) {
    flags += " ";
    flags += targetLinkFlags;
  }
  std::string flagsProp = "LINK_FLAGS_";
  flagsProp += CONFIG;
  if (const char* flagsConfig =
        this->GeneratorTarget->GetProperty(flagsProp)) {
    flags += " ";
    flags += flagsConfig;
  }

  std::vector<std::string> opts;
  this->GeneratorTarget->GetLinkOptions(opts, config, linkLanguage);
  // LINK_OPTIONS are escaped.
  this->LocalGenerator->AppendCompileOptions(flags, opts);

  cmComputeLinkInformation* pcli =
    this->GeneratorTarget->GetLinkInformation(config);
  if (!pcli) {
    cmSystemTools::Error(
      "CMake can not compute cmComputeLinkInformation for target: ",
      this->Name.c_str());
    return false;
  }
  cmComputeLinkInformation& cli = *pcli;

  std::vector<std::string> libVec;
  std::vector<std::string> vsTargetVec;
  this->AddLibraries(cli, libVec, vsTargetVec, config);
  if (std::find(linkClosure->Languages.begin(), linkClosure->Languages.end(),
                "CUDA") != linkClosure->Languages.end() &&
      this->CudaOptions[config] != nullptr) {
    switch (this->CudaOptions[config]->GetCudaRuntime()) {
      case cmVisualStudioGeneratorOptions::CudaRuntimeStatic:
        libVec.push_back("cudadevrt.lib");
        libVec.push_back("cudart_static.lib");
        break;
      case cmVisualStudioGeneratorOptions::CudaRuntimeShared:
        libVec.push_back("cudadevrt.lib");
        libVec.push_back("cudart.lib");
        break;
      case cmVisualStudioGeneratorOptions::CudaRuntimeNone:
        break;
    }
  }
  std::string standardLibsVar = "CMAKE_";
  standardLibsVar += linkLanguage;
  standardLibsVar += "_STANDARD_LIBRARIES";
  std::string const libs = this->Makefile->GetSafeDefinition(standardLibsVar);
  cmSystemTools::ParseWindowsCommandLine(libs.c_str(), libVec);
  linkOptions.AddFlag("AdditionalDependencies", libVec);

  // Populate TargetsFileAndConfigsVec
  for (std::string const& ti : vsTargetVec) {
    this->AddTargetsFileAndConfigPair(ti, config);
  }

  std::vector<std::string> const& ldirs = cli.GetDirectories();
  std::vector<std::string> linkDirs;
  for (std::string const& d : ldirs) {
    // first just full path
    linkDirs.push_back(d);
    // next path with configuration type Debug, Release, etc
    linkDirs.push_back(d + "/$(Configuration)");
  }
  linkDirs.push_back("%(AdditionalLibraryDirectories)");
  linkOptions.AddFlag("AdditionalLibraryDirectories", linkDirs);

  std::string targetName;
  std::string targetNameSO;
  std::string targetNameFull;
  std::string targetNameImport;
  std::string targetNamePDB;
  if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE) {
    this->GeneratorTarget->GetExecutableNames(
      targetName, targetNameFull, targetNameImport, targetNamePDB, config);
  } else {
    this->GeneratorTarget->GetLibraryNames(targetName, targetNameSO,
                                           targetNameFull, targetNameImport,
                                           targetNamePDB, config);
  }

  if (this->MSTools) {
    if (this->GeneratorTarget->GetPropertyAsBool("WIN32_EXECUTABLE")) {
      if (this->GlobalGenerator->TargetsWindowsCE()) {
        linkOptions.AddFlag("SubSystem", "WindowsCE");
        if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE) {
          if (this->ClOptions[config]->UsingUnicode()) {
            linkOptions.AddFlag("EntryPointSymbol", "wWinMainCRTStartup");
          } else {
            linkOptions.AddFlag("EntryPointSymbol", "WinMainCRTStartup");
          }
        }
      } else {
        linkOptions.AddFlag("SubSystem", "Windows");
      }
    } else {
      if (this->GlobalGenerator->TargetsWindowsCE()) {
        linkOptions.AddFlag("SubSystem", "WindowsCE");
        if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE) {
          if (this->ClOptions[config]->UsingUnicode()) {
            linkOptions.AddFlag("EntryPointSymbol", "mainWCRTStartup");
          } else {
            linkOptions.AddFlag("EntryPointSymbol", "mainACRTStartup");
          }
        }
      } else {
        linkOptions.AddFlag("SubSystem", "Console");
      };
    }

    if (const char* stackVal = this->Makefile->GetDefinition(
          "CMAKE_" + linkLanguage + "_STACK_SIZE")) {
      linkOptions.AddFlag("StackReserveSize", stackVal);
    }

    linkOptions.AddFlag("GenerateDebugInformation", "false");

    std::string pdb = this->GeneratorTarget->GetPDBDirectory(config);
    pdb += "/";
    pdb += targetNamePDB;
    std::string imLib = this->GeneratorTarget->GetDirectory(
      config, cmStateEnums::ImportLibraryArtifact);
    imLib += "/";
    imLib += targetNameImport;

    linkOptions.AddFlag("ImportLibrary", imLib);
    linkOptions.AddFlag("ProgramDataBaseFile", pdb);

    // A Windows Runtime component uses internal .NET metadata,
    // so does not have an import library.
    if (this->GeneratorTarget->GetPropertyAsBool("VS_WINRT_COMPONENT") &&
        this->GeneratorTarget->GetType() != cmStateEnums::EXECUTABLE) {
      linkOptions.AddFlag("GenerateWindowsMetadata", "true");
    } else if (this->GlobalGenerator->TargetsWindowsPhone() ||
               this->GlobalGenerator->TargetsWindowsStore()) {
      // WindowsPhone and WindowsStore components are in an app container
      // and produce WindowsMetadata.  If we are not producing a WINRT
      // component, then do not generate the metadata here.
      linkOptions.AddFlag("GenerateWindowsMetadata", "false");
    }

    if (this->GlobalGenerator->TargetsWindowsPhone() &&
        this->GlobalGenerator->GetSystemVersion() == "8.0") {
      // WindowsPhone 8.0 does not have ole32.
      linkOptions.AppendFlag("IgnoreSpecificDefaultLibraries", "ole32.lib");
    }
  } else if (this->NsightTegra) {
    linkOptions.AddFlag("SoName", targetNameSO);
  }

  linkOptions.Parse(flags);
  linkOptions.FixManifestUACFlags();

  if (this->MSTools) {
    cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
      this->GeneratorTarget->GetModuleDefinitionInfo(config);
    if (mdi && !mdi->DefFile.empty()) {
      linkOptions.AddFlag("ModuleDefinitionFile", mdi->DefFile);
    }
    linkOptions.AppendFlag("IgnoreSpecificDefaultLibraries",
                           "%(IgnoreSpecificDefaultLibraries)");
  }

  // VS 2015 without all updates has a v140 toolset whose
  // GenerateDebugInformation expects No/Debug instead of false/true.
  if (gg->GetPlatformToolsetNeedsDebugEnum()) {
    if (const char* debug = linkOptions.GetFlag("GenerateDebugInformation")) {
      if (strcmp(debug, "false") == 0) {
        linkOptions.AddFlag("GenerateDebugInformation", "No");
      } else if (strcmp(debug, "true") == 0) {
        linkOptions.AddFlag("GenerateDebugInformation", "Debug");
      }
    }
  }

  // Managed code cannot be linked with /DEBUG:FASTLINK
  if (this->Managed) {
    if (const char* debug = linkOptions.GetFlag("GenerateDebugInformation")) {
      if (strcmp(debug, "DebugFastLink") == 0) {
        linkOptions.AddFlag("GenerateDebugInformation", "Debug");
      }
    }
  }

  this->LinkOptions[config] = std::move(pOptions);
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeLibOptions()
{
  if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY) {
    for (std::string const& c : this->Configurations) {
      if (!this->ComputeLibOptions(c)) {
        return false;
      }
    }
  }
  return true;
}

bool cmVisualStudio10TargetGenerator::ComputeLibOptions(
  std::string const& config)
{
  cmComputeLinkInformation* pcli =
    this->GeneratorTarget->GetLinkInformation(config);
  if (!pcli) {
    cmSystemTools::Error(
      "CMake can not compute cmComputeLinkInformation for target: ",
      this->Name.c_str());
    return false;
  }

  cmComputeLinkInformation& cli = *pcli;
  typedef cmComputeLinkInformation::ItemVector ItemVector;
  const ItemVector& libs = cli.GetItems();
  std::string currentBinDir =
    this->LocalGenerator->GetCurrentBinaryDirectory();
  for (cmComputeLinkInformation::Item const& l : libs) {
    if (l.IsPath && cmVS10IsTargetsFile(l.Value)) {
      std::string path =
        this->LocalGenerator->ConvertToRelativePath(currentBinDir, l.Value);
      ConvertToWindowsSlash(path);
      this->AddTargetsFileAndConfigPair(path, config);
    }
  }

  return true;
}

void cmVisualStudio10TargetGenerator::WriteLinkOptions(
  Elem& e1, std::string const& config)
{
  if (this->GeneratorTarget->GetType() == cmStateEnums::STATIC_LIBRARY ||
      this->GeneratorTarget->GetType() > cmStateEnums::MODULE_LIBRARY) {
    return;
  }
  if (this->ProjectType == csproj) {
    return;
  }

  {
    Elem e2(e1, "Link");
    OptionsHelper linkOptions(*(this->LinkOptions[config]), e2);
    linkOptions.PrependInheritedString("AdditionalOptions");
    linkOptions.OutputFlagMap();
  }

  if (!this->GlobalGenerator->NeedLinkLibraryDependencies(
        this->GeneratorTarget)) {
    Elem e2(e1, "ProjectReference");
    e2.Element("LinkLibraryDependencies", "false");
  }
}

void cmVisualStudio10TargetGenerator::AddLibraries(
  const cmComputeLinkInformation& cli, std::vector<std::string>& libVec,
  std::vector<std::string>& vsTargetVec, const std::string& config)
{
  typedef cmComputeLinkInformation::ItemVector ItemVector;
  ItemVector const& libs = cli.GetItems();
  std::string currentBinDir =
    this->LocalGenerator->GetCurrentBinaryDirectory();
  for (cmComputeLinkInformation::Item const& l : libs) {
    if (l.Target) {
      auto managedType = l.Target->GetManagedType(config);
      if (managedType != cmGeneratorTarget::ManagedType::Native &&
          this->GeneratorTarget->GetManagedType(config) !=
            cmGeneratorTarget::ManagedType::Native &&
          l.Target->IsImported()) {
        auto location = l.Target->GetFullPath(config);
        if (!location.empty()) {
          ConvertToWindowsSlash(location);
          switch (this->ProjectType) {
            case csproj:
              // If the target we want to "link" to is an imported managed
              // target and this is a C# project, we add a hint reference. This
              // reference is written to project file in
              // WriteDotNetReferences().
              this->DotNetHintReferences[config].push_back(
                DotNetHintReference(l.Target->GetName(), location));
              break;
            case vcxproj:
              // Add path of assembly to list of using-directories, so the
              // managed assembly can be used by '#using <assembly.dll>' in
              // code.
              this->AdditionalUsingDirectories[config].insert(
                cmSystemTools::GetFilenamePath(location));
              break;
          }
        }
      }
      // Do not allow C# targets to be added to the LIB listing. LIB files are
      // used for linking C++ dependencies. C# libraries do not have lib files.
      // Instead, they compile down to C# reference libraries (DLL files). The
      // `<ProjectReference>` elements added to the vcxproj are enough for the
      // IDE to deduce the DLL file required by other C# projects that need its
      // reference library.
      if (managedType == cmGeneratorTarget::ManagedType::Managed) {
        continue;
      }
    }

    if (l.IsPath) {
      std::string path =
        this->LocalGenerator->ConvertToRelativePath(currentBinDir, l.Value);
      ConvertToWindowsSlash(path);
      if (cmVS10IsTargetsFile(l.Value)) {
        vsTargetVec.push_back(path);
      } else {
        libVec.push_back(path);
      }
    } else if (!l.Target ||
               l.Target->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
      libVec.push_back(l.Value);
    }
  }
}

void cmVisualStudio10TargetGenerator::AddTargetsFileAndConfigPair(
  std::string const& targetsFile, std::string const& config)
{
  for (TargetsFileAndConfigs& i : this->TargetsFileAndConfigsVec) {
    if (cmSystemTools::ComparePath(targetsFile, i.File)) {
      if (std::find(i.Configs.begin(), i.Configs.end(), config) ==
          i.Configs.end()) {
        i.Configs.push_back(config);
      }
      return;
    }
  }
  TargetsFileAndConfigs entry;
  entry.File = targetsFile;
  entry.Configs.push_back(config);
  this->TargetsFileAndConfigsVec.push_back(entry);
}

void cmVisualStudio10TargetGenerator::WriteMidlOptions(
  Elem& e1, std::string const& configName)
{
  if (!this->MSTools) {
    return;
  }
  if (this->ProjectType == csproj) {
    return;
  }

  // This processes *any* of the .idl files specified in the project's file
  // list (and passed as the item metadata %(Filename) expressing the rule
  // input filename) into output files at the per-config *build* dir
  // ($(IntDir)) each.
  //
  // IOW, this MIDL section is intended to provide a fully generic syntax
  // content suitable for most cases (read: if you get errors, then it's quite
  // probable that the error is on your side of the .idl setup).
  //
  // Also, note that the marked-as-generated _i.c file in the Visual Studio
  // generator case needs to be referred to as $(IntDir)\foo_i.c at the
  // project's file list, otherwise the compiler-side processing won't pick it
  // up (for non-directory form, it ends up looking in project binary dir
  // only).  Perhaps there's something to be done to make this more automatic
  // on the CMake side?
  std::vector<std::string> const includes =
    this->GetIncludes(configName, "MIDL");
  std::ostringstream oss;
  for (std::string const& i : includes) {
    oss << i << ";";
  }
  oss << "%(AdditionalIncludeDirectories)";

  Elem e2(e1, "Midl");
  e2.Element("AdditionalIncludeDirectories", oss.str());
  e2.Element("OutputDirectory", "$(ProjectDir)/$(IntDir)");
  e2.Element("HeaderFileName", "%(Filename).h");
  e2.Element("TypeLibraryName", "%(Filename).tlb");
  e2.Element("InterfaceIdentifierFileName", "%(Filename)_i.c");
  e2.Element("ProxyFileName", "%(Filename)_p.c");
}

void cmVisualStudio10TargetGenerator::WriteItemDefinitionGroups(Elem& e0)
{
  if (this->ProjectType == csproj) {
    return;
  }
  for (const std::string& c : this->Configurations) {
    Elem e1(e0, "ItemDefinitionGroup");
    e1.Attribute("Condition", this->CalcCondition(c));

    //    output cl compile flags <ClCompile></ClCompile>
    if (this->GeneratorTarget->GetType() <= cmStateEnums::OBJECT_LIBRARY) {
      this->WriteClOptions(e1, c);
      //    output rc compile flags <ResourceCompile></ResourceCompile>
      this->WriteRCOptions(e1, c);
      this->WriteCudaOptions(e1, c);
      this->WriteMasmOptions(e1, c);
      this->WriteNasmOptions(e1, c);
    }
    //    output midl flags       <Midl></Midl>
    this->WriteMidlOptions(e1, c);
    // write events
    if (this->ProjectType != csproj) {
      this->WriteEvents(e1, c);
    }
    //    output link flags       <Link></Link>
    this->WriteLinkOptions(e1, c);
    this->WriteCudaLinkOptions(e1, c);
    //    output lib flags       <Lib></Lib>
    this->WriteLibOptions(e1, c);
    //    output manifest flags  <Manifest></Manifest>
    this->WriteManifestOptions(e1, c);
    if (this->NsightTegra &&
        this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE &&
        this->GeneratorTarget->GetPropertyAsBool("ANDROID_GUI")) {
      this->WriteAntBuildOptions(e1, c);
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteEvents(
  Elem& e1, std::string const& configName)
{
  bool addedPrelink = false;
  cmGeneratorTarget::ModuleDefinitionInfo const* mdi =
    this->GeneratorTarget->GetModuleDefinitionInfo(configName);
  if (mdi && mdi->DefFileGenerated) {
    addedPrelink = true;
    std::vector<cmCustomCommand> commands =
      this->GeneratorTarget->GetPreLinkCommands();
    this->GlobalGenerator->AddSymbolExportCommand(this->GeneratorTarget,
                                                  commands, configName);
    this->WriteEvent(e1, "PreLinkEvent", commands, configName);
  }
  if (!addedPrelink) {
    this->WriteEvent(e1, "PreLinkEvent",
                     this->GeneratorTarget->GetPreLinkCommands(), configName);
  }
  this->WriteEvent(e1, "PreBuildEvent",
                   this->GeneratorTarget->GetPreBuildCommands(), configName);
  this->WriteEvent(e1, "PostBuildEvent",
                   this->GeneratorTarget->GetPostBuildCommands(), configName);
}

void cmVisualStudio10TargetGenerator::WriteEvent(
  Elem& e1, const char* name, std::vector<cmCustomCommand> const& commands,
  std::string const& configName)
{
  if (commands.empty()) {
    return;
  }
  cmLocalVisualStudio7Generator* lg = this->LocalGenerator;
  std::string script;
  const char* pre = "";
  std::string comment;
  for (cmCustomCommand const& cc : commands) {
    cmCustomCommandGenerator ccg(cc, configName, lg);
    if (!ccg.HasOnlyEmptyCommandLines()) {
      comment += pre;
      comment += lg->ConstructComment(ccg);
      script += pre;
      pre = "\n";
      script += lg->ConstructScript(ccg);
    }
  }
  comment = cmVS10EscapeComment(comment);
  if (this->ProjectType != csproj) {
    Elem e2(e1, name);
    e2.Element("Message", comment);
    e2.Element("Command", script);
  } else {
    std::string strippedComment = comment;
    strippedComment.erase(
      std::remove(strippedComment.begin(), strippedComment.end(), '\t'),
      strippedComment.end());
    std::ostringstream oss;
    if (!comment.empty() && !strippedComment.empty()) {
      oss << "echo " << comment << "\n";
    }
    oss << script << "\n";
    e1.Element(name, oss.str());
  }
}

void cmVisualStudio10TargetGenerator::WriteProjectReferences(Elem& e0)
{
  cmGlobalGenerator::TargetDependSet const& unordered =
    this->GlobalGenerator->GetTargetDirectDepends(this->GeneratorTarget);
  typedef cmGlobalVisualStudioGenerator::OrderedTargetDependSet
    OrderedTargetDependSet;
  OrderedTargetDependSet depends(unordered, CMAKE_CHECK_BUILD_SYSTEM_TARGET);
  Elem e1(e0, "ItemGroup");
  e1.SetHasElements();
  for (cmGeneratorTarget const* dt : depends) {
    if (dt->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    // skip fortran targets as they can not be processed by MSBuild
    // the only reference will be in the .sln file
    if (this->GlobalGenerator->TargetIsFortranOnly(dt)) {
      continue;
    }
    cmLocalGenerator* lg = dt->GetLocalGenerator();
    std::string name = dt->GetName();
    std::string path;
    const char* p = dt->GetProperty("EXTERNAL_MSPROJECT");
    if (p) {
      path = p;
    } else {
      path = lg->GetCurrentBinaryDirectory();
      path += "/";
      path += dt->GetName();
      path += computeProjectFileExtension(dt);
    }
    ConvertToWindowsSlash(path);
    Elem e2(e1, "ProjectReference");
    e2.Attribute("Include", path);
    e2.Element("Project", "{" + this->GlobalGenerator->GetGUID(name) + "}");
    e2.Element("Name", name);
    this->WriteDotNetReferenceCustomTags(e2, name);

    // If the dependency target is not managed (compiled with /clr or
    // C# target) we cannot reference it and have to set
    // 'ReferenceOutputAssembly' to false.
    auto referenceNotManaged =
      dt->GetManagedType("") < cmGeneratorTarget::ManagedType::Mixed;
    // Workaround to check for manually set /clr flags.
    if (referenceNotManaged) {
      if (const auto* flags = dt->GetProperty("COMPILE_OPTIONS")) {
        std::string flagsStr = flags;
        if (flagsStr.find("clr") != std::string::npos) {
          // There is a warning already issued when building the flags.
          referenceNotManaged = false;
        }
      }
    }
    // Workaround for static library C# targets
    if (referenceNotManaged && dt->GetType() == cmStateEnums::STATIC_LIBRARY) {
      referenceNotManaged = !dt->IsCSharpOnly();
    }
    if (referenceNotManaged) {
      e2.Element("ReferenceOutputAssembly", "false");
      e2.Element("CopyToOutputDirectory", "Never");
    }
  }
}

void cmVisualStudio10TargetGenerator::WritePlatformExtensions(Elem& e1)
{
  // This only applies to Windows 10 apps
  if (this->GlobalGenerator->TargetsWindowsStore() &&
      cmHasLiteralPrefix(this->GlobalGenerator->GetSystemVersion(), "10.0")) {
    const char* desktopExtensionsVersion =
      this->GeneratorTarget->GetProperty("VS_DESKTOP_EXTENSIONS_VERSION");
    if (desktopExtensionsVersion) {
      this->WriteSinglePlatformExtension(e1, "WindowsDesktop",
                                         desktopExtensionsVersion);
    }
    const char* mobileExtensionsVersion =
      this->GeneratorTarget->GetProperty("VS_MOBILE_EXTENSIONS_VERSION");
    if (mobileExtensionsVersion) {
      this->WriteSinglePlatformExtension(e1, "WindowsMobile",
                                         mobileExtensionsVersion);
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteSinglePlatformExtension(
  Elem& e1, std::string const& extension, std::string const& version)
{
  const std::string s = "$([Microsoft.Build.Utilities.ToolLocationHelper]"
                        "::GetPlatformExtensionSDKLocation(`" +
    extension + ", Version=" + version +
    "`, $(TargetPlatformIdentifier), $(TargetPlatformVersion), null, "
    "$(ExtensionSDKDirectoryRoot), null))"
    "\\DesignTime\\CommonConfiguration\\Neutral\\" +
    extension + ".props";

  Elem e2(e1, "Import");
  e2.Attribute("Project", s);
  e2.Attribute("Condition", "exists('" + s + "')");
}

void cmVisualStudio10TargetGenerator::WriteSDKReferences(Elem& e0)
{
  std::vector<std::string> sdkReferences;
  Elem e1(e0);
  bool hasWrittenItemGroup = false;
  if (const char* vsSDKReferences =
        this->GeneratorTarget->GetProperty("VS_SDK_REFERENCES")) {
    cmSystemTools::ExpandListArgument(vsSDKReferences, sdkReferences);
    e1.StartElement("ItemGroup");
    hasWrittenItemGroup = true;
    for (std::string const& ri : sdkReferences) {
      Elem(e1, "SDKReference").Attribute("Include", ri);
    }
  }

  // This only applies to Windows 10 apps
  if (this->GlobalGenerator->TargetsWindowsStore() &&
      cmHasLiteralPrefix(this->GlobalGenerator->GetSystemVersion(), "10.0")) {
    const char* desktopExtensionsVersion =
      this->GeneratorTarget->GetProperty("VS_DESKTOP_EXTENSIONS_VERSION");
    const char* mobileExtensionsVersion =
      this->GeneratorTarget->GetProperty("VS_MOBILE_EXTENSIONS_VERSION");
    const char* iotExtensionsVersion =
      this->GeneratorTarget->GetProperty("VS_IOT_EXTENSIONS_VERSION");

    if (desktopExtensionsVersion || mobileExtensionsVersion ||
        iotExtensionsVersion) {
      if (!hasWrittenItemGroup) {
        e1.StartElement("ItemGroup");
      }
      if (desktopExtensionsVersion) {
        this->WriteSingleSDKReference(e1, "WindowsDesktop",
                                      desktopExtensionsVersion);
      }
      if (mobileExtensionsVersion) {
        this->WriteSingleSDKReference(e1, "WindowsMobile",
                                      mobileExtensionsVersion);
      }
      if (iotExtensionsVersion) {
        this->WriteSingleSDKReference(e1, "WindowsIoT", iotExtensionsVersion);
      }
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteSingleSDKReference(
  Elem& e1, std::string const& extension, std::string const& version)
{
  Elem(e1, "SDKReference")
    .Attribute("Include", extension + ", Version=" + version);
}

void cmVisualStudio10TargetGenerator::WriteWinRTPackageCertificateKeyFile(
  Elem& e0)
{
  if ((this->GlobalGenerator->TargetsWindowsStore() ||
       this->GlobalGenerator->TargetsWindowsPhone()) &&
      (cmStateEnums::EXECUTABLE == this->GeneratorTarget->GetType())) {
    std::string pfxFile;
    std::vector<cmSourceFile const*> certificates;
    this->GeneratorTarget->GetCertificates(certificates, "");
    for (cmSourceFile const* si : certificates) {
      pfxFile = this->ConvertPath(si->GetFullPath(), false);
      ConvertToWindowsSlash(pfxFile);
      break;
    }

    if (this->IsMissingFiles &&
        !(this->GlobalGenerator->TargetsWindowsPhone() &&
          this->GlobalGenerator->GetSystemVersion() == "8.0")) {
      // Move the manifest to a project directory to avoid clashes
      std::string artifactDir =
        this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
      ConvertToWindowsSlash(artifactDir);
      Elem e1(e0, "PropertyGroup");
      e1.Element("AppxPackageArtifactsDir", artifactDir + "\\");
      std::string resourcePriFile =
        this->DefaultArtifactDir + "/resources.pri";
      ConvertToWindowsSlash(resourcePriFile);
      e1.Element("ProjectPriFullPath", resourcePriFile);

      // If we are missing files and we don't have a certificate and
      // aren't targeting WP8.0, add a default certificate
      if (pfxFile.empty()) {
        std::string templateFolder =
          cmSystemTools::GetCMakeRoot() + "/Templates/Windows";
        pfxFile = this->DefaultArtifactDir + "/Windows_TemporaryKey.pfx";
        cmSystemTools::CopyAFile(templateFolder + "/Windows_TemporaryKey.pfx",
                                 pfxFile, false);
        ConvertToWindowsSlash(pfxFile);
        this->AddedFiles.push_back(pfxFile);
        this->AddedDefaultCertificate = true;
      }

      e1.Element("PackageCertificateKeyFile", pfxFile);
      std::string thumb = cmSystemTools::ComputeCertificateThumbprint(pfxFile);
      if (!thumb.empty()) {
        e1.Element("PackageCertificateThumbprint", thumb);
      }
    } else if (!pfxFile.empty()) {
      Elem e1(e0, "PropertyGroup");
      e1.Element("PackageCertificateKeyFile", pfxFile);
      std::string thumb = cmSystemTools::ComputeCertificateThumbprint(pfxFile);
      if (!thumb.empty()) {
        e1.Element("PackageCertificateThumbprint", thumb);
      }
    }
  }
}

bool cmVisualStudio10TargetGenerator::IsResxHeader(
  const std::string& headerFile)
{
  std::set<std::string> expectedResxHeaders;
  this->GeneratorTarget->GetExpectedResxHeaders(expectedResxHeaders, "");

  std::set<std::string>::const_iterator it =
    expectedResxHeaders.find(headerFile);
  return it != expectedResxHeaders.end();
}

bool cmVisualStudio10TargetGenerator::IsXamlHeader(
  const std::string& headerFile)
{
  std::set<std::string> expectedXamlHeaders;
  this->GeneratorTarget->GetExpectedXamlHeaders(expectedXamlHeaders, "");

  std::set<std::string>::const_iterator it =
    expectedXamlHeaders.find(headerFile);
  return it != expectedXamlHeaders.end();
}

bool cmVisualStudio10TargetGenerator::IsXamlSource(
  const std::string& sourceFile)
{
  std::set<std::string> expectedXamlSources;
  this->GeneratorTarget->GetExpectedXamlSources(expectedXamlSources, "");

  std::set<std::string>::const_iterator it =
    expectedXamlSources.find(sourceFile);
  return it != expectedXamlSources.end();
}

void cmVisualStudio10TargetGenerator::WriteApplicationTypeSettings(Elem& e1)
{
  cmGlobalVisualStudio10Generator* gg = this->GlobalGenerator;
  bool isAppContainer = false;
  bool const isWindowsPhone = this->GlobalGenerator->TargetsWindowsPhone();
  bool const isWindowsStore = this->GlobalGenerator->TargetsWindowsStore();
  std::string const& v = this->GlobalGenerator->GetSystemVersion();
  if (isWindowsPhone || isWindowsStore) {
    e1.Element("ApplicationType",
               (isWindowsPhone ? "Windows Phone" : "Windows Store"));
    e1.Element("DefaultLanguage", "en-US");
    if (cmHasLiteralPrefix(v, "10.0")) {
      e1.Element("ApplicationTypeRevision", "10.0");
      // Visual Studio 14.0 is necessary for building 10.0 apps
      e1.Element("MinimumVisualStudioVersion", "14.0");

      if (this->GeneratorTarget->GetType() < cmStateEnums::UTILITY) {
        isAppContainer = true;
      }
    } else if (v == "8.1") {
      e1.Element("ApplicationTypeRevision", v);
      // Visual Studio 12.0 is necessary for building 8.1 apps
      e1.Element("MinimumVisualStudioVersion", "12.0");

      if (this->GeneratorTarget->GetType() < cmStateEnums::UTILITY) {
        isAppContainer = true;
      }
    } else if (v == "8.0") {
      e1.Element("ApplicationTypeRevision", v);
      // Visual Studio 11.0 is necessary for building 8.0 apps
      e1.Element("MinimumVisualStudioVersion", "11.0");

      if (isWindowsStore &&
          this->GeneratorTarget->GetType() < cmStateEnums::UTILITY) {
        isAppContainer = true;
      } else if (isWindowsPhone &&
                 this->GeneratorTarget->GetType() ==
                   cmStateEnums::EXECUTABLE) {
        e1.Element("XapOutputs", "true");
        e1.Element("XapFilename",
                   this->Name + "_$(Configuration)_$(Platform).xap");
      }
    }
  }
  if (isAppContainer) {
    e1.Element("AppContainerApplication", "true");
  } else if (this->Platform == "ARM64") {
    e1.Element("WindowsSDKDesktopARM64Support", "true");
  } else if (this->Platform == "ARM") {
    e1.Element("WindowsSDKDesktopARMSupport", "true");
  }
  std::string const& targetPlatformVersion =
    gg->GetWindowsTargetPlatformVersion();
  if (!targetPlatformVersion.empty()) {
    e1.Element("WindowsTargetPlatformVersion", targetPlatformVersion);
  }
  const char* targetPlatformMinVersion = this->GeneratorTarget->GetProperty(
    "VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION");
  if (targetPlatformMinVersion) {
    e1.Element("WindowsTargetPlatformMinVersion", targetPlatformMinVersion);
  } else if (isWindowsStore && cmHasLiteralPrefix(v, "10.0")) {
    // If the min version is not set, then use the TargetPlatformVersion
    if (!targetPlatformVersion.empty()) {
      e1.Element("WindowsTargetPlatformMinVersion", targetPlatformVersion);
    }
  }

  // Added IoT Startup Task support
  if (this->GeneratorTarget->GetPropertyAsBool("VS_IOT_STARTUP_TASK")) {
    e1.Element("ContainsStartupTask", "true");
  }
}

void cmVisualStudio10TargetGenerator::VerifyNecessaryFiles()
{
  // For Windows and Windows Phone executables, we will assume that if a
  // manifest is not present that we need to add all the necessary files
  if (this->GeneratorTarget->GetType() == cmStateEnums::EXECUTABLE) {
    std::vector<cmSourceFile const*> manifestSources;
    this->GeneratorTarget->GetAppManifest(manifestSources, "");
    {
      std::string const& v = this->GlobalGenerator->GetSystemVersion();
      if (this->GlobalGenerator->TargetsWindowsPhone()) {
        if (v == "8.0") {
          // Look through the sources for WMAppManifest.xml
          std::vector<cmSourceFile const*> extraSources;
          this->GeneratorTarget->GetExtraSources(extraSources, "");
          bool foundManifest = false;
          for (cmSourceFile const* si : extraSources) {
            // Need to do a lowercase comparison on the filename
            if ("wmappmanifest.xml" ==
                cmSystemTools::LowerCase(si->GetLocation().GetName())) {
              foundManifest = true;
              break;
            }
          }
          if (!foundManifest) {
            this->IsMissingFiles = true;
          }
        } else if (v == "8.1") {
          if (manifestSources.empty()) {
            this->IsMissingFiles = true;
          }
        }
      } else if (this->GlobalGenerator->TargetsWindowsStore()) {
        if (manifestSources.empty()) {
          if (v == "8.0") {
            this->IsMissingFiles = true;
          } else if (v == "8.1" || cmHasLiteralPrefix(v, "10.0")) {
            this->IsMissingFiles = true;
          }
        }
      }
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteMissingFiles(Elem& e1)
{
  std::string const& v = this->GlobalGenerator->GetSystemVersion();
  if (this->GlobalGenerator->TargetsWindowsPhone()) {
    if (v == "8.0") {
      this->WriteMissingFilesWP80(e1);
    } else if (v == "8.1") {
      this->WriteMissingFilesWP81(e1);
    }
  } else if (this->GlobalGenerator->TargetsWindowsStore()) {
    if (v == "8.0") {
      this->WriteMissingFilesWS80(e1);
    } else if (v == "8.1") {
      this->WriteMissingFilesWS81(e1);
    } else if (cmHasLiteralPrefix(v, "10.0")) {
      this->WriteMissingFilesWS10_0(e1);
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteMissingFilesWP80(Elem& e1)
{
  std::string templateFolder =
    cmSystemTools::GetCMakeRoot() + "/Templates/Windows";

  // For WP80, the manifest needs to be in the same folder as the project
  // this can cause an overwrite problem if projects aren't organized in
  // folders
  std::string manifestFile =
    this->LocalGenerator->GetCurrentBinaryDirectory() + "/WMAppManifest.xml";
  std::string artifactDir =
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  ConvertToWindowsSlash(artifactDir);
  std::string artifactDirXML = cmVS10EscapeXML(artifactDir);
  std::string targetNameXML =
    cmVS10EscapeXML(this->GeneratorTarget->GetName());

  cmGeneratedFileStream fout(manifestFile);
  fout.SetCopyIfDifferent(true);

  /* clang-format off */
  fout <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<Deployment"
    " xmlns=\"http://schemas.microsoft.com/windowsphone/2012/deployment\""
    " AppPlatformVersion=\"8.0\">\n"
    "\t<DefaultLanguage xmlns=\"\" code=\"en-US\"/>\n"
    "\t<App xmlns=\"\" ProductID=\"{" << this->GUID << "}\""
    " Title=\"CMake Test Program\" RuntimeType=\"Modern Native\""
    " Version=\"1.0.0.0\" Genre=\"apps.normal\"  Author=\"CMake\""
    " Description=\"Default CMake App\" Publisher=\"CMake\""
    " PublisherID=\"{" << this->GUID << "}\">\n"
    "\t\t<IconPath IsRelative=\"true\" IsResource=\"false\">"
       << artifactDirXML << "\\ApplicationIcon.png</IconPath>\n"
    "\t\t<Capabilities/>\n"
    "\t\t<Tasks>\n"
    "\t\t\t<DefaultTask Name=\"_default\""
    " ImagePath=\"" << targetNameXML << ".exe\" ImageParams=\"\" />\n"
    "\t\t</Tasks>\n"
    "\t\t<Tokens>\n"
    "\t\t\t<PrimaryToken TokenID=\"" << targetNameXML << "Token\""
    " TaskName=\"_default\">\n"
    "\t\t\t\t<TemplateFlip>\n"
    "\t\t\t\t\t<SmallImageURI IsRelative=\"true\" IsResource=\"false\">"
       << artifactDirXML << "\\SmallLogo.png</SmallImageURI>\n"
    "\t\t\t\t\t<Count>0</Count>\n"
    "\t\t\t\t\t<BackgroundImageURI IsRelative=\"true\" IsResource=\"false\">"
       << artifactDirXML << "\\Logo.png</BackgroundImageURI>\n"
    "\t\t\t\t</TemplateFlip>\n"
    "\t\t\t</PrimaryToken>\n"
    "\t\t</Tokens>\n"
    "\t\t<ScreenResolutions>\n"
    "\t\t\t<ScreenResolution Name=\"ID_RESOLUTION_WVGA\" />\n"
    "\t\t</ScreenResolutions>\n"
    "\t</App>\n"
    "</Deployment>\n";
  /* clang-format on */

  std::string sourceFile = this->ConvertPath(manifestFile, false);
  ConvertToWindowsSlash(sourceFile);
  {
    Elem e2(e1, "Xml");
    e2.Attribute("Include", sourceFile);
    e2.Element("SubType", "Designer");
  }
  this->AddedFiles.push_back(sourceFile);

  std::string smallLogo = this->DefaultArtifactDir + "/SmallLogo.png";
  cmSystemTools::CopyAFile(templateFolder + "/SmallLogo.png", smallLogo,
                           false);
  ConvertToWindowsSlash(smallLogo);
  Elem(e1, "Image").Attribute("Include", smallLogo);
  this->AddedFiles.push_back(smallLogo);

  std::string logo = this->DefaultArtifactDir + "/Logo.png";
  cmSystemTools::CopyAFile(templateFolder + "/Logo.png", logo, false);
  ConvertToWindowsSlash(logo);
  Elem(e1, "Image").Attribute("Include", logo);
  this->AddedFiles.push_back(logo);

  std::string applicationIcon =
    this->DefaultArtifactDir + "/ApplicationIcon.png";
  cmSystemTools::CopyAFile(templateFolder + "/ApplicationIcon.png",
                           applicationIcon, false);
  ConvertToWindowsSlash(applicationIcon);
  Elem(e1, "Image").Attribute("Include", applicationIcon);
  this->AddedFiles.push_back(applicationIcon);
}

void cmVisualStudio10TargetGenerator::WriteMissingFilesWP81(Elem& e1)
{
  std::string manifestFile =
    this->DefaultArtifactDir + "/package.appxManifest";
  std::string artifactDir =
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  ConvertToWindowsSlash(artifactDir);
  std::string artifactDirXML = cmVS10EscapeXML(artifactDir);
  std::string targetNameXML =
    cmVS10EscapeXML(this->GeneratorTarget->GetName());

  cmGeneratedFileStream fout(manifestFile);
  fout.SetCopyIfDifferent(true);

  /* clang-format off */
  fout <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<Package xmlns=\"http://schemas.microsoft.com/appx/2010/manifest\""
    " xmlns:m2=\"http://schemas.microsoft.com/appx/2013/manifest\""
    " xmlns:mp=\"http://schemas.microsoft.com/appx/2014/phone/manifest\">\n"
    "\t<Identity Name=\"" << this->GUID << "\" Publisher=\"CN=CMake\""
    " Version=\"1.0.0.0\" />\n"
    "\t<mp:PhoneIdentity PhoneProductId=\"" << this->GUID << "\""
    " PhonePublisherId=\"00000000-0000-0000-0000-000000000000\"/>\n"
    "\t<Properties>\n"
    "\t\t<DisplayName>" << targetNameXML << "</DisplayName>\n"
    "\t\t<PublisherDisplayName>CMake</PublisherDisplayName>\n"
    "\t\t<Logo>" << artifactDirXML << "\\StoreLogo.png</Logo>\n"
    "\t</Properties>\n"
    "\t<Prerequisites>\n"
    "\t\t<OSMinVersion>6.3.1</OSMinVersion>\n"
    "\t\t<OSMaxVersionTested>6.3.1</OSMaxVersionTested>\n"
    "\t</Prerequisites>\n"
    "\t<Resources>\n"
    "\t\t<Resource Language=\"x-generate\" />\n"
    "\t</Resources>\n"
    "\t<Applications>\n"
    "\t\t<Application Id=\"App\""
    " Executable=\"" << targetNameXML << ".exe\""
    " EntryPoint=\"" << targetNameXML << ".App\">\n"
    "\t\t\t<m2:VisualElements\n"
    "\t\t\t\tDisplayName=\"" << targetNameXML << "\"\n"
    "\t\t\t\tDescription=\"" << targetNameXML << "\"\n"
    "\t\t\t\tBackgroundColor=\"#336699\"\n"
    "\t\t\t\tForegroundText=\"light\"\n"
    "\t\t\t\tSquare150x150Logo=\"" << artifactDirXML << "\\Logo.png\"\n"
    "\t\t\t\tSquare30x30Logo=\"" << artifactDirXML << "\\SmallLogo.png\">\n"
    "\t\t\t\t<m2:DefaultTile ShortName=\"" << targetNameXML << "\">\n"
    "\t\t\t\t\t<m2:ShowNameOnTiles>\n"
    "\t\t\t\t\t\t<m2:ShowOn Tile=\"square150x150Logo\" />\n"
    "\t\t\t\t\t</m2:ShowNameOnTiles>\n"
    "\t\t\t\t</m2:DefaultTile>\n"
    "\t\t\t\t<m2:SplashScreen"
    " Image=\"" << artifactDirXML << "\\SplashScreen.png\" />\n"
    "\t\t\t</m2:VisualElements>\n"
    "\t\t</Application>\n"
    "\t</Applications>\n"
    "</Package>\n";
  /* clang-format on */

  this->WriteCommonMissingFiles(e1, manifestFile);
}

void cmVisualStudio10TargetGenerator::WriteMissingFilesWS80(Elem& e1)
{
  std::string manifestFile =
    this->DefaultArtifactDir + "/package.appxManifest";
  std::string artifactDir =
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  ConvertToWindowsSlash(artifactDir);
  std::string artifactDirXML = cmVS10EscapeXML(artifactDir);
  std::string targetNameXML =
    cmVS10EscapeXML(this->GeneratorTarget->GetName());

  cmGeneratedFileStream fout(manifestFile);
  fout.SetCopyIfDifferent(true);

  /* clang-format off */
  fout <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<Package xmlns=\"http://schemas.microsoft.com/appx/2010/manifest\">\n"
    "\t<Identity Name=\"" << this->GUID << "\" Publisher=\"CN=CMake\""
    " Version=\"1.0.0.0\" />\n"
    "\t<Properties>\n"
    "\t\t<DisplayName>" << targetNameXML << "</DisplayName>\n"
    "\t\t<PublisherDisplayName>CMake</PublisherDisplayName>\n"
    "\t\t<Logo>" << artifactDirXML << "\\StoreLogo.png</Logo>\n"
    "\t</Properties>\n"
    "\t<Prerequisites>\n"
    "\t\t<OSMinVersion>6.2.1</OSMinVersion>\n"
    "\t\t<OSMaxVersionTested>6.2.1</OSMaxVersionTested>\n"
    "\t</Prerequisites>\n"
    "\t<Resources>\n"
    "\t\t<Resource Language=\"x-generate\" />\n"
    "\t</Resources>\n"
    "\t<Applications>\n"
    "\t\t<Application Id=\"App\""
    " Executable=\"" << targetNameXML << ".exe\""
    " EntryPoint=\"" << targetNameXML << ".App\">\n"
    "\t\t\t<VisualElements"
    " DisplayName=\"" << targetNameXML << "\""
    " Description=\"" << targetNameXML << "\""
    " BackgroundColor=\"#336699\" ForegroundText=\"light\""
    " Logo=\"" << artifactDirXML << "\\Logo.png\""
    " SmallLogo=\"" << artifactDirXML << "\\SmallLogo.png\">\n"
    "\t\t\t\t<DefaultTile ShowName=\"allLogos\""
    " ShortName=\"" << targetNameXML << "\" />\n"
    "\t\t\t\t<SplashScreen"
    " Image=\"" << artifactDirXML << "\\SplashScreen.png\" />\n"
    "\t\t\t</VisualElements>\n"
    "\t\t</Application>\n"
    "\t</Applications>\n"
    "</Package>\n";
  /* clang-format on */

  this->WriteCommonMissingFiles(e1, manifestFile);
}

void cmVisualStudio10TargetGenerator::WriteMissingFilesWS81(Elem& e1)
{
  std::string manifestFile =
    this->DefaultArtifactDir + "/package.appxManifest";
  std::string artifactDir =
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  ConvertToWindowsSlash(artifactDir);
  std::string artifactDirXML = cmVS10EscapeXML(artifactDir);
  std::string targetNameXML =
    cmVS10EscapeXML(this->GeneratorTarget->GetName());

  cmGeneratedFileStream fout(manifestFile);
  fout.SetCopyIfDifferent(true);

  /* clang-format off */
  fout <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<Package xmlns=\"http://schemas.microsoft.com/appx/2010/manifest\""
    " xmlns:m2=\"http://schemas.microsoft.com/appx/2013/manifest\">\n"
    "\t<Identity Name=\"" << this->GUID << "\" Publisher=\"CN=CMake\""
    " Version=\"1.0.0.0\" />\n"
    "\t<Properties>\n"
    "\t\t<DisplayName>" << targetNameXML << "</DisplayName>\n"
    "\t\t<PublisherDisplayName>CMake</PublisherDisplayName>\n"
    "\t\t<Logo>" << artifactDirXML << "\\StoreLogo.png</Logo>\n"
    "\t</Properties>\n"
    "\t<Prerequisites>\n"
    "\t\t<OSMinVersion>6.3</OSMinVersion>\n"
    "\t\t<OSMaxVersionTested>6.3</OSMaxVersionTested>\n"
    "\t</Prerequisites>\n"
    "\t<Resources>\n"
    "\t\t<Resource Language=\"x-generate\" />\n"
    "\t</Resources>\n"
    "\t<Applications>\n"
    "\t\t<Application Id=\"App\""
    " Executable=\"" << targetNameXML << ".exe\""
    " EntryPoint=\"" << targetNameXML << ".App\">\n"
    "\t\t\t<m2:VisualElements\n"
    "\t\t\t\tDisplayName=\"" << targetNameXML << "\"\n"
    "\t\t\t\tDescription=\"" << targetNameXML << "\"\n"
    "\t\t\t\tBackgroundColor=\"#336699\"\n"
    "\t\t\t\tForegroundText=\"light\"\n"
    "\t\t\t\tSquare150x150Logo=\"" << artifactDirXML << "\\Logo.png\"\n"
    "\t\t\t\tSquare30x30Logo=\"" << artifactDirXML << "\\SmallLogo.png\">\n"
    "\t\t\t\t<m2:DefaultTile ShortName=\"" << targetNameXML << "\">\n"
    "\t\t\t\t\t<m2:ShowNameOnTiles>\n"
    "\t\t\t\t\t\t<m2:ShowOn Tile=\"square150x150Logo\" />\n"
    "\t\t\t\t\t</m2:ShowNameOnTiles>\n"
    "\t\t\t\t</m2:DefaultTile>\n"
    "\t\t\t\t<m2:SplashScreen"
    " Image=\"" << artifactDirXML << "\\SplashScreen.png\" />\n"
    "\t\t\t</m2:VisualElements>\n"
    "\t\t</Application>\n"
    "\t</Applications>\n"
    "</Package>\n";
  /* clang-format on */

  this->WriteCommonMissingFiles(e1, manifestFile);
}

void cmVisualStudio10TargetGenerator::WriteMissingFilesWS10_0(Elem& e1)
{
  std::string manifestFile =
    this->DefaultArtifactDir + "/package.appxManifest";
  std::string artifactDir =
    this->LocalGenerator->GetTargetDirectory(this->GeneratorTarget);
  ConvertToWindowsSlash(artifactDir);
  std::string artifactDirXML = cmVS10EscapeXML(artifactDir);
  std::string targetNameXML =
    cmVS10EscapeXML(this->GeneratorTarget->GetName());

  cmGeneratedFileStream fout(manifestFile);
  fout.SetCopyIfDifferent(true);

  /* clang-format off */
  fout <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<Package\n\t"
    "xmlns=\"http://schemas.microsoft.com/appx/manifest/foundation/windows10\""
    "\txmlns:mp=\"http://schemas.microsoft.com/appx/2014/phone/manifest\"\n"
    "\txmlns:uap=\"http://schemas.microsoft.com/appx/manifest/uap/windows10\""
    "\n\tIgnorableNamespaces=\"uap mp\">\n\n"
    "\t<Identity Name=\"" << this->GUID << "\" Publisher=\"CN=CMake\""
    " Version=\"1.0.0.0\" />\n"
    "\t<mp:PhoneIdentity PhoneProductId=\"" << this->GUID <<
    "\" PhonePublisherId=\"00000000-0000-0000-0000-000000000000\"/>\n"
    "\t<Properties>\n"
    "\t\t<DisplayName>" << targetNameXML << "</DisplayName>\n"
    "\t\t<PublisherDisplayName>CMake</PublisherDisplayName>\n"
    "\t\t<Logo>" << artifactDirXML << "\\StoreLogo.png</Logo>\n"
    "\t</Properties>\n"
    "\t<Dependencies>\n"
    "\t\t<TargetDeviceFamily Name=\"Windows.Universal\" "
    "MinVersion=\"10.0.0.0\" MaxVersionTested=\"10.0.0.0\" />\n"
    "\t</Dependencies>\n"

    "\t<Resources>\n"
    "\t\t<Resource Language=\"x-generate\" />\n"
    "\t</Resources>\n"
    "\t<Applications>\n"
    "\t\t<Application Id=\"App\""
    " Executable=\"" << targetNameXML << ".exe\""
    " EntryPoint=\"" << targetNameXML << ".App\">\n"
    "\t\t\t<uap:VisualElements\n"
    "\t\t\t\tDisplayName=\"" << targetNameXML << "\"\n"
    "\t\t\t\tDescription=\"" << targetNameXML << "\"\n"
    "\t\t\t\tBackgroundColor=\"#336699\"\n"
    "\t\t\t\tSquare150x150Logo=\"" << artifactDirXML << "\\Logo.png\"\n"
    "\t\t\t\tSquare44x44Logo=\"" << artifactDirXML <<
    "\\SmallLogo44x44.png\">\n"
    "\t\t\t\t<uap:SplashScreen"
    " Image=\"" << artifactDirXML << "\\SplashScreen.png\" />\n"
    "\t\t\t</uap:VisualElements>\n"
    "\t\t</Application>\n"
    "\t</Applications>\n"
    "</Package>\n";
  /* clang-format on */

  this->WriteCommonMissingFiles(e1, manifestFile);
}

void cmVisualStudio10TargetGenerator::WriteCommonMissingFiles(
  Elem& e1, const std::string& manifestFile)
{
  std::string templateFolder =
    cmSystemTools::GetCMakeRoot() + "/Templates/Windows";

  std::string sourceFile = this->ConvertPath(manifestFile, false);
  ConvertToWindowsSlash(sourceFile);
  {
    Elem e2(e1, "AppxManifest");
    e2.Attribute("Include", sourceFile);
    e2.Element("SubType", "Designer");
  }
  this->AddedFiles.push_back(sourceFile);

  std::string smallLogo = this->DefaultArtifactDir + "/SmallLogo.png";
  cmSystemTools::CopyAFile(templateFolder + "/SmallLogo.png", smallLogo,
                           false);
  ConvertToWindowsSlash(smallLogo);
  Elem(e1, "Image").Attribute("Include", smallLogo);
  this->AddedFiles.push_back(smallLogo);

  std::string smallLogo44 = this->DefaultArtifactDir + "/SmallLogo44x44.png";
  cmSystemTools::CopyAFile(templateFolder + "/SmallLogo44x44.png", smallLogo44,
                           false);
  ConvertToWindowsSlash(smallLogo44);
  Elem(e1, "Image").Attribute("Include", smallLogo44);
  this->AddedFiles.push_back(smallLogo44);

  std::string logo = this->DefaultArtifactDir + "/Logo.png";
  cmSystemTools::CopyAFile(templateFolder + "/Logo.png", logo, false);
  ConvertToWindowsSlash(logo);
  Elem(e1, "Image").Attribute("Include", logo);
  this->AddedFiles.push_back(logo);

  std::string storeLogo = this->DefaultArtifactDir + "/StoreLogo.png";
  cmSystemTools::CopyAFile(templateFolder + "/StoreLogo.png", storeLogo,
                           false);
  ConvertToWindowsSlash(storeLogo);
  Elem(e1, "Image").Attribute("Include", storeLogo);
  this->AddedFiles.push_back(storeLogo);

  std::string splashScreen = this->DefaultArtifactDir + "/SplashScreen.png";
  cmSystemTools::CopyAFile(templateFolder + "/SplashScreen.png", splashScreen,
                           false);
  ConvertToWindowsSlash(splashScreen);
  Elem(e1, "Image").Attribute("Include", splashScreen);
  this->AddedFiles.push_back(splashScreen);

  if (this->AddedDefaultCertificate) {
    // This file has already been added to the build so don't copy it
    std::string keyFile =
      this->DefaultArtifactDir + "/Windows_TemporaryKey.pfx";
    ConvertToWindowsSlash(keyFile);
    Elem(e1, "None").Attribute("Include", keyFile);
  }
}

bool cmVisualStudio10TargetGenerator::ForceOld(const std::string& source) const
{
  HANDLE h =
    CreateFileW(cmSystemTools::ConvertToWindowsExtendedPath(source).c_str(),
                FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, 0, OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS, 0);
  if (!h) {
    return false;
  }

  FILETIME const ftime_20010101 = { 3365781504u, 29389701u };
  if (!SetFileTime(h, &ftime_20010101, &ftime_20010101, &ftime_20010101)) {
    CloseHandle(h);
    return false;
  }

  CloseHandle(h);
  return true;
}

void cmVisualStudio10TargetGenerator::GetCSharpSourceProperties(
  cmSourceFile const* sf, std::map<std::string, std::string>& tags)
{
  if (this->ProjectType == csproj) {
    const cmPropertyMap& props = sf->GetProperties();
    for (auto const& p : props) {
      static const std::string propNamePrefix = "VS_CSHARP_";
      if (p.first.find(propNamePrefix) == 0) {
        std::string tagName = p.first.substr(propNamePrefix.length());
        if (!tagName.empty()) {
          const std::string val = props.GetPropertyValue(p.first);
          if (!val.empty()) {
            tags[tagName] = val;
          } else {
            tags.erase(tagName);
          }
        }
      }
    }
  }
}

void cmVisualStudio10TargetGenerator::WriteCSharpSourceProperties(
  Elem& e2, const std::map<std::string, std::string>& tags)
{
  if (!tags.empty()) {
    for (const auto& i : tags) {
      e2.Element(i.first.c_str(), i.second);
    }
  }
}

void cmVisualStudio10TargetGenerator::GetCSharpSourceLink(
  cmSourceFile const* sf, std::string& link)
{
  std::string const& sourceFilePath = sf->GetFullPath();
  std::string const& binaryDir = LocalGenerator->GetCurrentBinaryDirectory();

  if (!cmSystemTools::IsSubDirectory(sourceFilePath, binaryDir)) {
    const std::string& stripFromPath =
      this->Makefile->GetCurrentSourceDirectory();
    if (sourceFilePath.find(stripFromPath) == 0) {
      if (const char* l = sf->GetProperty("VS_CSHARP_Link")) {
        link = l;
      } else {
        link = sourceFilePath.substr(stripFromPath.length() + 1);
      }
      ConvertToWindowsSlash(link);
    }
  }
}

std::string cmVisualStudio10TargetGenerator::GetCMakeFilePath(
  const char* relativeFilePath) const
{
  // Always search in the standard modules location.
  std::string path = cmSystemTools::GetCMakeRoot() + "/";
  path += relativeFilePath;
  ConvertToWindowsSlash(path);

  return path;
}
