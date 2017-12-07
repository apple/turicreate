static cmVS7FlagTable cmVS12CSharpFlagTable[] = {
  { "ProjectName", "out:", "", "", cmIDEFlagTable::UserValueRequired },

  { "OutputType", "target:exe", "", "Exe", 0 },
  { "OutputType", "target:winexe", "", "Winexe", 0 },
  { "OutputType", "target:library", "", "Library", 0 },
  { "OutputType", "target:module", "", "Module", 0 },

  { "DocumentationFile", "doc", "", "", cmIDEFlagTable::UserValueRequired },

  { "Platform", "platform:x86", "", "x86", 0 },
  { "Platform", "platform:Itanium", "", "Itanium", 0 },
  { "Platform", "platform:x64", "", "x64", 0 },
  { "Platform", "platform:arm", "", "arm", 0 },
  { "Platform", "platform:anycpu32bitpreferred", "", "anycpu32bitpreferred",
    0 },
  { "Platform", "platform:anycpu", "", "anycpu", 0 },

  { "References", "reference:", "mit alias", "", 0 },
  { "References", "reference:", "dateiliste", "", 0 },
  { "AddModules", "addmodule:", "", "", cmIDEFlagTable::SemicolonAppendable },
  { "", "link", "", "", 0 },

  { "Win32Resource", "win32res", "", "", cmIDEFlagTable::UserValueRequired },
  { "ApplicationIcon", "win32icon", "", "",
    cmIDEFlagTable::UserValueRequired },

  { "Win32Manifest", "win32manifest:", "", "true", 0 },

  { "NoWin32Manifest", "nowin32manifest", "", "true", 0 },

  { "DefineDebug", "debug", "", "true", cmIDEFlagTable::Continue },

  { "DebugSymbols", "debug", "", "true", 0 },
  { "DebugSymbols", "debug-", "", "false", 0 },
  { "DebugSymbols", "debug+", "", "true", 0 },

  { "DebugType", "debug:none", "", "none", 0 },
  { "DebugType", "debug:full", "", "full", 0 },
  { "DebugType", "debug:pdbonly", "", "pdbonly", 0 },

  { "Optimize", "optimize", "", "true", 0 },
  { "Optimize", "optimize-", "", "false", 0 },
  { "Optimize", "optimize+", "", "true", 0 },

  { "TreatWarningsAsErrors", "warnaserror", "", "true", 0 },
  { "TreatWarningsAsErrors", "warnaserror-", "", "false", 0 },
  { "TreatWarningsAsErrors", "warnaserror+", "", "true", 0 },

  { "WarningsAsErrors", "warnaserror", "", "", 0 },
  { "WarningsAsErrors", "warnaserror-", "", "", 0 },
  { "WarningsAsErrors", "warnaserror+", "", "", 0 },

  { "WarningLevel", "warn:0", "", "0", 0 },
  { "WarningLevel", "warn:1", "", "1", 0 },
  { "WarningLevel", "warn:2", "", "2", 0 },
  { "WarningLevel", "warn:3", "", "3", 0 },
  { "WarningLevel", "warn:4", "", "4", 0 },
  { "DisabledWarnings", "nowarn", "", "", 0 },

  { "CheckForOverflowUnderflow", "checked", "", "true", 0 },
  { "CheckForOverflowUnderflow", "checked-", "", "false", 0 },
  { "CheckForOverflowUnderflow", "checked+", "", "true", 0 },

  { "AllowUnsafeBlocks", "unsafe", "", "true", 0 },
  { "AllowUnsafeBlocks", "unsafe-", "", "false", 0 },
  { "AllowUnsafeBlocks", "unsafe+", "", "true", 0 },

  { "DefineConstants", "define:", "", "",
    cmIDEFlagTable::SemicolonAppendable | cmIDEFlagTable::UserValue },

  { "LangVersion", "langversion:ISO-1", "", "ISO-1", 0 },
  { "LangVersion", "langversion:ISO-2", "", "ISO-2", 0 },
  { "LangVersion", "langversion:3", "", "3", 0 },
  { "LangVersion", "langversion:4", "", "4", 0 },
  { "LangVersion", "langversion:5", "", "5", 0 },
  { "LangVersion", "langversion:6", "", "6", 0 },
  { "LangVersion", "langversion:default", "", "default", 0 },

  { "DelaySign", "delaysign", "", "true", 0 },
  { "DelaySign", "delaysign-", "", "false", 0 },
  { "DelaySign", "delaysign+", "", "true", 0 },

  { "AssemblyOriginatorKeyFile", "keyfile", "", "", 0 },

  { "KeyContainerName", "keycontainer", "", "", 0 },

  { "NoLogo", "nologo", "", "", 0 },

  { "NoConfig", "noconfig", "", "true", 0 },

  { "BaseAddress", "baseaddress:", "", "", 0 },

  { "CodePage", "codepage", "", "", 0 },

  { "Utf8Output", "utf8output", "", "", 0 },

  { "MainEntryPoint", "main:", "", "", 0 },

  { "GenerateFullPaths", "fullpaths", "", "true", 0 },

  { "FileAlignment", "filealign", "", "", 0 },

  { "PdbFile", "pdb:", "", "", 0 },

  { "NoStandardLib", "nostdlib", "", "true", 0 },
  { "NoStandardLib", "nostdlib-", "", "false", 0 },
  { "NoStandardLib", "nostdlib+", "", "true", 0 },

  { "SubsystemVersion", "subsystemversion", "", "", 0 },

  { "AdditionalLibPaths", "lib:", "", "", 0 },

  { "ErrorReport", "errorreport:none", "Do Not Send Report", "none", 0 },
  { "ErrorReport", "errorreport:prompt", "Prompt Immediately", "prompt", 0 },
  { "ErrorReport", "errorreport:queue", "Queue For Next Login", "queue", 0 },
  { "ErrorReport", "errorreport:send", "Send Automatically", "send", 0 },

  { 0, 0, 0, 0, 0 },
};
