include(CheckCXXSourceCompiles)

macro(Set_Compiler_Specific_Flags)

#**************************************************************************/
#*                                                                        */
#*       Some C++ Implementation Oddities between libc++ and stdc++       */
#*                                                                        */
#**************************************************************************/

  check_cxx_source_compiles("#include <ios>
                             #include <system_error>
                             int main(int argc, char** argv) {
                               throw std::ios_base::failure(\"hello\", std::error_code());
                               }" COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE)

  if(COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE)
    message(STATUS "Compiler supports ios_base::failure(str, error_code)")
    add_definitions(-DCOMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE)
  else()
    message(STATUS "Compiler does not support ios_base::failure(str, error_code)")
  endif()


# Check for _NOEXCEPT on std::ios_base::failure


  check_cxx_source_compiles("#include <ios>
                             #include <system_error>

                             class io_error : public std::ios_base::failure {
                               public:   /* The noexcept here is different between C++ 17 and C++ 11. */
                                 virtual const char *what() const noexcept { return \"\"; }
                             };
                             int main(int argc, char** argv) { return 0; }"
                            COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS_V1)
  
  check_cxx_source_compiles("#include <ios>
                             #include <system_error>

                             class io_error : public std::ios_base::failure {
                               public:   /* The noexcept here is different between C++ 17 and C++ 11. */
                                 virtual const char *what() const _NOEXCEPT { return \"\"; }
                             };
                             int main(int argc, char** argv) { return 0; }"
                            COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS_V2)

  check_cxx_source_compiles("#include <ios>
                             #include <system_error>

                             class io_error : public std::ios_base::failure {
                             public:   /* A noexcept here is different between C++ 17 and C++ 11. */
                                virtual const char *what() const { return \"\"; }
                             };
                             int main(int argc, char** argv) { return 0; }"
                             COMPILER_NO_NOEXCEPT_WHAT_ON_EXCEPTIONS)
  if(NOT COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS_V1
     AND NOT COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS_V2
     AND NOT COMPILER_NO_NOEXCEPT_WHAT_ON_EXCEPTIONS) 

    message(FATAL_ERROR "Cannot determine noexcept fladg on std::ios_base::failure.  See log.")
  elseif(COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS_V1) 
    add_definitions(-DCOMPILER_MODIFIER_ON_EXCEPTION_WHAT=noexcept)
  elseif(COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS_V2)
    add_definitions(-DCOMPILER_MODIFIER_ON_EXCEPTION_WHAT=_NOEXCEPT)
  else()
    add_definitions(-DCOMPILER_MODIFIER_ON_EXCEPTION_WHAT="")
  endif()

endmacro()
