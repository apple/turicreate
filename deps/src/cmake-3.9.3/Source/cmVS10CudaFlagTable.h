static cmVS7FlagTable cmVS10CudaFlagTable[] = {
  // Collect options meant for the host compiler.
  { "AdditionalCompilerOptions", "Xcompiler=", "Host compiler options", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SpaceAppendable },
  { "AdditionalCompilerOptions", "Xcompiler", "Host compiler options", "",
    cmVS7FlagTable::UserFollowing | cmVS7FlagTable::SpaceAppendable },

  // Select the CUDA runtime library.
  { "CudaRuntime", "cudart=none", "No CUDA runtime library", "None", 0 },
  { "CudaRuntime", "cudart=shared", "Shared/dynamic CUDA runtime library",
    "Shared", 0 },
  { "CudaRuntime", "cudart=static", "Static CUDA runtime library", "Static",
    0 },
  { "CudaRuntime", "cudart", "CUDA runtime library", "",
    cmVS7FlagTable::UserFollowing },

  // Capture arch/code arguments into temporaries for post-processing.
  { "cmake-temp-gencode", "gencode=", "", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "cmake-temp-gencode", "gencode", "", "",
    cmVS7FlagTable::UserFollowing | cmVS7FlagTable::SemicolonAppendable },
  { "cmake-temp-gencode", "-generate-code=", "", "",
    cmVS7FlagTable::UserValue | cmVS7FlagTable::SemicolonAppendable },
  { "cmake-temp-gencode", "-generate-code", "", "",
    cmVS7FlagTable::UserFollowing | cmVS7FlagTable::SemicolonAppendable },

  { "cmake-temp-code", "code=", "", "", cmVS7FlagTable::UserValue },
  { "cmake-temp-code", "code", "", "", cmVS7FlagTable::UserFollowing },
  { "cmake-temp-code", "-gpu-code=", "", "", cmVS7FlagTable::UserValue },
  { "cmake-temp-code", "-gpu-code", "", "", cmVS7FlagTable::UserFollowing },

  { "cmake-temp-arch", "arch=", "", "", cmVS7FlagTable::UserValue },
  { "cmake-temp-arch", "arch", "", "", cmVS7FlagTable::UserFollowing },
  { "cmake-temp-arch", "-gpu-architecture=", "", "",
    cmVS7FlagTable::UserValue },
  { "cmake-temp-arch", "-gpu-architecture", "", "",
    cmVS7FlagTable::UserFollowing },

  // Other flags.

  { "FastMath", "use_fast_math", "", "true", 0 },
  { "FastMath", "-use_fast_math", "", "true", 0 },

  { "GPUDebugInfo", "G", "", "true", 0 },
  { "GPUDebugInfo", "-device-debug", "", "true", 0 },

  { "HostDebugInfo", "g", "", "true", 0 },
  { "HostDebugInfo", "-debug", "", "true", 0 },

  { 0, 0, 0, 0, 0 }
};
