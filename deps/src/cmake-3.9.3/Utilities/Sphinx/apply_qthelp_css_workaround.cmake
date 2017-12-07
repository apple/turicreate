
file(READ "${CSS_DIR}/basic.css" BasicCssContent)

if(EXISTS "${CSS_DIR}/default.css")
  file(READ "${CSS_DIR}/default.css" DefaultCssContent)
  string(REPLACE
    "@import url(\"basic.css\")" "${BasicCssContent}"
    DefaultCssContent "${DefaultCssContent}"
  )
else()
  set(DefaultCssContent "${BasicCssContent}")
endif()

file(READ "${CSS_DIR}/cmake.css" CMakeCssContent)
string(REPLACE
  "@import url(\"default.css\")" "${DefaultCssContent}"
  CMakeCssContent "${CMakeCssContent}"
)
file(WRITE "${CSS_DIR}/cmake.css" "${CMakeCssContent}")
