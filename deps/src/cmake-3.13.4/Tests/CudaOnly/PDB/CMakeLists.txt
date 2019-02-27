cmake_minimum_required(VERSION 3.11)
project (PDB CUDA)

add_executable(CudaOnlyPDB main.cu)
set_target_properties(CudaOnlyPDB PROPERTIES
  PDB_NAME LinkPDBName
  PDB_OUTPUT_DIRECTORY LinkPDBDir
  COMPILE_PDB_NAME CompPDBName
  COMPILE_PDB_OUTPUT_DIRECTORY CompPDBDir
  )

set(pdbs
  ${CMAKE_CURRENT_BINARY_DIR}/CompPDBDir/${CMAKE_CFG_INTDIR}/CompPDBName.pdb
  ${CMAKE_CURRENT_BINARY_DIR}/LinkPDBDir/${CMAKE_CFG_INTDIR}/LinkPDBName.pdb
  )
add_custom_command(TARGET CudaOnlyPDB POST_BUILD
  COMMAND ${CMAKE_COMMAND} -Dconfig=$<CONFIG> "-Dpdbs=${pdbs}"
                           -P ${CMAKE_CURRENT_SOURCE_DIR}/check_pdbs.cmake
  )
