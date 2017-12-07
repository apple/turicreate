static cmVS7FlagTable cmVS11LibFlagTable[] = {

  // Enum Properties
  { "ErrorReporting", "ERRORREPORT:PROMPT", "PromptImmediately",
    "PromptImmediately", 0 },
  { "ErrorReporting", "ERRORREPORT:QUEUE", "Queue For Next Login",
    "QueueForNextLogin", 0 },
  { "ErrorReporting", "ERRORREPORT:SEND", "Send Error Report",
    "SendErrorReport", 0 },
  { "ErrorReporting", "ERRORREPORT:NONE", "No Error Report", "NoErrorReport",
    0 },

  { "TargetMachine", "MACHINE:ARM", "MachineARM", "MachineARM", 0 },
  { "TargetMachine", "MACHINE:EBC", "MachineEBC", "MachineEBC", 0 },
  { "TargetMachine", "MACHINE:IA64", "MachineIA64", "MachineIA64", 0 },
  { "TargetMachine", "MACHINE:MIPS", "MachineMIPS", "MachineMIPS", 0 },
  { "TargetMachine", "MACHINE:MIPS16", "MachineMIPS16", "MachineMIPS16", 0 },
  { "TargetMachine", "MACHINE:MIPSFPU", "MachineMIPSFPU", "MachineMIPSFPU",
    0 },
  { "TargetMachine", "MACHINE:MIPSFPU16", "MachineMIPSFPU16",
    "MachineMIPSFPU16", 0 },
  { "TargetMachine", "MACHINE:SH4", "MachineSH4", "MachineSH4", 0 },
  { "TargetMachine", "MACHINE:THUMB", "MachineTHUMB", "MachineTHUMB", 0 },
  { "TargetMachine", "MACHINE:X64", "MachineX64", "MachineX64", 0 },
  { "TargetMachine", "MACHINE:X86", "MachineX86", "MachineX86", 0 },

  { "SubSystem", "SUBSYSTEM:CONSOLE", "Console", "Console", 0 },
  { "SubSystem", "SUBSYSTEM:WINDOWS", "Windows", "Windows", 0 },
  { "SubSystem", "SUBSYSTEM:NATIVE", "Native", "Native", 0 },
  { "SubSystem", "SUBSYSTEM:EFI_APPLICATION", "EFI Application",
    "EFI Application", 0 },
  { "SubSystem", "SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER",
    "EFI Boot Service Driver", "EFI Boot Service Driver", 0 },
  { "SubSystem", "SUBSYSTEM:EFI_ROM", "EFI ROM", "EFI ROM", 0 },
  { "SubSystem", "SUBSYSTEM:EFI_RUNTIME_DRIVER", "EFI Runtime", "EFI Runtime",
    0 },
  { "SubSystem", "SUBSYSTEM:WINDOWSCE", "WindowsCE", "WindowsCE", 0 },
  { "SubSystem", "SUBSYSTEM:POSIX", "POSIX", "POSIX", 0 },

  // Bool Properties
  { "SuppressStartupBanner", "NOLOGO", "", "true", 0 },
  { "IgnoreAllDefaultLibraries", "NODEFAULTLIB", "", "true", 0 },
  { "TreatLibWarningAsErrors", "WX:NO", "", "false", 0 },
  { "TreatLibWarningAsErrors", "WX", "", "true", 0 },
  { "Verbose", "VERBOSE", "", "true", 0 },
  { "LinkTimeCodeGeneration", "LTCG", "", "true", 0 },

  // Bool Properties With Argument

  // String List Properties
  // Skip [AdditionalDependencies] - no command line Switch.
  { "AdditionalLibraryDirectories", "LIBPATH:",
    "Additional Library Directories", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "IgnoreSpecificDefaultLibraries", "NODEFAULTLIB:",
    "Ignore Specific Default Libraries", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "ExportNamedFunctions", "EXPORT:", "Export Named Functions", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "RemoveObjects", "REMOVE:", "Remove Objects", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },

  // String Properties
  { "OutputFile", "OUT:", "Output File", "", cmVS7FlagTable::UserValue },
  { "ModuleDefinitionFile", "DEF:", "Module Definition File Name", "",
    cmVS7FlagTable::UserValue },
  { "ForceSymbolReferences", "INCLUDE:", "Force Symbol References", "",
    cmVS7FlagTable::UserValue },
  { "DisplayLibrary", "LIST:", "Display Library to standard output", "",
    cmVS7FlagTable::UserValue },
  // Skip [MinimumRequiredVersion] - no command line Switch.
  { "Name", "NAME:", "Name", "", cmVS7FlagTable::UserValue },
  // Skip [AdditionalOptions] - no command line Switch.
  // Skip [TrackerLogDirectory] - no command line Switch.
  { 0, 0, 0, 0, 0 }
};
