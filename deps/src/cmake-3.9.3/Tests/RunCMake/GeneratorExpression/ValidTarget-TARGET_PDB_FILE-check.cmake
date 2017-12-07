file(STRINGS ${RunCMake_TEST_BINARY_DIR}/test.txt TEST_TXT ENCODING UTF-8)

list(GET TEST_TXT 0 PDB_PATH)
list(GET TEST_TXT 1 PDB_NAME)
list(GET TEST_TXT 2 PDB_DIR)

if(NOT PDB_PATH MATCHES "empty\\.pdb")
  message(FATAL_ERROR "unexpected PDB_PATH [${PDB_PATH}]")
endif()

if(NOT PDB_NAME STREQUAL "empty.pdb")
  message(FATAL_ERROR "unexpected PDB_NAME [${PDB_NAME}]")
endif()

if(PDB_DIR MATCHES "empty\\.pdb")
  message(FATAL_ERROR "unexpected PDB_DIR [${PDB_DIR}]")
endif()
