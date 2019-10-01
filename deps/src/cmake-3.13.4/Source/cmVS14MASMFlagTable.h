static cmVS7FlagTable cmVS14MASMFlagTable[] = {

  // Enum Properties
  { "PreserveIdentifierCase", "", "Default", "0", 0 },
  { "PreserveIdentifierCase", "Cp", "Preserves Identifier Case (/Cp)", "1",
    0 },
  { "PreserveIdentifierCase", "Cu",
    "Maps all identifiers to upper case. (/Cu)", "2", 0 },
  { "PreserveIdentifierCase", "Cx",
    "Preserves case in public and extern symbols. (/Cx)", "3", 0 },

  { "WarningLevel", "W0", "Warning Level 0 (/W0)", "0", 0 },
  { "WarningLevel", "W1", "Warning Level 1 (/W1)", "1", 0 },
  { "WarningLevel", "W2", "Warning Level 2 (/W2)", "2", 0 },
  { "WarningLevel", "W3", "Warning Level 3 (/W3)", "3", 0 },

  { "PackAlignmentBoundary", "", "Default", "0", 0 },
  { "PackAlignmentBoundary", "Zp1", "One Byte Boundary (/Zp1)", "1", 0 },
  { "PackAlignmentBoundary", "Zp2", "Two Byte Boundary (/Zp2)", "2", 0 },
  { "PackAlignmentBoundary", "Zp4", "Four Byte Boundary (/Zp4)", "3", 0 },
  { "PackAlignmentBoundary", "Zp8", "Eight Byte Boundary (/Zp8)", "4", 0 },
  { "PackAlignmentBoundary", "Zp16", "Sixteen Byte Boundary (/Zp16)", "5", 0 },

  { "CallingConvention", "", "Default", "0", 0 },
  { "CallingConvention", "Gd", "Use C-style Calling Convention (/Gd)", "1",
    0 },
  { "CallingConvention", "Gz", "Use stdcall Calling Convention (/Gz)", "2",
    0 },
  { "CallingConvention", "Gc", "Use Pascal Calling Convention (/Gc)", "3", 0 },

  { "ErrorReporting", "errorReport:prompt",
    "Prompt to send report immediately (/errorReport:prompt)", "0", 0 },
  { "ErrorReporting", "errorReport:queue",
    "Prompt to send report at the next logon (/errorReport:queue)", "1", 0 },
  { "ErrorReporting", "errorReport:send",
    "Automatically send report (/errorReport:send)", "2", 0 },
  { "ErrorReporting", "errorReport:none",
    "Do not send report (/errorReport:none)", "3", 0 },

  // Bool Properties
  { "NoLogo", "nologo", "", "true", 0 },
  { "GeneratePreprocessedSourceListing", "EP", "", "true", 0 },
  { "ListAllAvailableInformation", "Sa", "", "true", 0 },
  { "UseSafeExceptionHandlers", "safeseh", "", "true", 0 },
  { "AddFirstPassListing", "Sf", "", "true", 0 },
  { "EnableAssemblyGeneratedCodeListing", "Sg", "", "true", 0 },
  { "DisableSymbolTable", "Sn", "", "true", 0 },
  { "EnableFalseConditionalsInListing", "Sx", "", "true", 0 },
  { "TreatWarningsAsErrors", "WX", "", "true", 0 },
  { "MakeAllSymbolsPublic", "Zf", "", "true", 0 },
  { "GenerateDebugInformation", "Zi", "", "true", 0 },
  { "EnableMASM51Compatibility", "Zm", "", "true", 0 },
  { "PerformSyntaxCheckOnly", "Zs", "", "true", 0 },

  // Bool Properties With Argument

  // String List Properties
  { "PreprocessorDefinitions", "D", "Preprocessor Definitions", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "IncludePaths", "I", "Include Paths", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "BrowseFile", "FR", "Generate Browse Information File", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  // Skip [AdditionalDependencies] - no command line Switch.

  // String Properties
  // Skip [Inputs] - no command line Switch.
  { "ObjectFileName", "Fo", "Object File Name", "",
    cmVS7FlagTable::UserValue },
  { "AssembledCodeListingFile", "Fl", "Assembled Code Listing File", "",
    cmVS7FlagTable::UserValue },
  // Skip [CommandLineTemplate] - no command line Switch.
  // Skip [ExecutionDescription] - no command line Switch.
  // Skip [AdditionalOptions] - no command line Switch.
  { 0, 0, 0, 0, 0 }
};
