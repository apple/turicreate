include(RunCMake)

run_cmake(NoLangSHARED)
run_cmake(LINK_LANGUAGE-genex)
run_cmake(link-libraries-TARGET_FILE-genex)
run_cmake(link-libraries-TARGET_FILE-genex-ok)

run_cmake(DetermineFail)
