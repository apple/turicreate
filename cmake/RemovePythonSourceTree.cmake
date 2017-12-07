# Note: when executed in the build dir, then CMAKE_CURRENT_SOURCE_DIR is the
# build dir.

file(GLOB_RECURSE pythonfiles "${CMAKE_ARGV3}/*")
foreach(item ${pythonfiles})
  get_filename_component(fname ${item} NAME)
  if ( "${fname}" MATCHES ".*\\.py$" OR
       "${fname}" MATCHES ".*\\.pyc$" OR
       "${fname}" MATCHES ".*\\.ipynb$"  OR
       "${fname}" MATCHES ".*\\.png$"  OR
       "${fname}" MATCHES ".*\\.in$"  OR
       "${fname}" MATCHES ".*\\.js$"  OR
       "${fname}" MATCHES ".*\\.html$"  OR
       "${fname}" MATCHES ".*\\.ico$"  OR
       "${fname}" MATCHES ".*\\.sh$"  OR
       "${fname}" MATCHES ".*\\.css$"  OR
       "${fname}" MATCHES ".*\\.lua$"  OR
       "${fname}" MATCHES "LICENSE$"  OR
       "${fname}" MATCHES ".*\\.otf$"  OR
       "${fname}" MATCHES ".*\\.eot$"  OR
       "${fname}" MATCHES ".*\\.svg$"  OR
       "${fname}" MATCHES ".*\\.ttf$"  OR
       "${fname}" MATCHES ".*\\.woff$" OR
       "${fname}" MATCHES ".*\\.so$"  OR
       "${fname}" MATCHES ".*\\.so\\..*$" OR
       "${fname}" MATCHES ".*\\.dylib$" OR
       "${fname}" MATCHES ".*\\.dll$" OR
       "${fname}" MATCHES ".*\\.dll$" 
       )
    file(REMOVE ${item})
  endif()
endforeach()


file (REMOVE_RECURSE ${CMAKE_ARGV3}/docs)
