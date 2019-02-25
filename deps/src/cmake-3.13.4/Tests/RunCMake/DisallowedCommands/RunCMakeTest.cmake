include(RunCMake)

foreach(p
    CMP0029
    CMP0030
    CMP0031
    CMP0032
    CMP0033
    CMP0034
    CMP0035
    CMP0036
    )
  run_cmake(${p}-WARN)
  run_cmake(${p}-OLD)
  run_cmake(${p}-NEW)
endforeach()
