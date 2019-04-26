static cmVS7FlagTable cmVS10NASMFlagTable[] = {

  // Enum Properties
  { "Outputswitch", "fwin32", "", "0", 0 },
  { "Outputswitch", "fwin", "", "0", 0 },
  { "Outputswitch", "fwin64", "", "1", 0 },
  { "Outputswitch", "felf", "", "2", 0 },
  { "Outputswitch", "felf32", "", "2", 0 },
  { "Outputswitch", "felf64", "", "3", 0 },

  { "ErrorReportingFormat", "Xgnu", "", "-Xgnu	GNU format: Default format",
    0 },
  { "ErrorReportingFormat", "Xvc", "",
    "-Xvc	Style used by Microsoft Visual C++", 0 },

  // Bool Properties
  { "TreatWarningsAsErrors", "Werror", "", "true", 0 },
  { "GenerateDebugInformation", "g", "", "true", 0 },
  { "floatunderflow", "w+float-underflow", "", "true", 0 },
  { "macrodefaults", "w-macro-defaults", "", "true", 0 },
  { "user", "w-user", "%warning directives (default on)", "true", 0 },
  { "floatoverflow", "w-float-overflow", "", "true", 0 },
  { "floatdenorm", "w+float-denorm", "", "true", 0 },
  { "numberoverflow", "w-number-overflow", "", "true", 0 },
  { "macroselfref", "w+macro-selfref", "", "true", 0 },
  { "floattoolong", "w-float-toolong", "", "true", 0 },
  { "orphanlabels", "w-orphan-labels", "", "true", 0 },
  { "tasmmode", "t", "", "true", 0 },

  // Bool Properties With Argument

  // String List Properties
  { "PreprocessorDefinitions", "D", "Preprocessor Definitions", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "UndefinePreprocessorDefinitions", "U",
    "Undefine Preprocessor Definitions", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "IncludePaths", "I", "Include Paths", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "AssembledCodeListingFile", "l",
    "Generates an assembled code listing file.", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },

  // String Properties
  // Skip [Inputs] - no command line Switch.
  // Skip [CommandLineTemplate] - no command line Switch.
  // Skip [ExecutionDescription] - no command line Switch.
  // Skip [AdditionalOptions] - no command line Switch.
  { 0, 0, 0, 0, 0 }
};
