
enable_language(CXX C)

add_library(empty empty.cpp empty.c)
target_compile_options(empty
  PRIVATE LANG_IS_$<COMPILE_LANGUAGE>
)

file(GENERATE
  OUTPUT opts-$<COMPILE_LANGUAGE>.txt
  CONTENT "$<TARGET_PROPERTY:empty,COMPILE_OPTIONS>\n"
)
