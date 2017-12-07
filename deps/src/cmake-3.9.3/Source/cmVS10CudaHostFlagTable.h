static cmVS7FlagTable cmVS10CudaHostFlagTable[] = {
  //{"Optimization", "", "<inherit from host>", "InheritFromHost", 0},
  { "Optimization", "Od", "Disabled", "Od", 0 },
  { "Optimization", "O1", "Minimize Size", "O1", 0 },
  { "Optimization", "O2", "Maximize Speed", "O2", 0 },
  { "Optimization", "Ox", "Full Optimization", "O3", 0 },

  //{"Runtime", "", "<inherit from host>", "InheritFromHost", 0},
  { "Runtime", "MT", "Multi-Threaded", "MT", 0 },
  { "Runtime", "MTd", "Multi-Threaded Debug", "MTd", 0 },
  { "Runtime", "MD", "Multi-Threaded DLL", "MD", 0 },
  { "Runtime", "MDd", "Multi-threaded Debug DLL", "MDd", 0 },
  { "Runtime", "ML", "Single-Threaded", "ML", 0 },
  { "Runtime", "MLd", "Single-Threaded Debug", "MLd", 0 },

  //{"RuntimeChecks", "", "<inherit from host>", "InheritFromHost", 0},
  //{"RuntimeChecks", "", "Default", "Default", 0},
  { "RuntimeChecks", "RTCs", "Stack Frames", "RTCs", 0 },
  { "RuntimeChecks", "RTCu", "Uninitialized Variables", "RTCu", 0 },
  { "RuntimeChecks", "RTC1", "Both", "RTC1", 0 },

  //{"TypeInfo", "", "<inherit from host>", "InheritFromHost", 0},
  { "TypeInfo", "GR", "Yes", "true", 0 },
  { "TypeInfo", "GR-", "No", "false", 0 },

  //{"Warning", "", "<inherit from host>", "InheritFromHost", 0},
  { "Warning", "W0", "Off: Turn Off All Warnings", "W0", 0 },
  { "Warning", "W1", "Level 1", "W1", 0 },
  { "Warning", "W2", "Level 2", "W2", 0 },
  { "Warning", "W3", "Level 3", "W3", 0 },
  { "Warning", "W4", "Level 4", "W4", 0 },
  { "Warning", "Wall", "Enable All Warnings", "Wall", 0 },

  { 0, 0, 0, 0, 0 }
};
