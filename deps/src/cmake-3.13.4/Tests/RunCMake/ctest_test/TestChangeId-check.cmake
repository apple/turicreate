file(GLOB test_xml_file "${RunCMake_TEST_BINARY_DIR}/Testing/*/Test.xml")
if(test_xml_file)
  file(READ "${test_xml_file}" test_xml LIMIT 4096)
  if(NOT test_xml MATCHES [[ChangeId="&lt;&gt;1"]])
    string(REPLACE "\n" "\n  " test_xml "  ${test_xml}")
    set(RunCMake_TEST_FAILED
      "Test.xml does not have expected ChangeId:\n${test_xml}"
      )
  endif()
else()
  set(RunCMake_TEST_FAILED "Test.xml not found")
endif()
