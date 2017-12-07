/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmPolicies_h
#define cmPolicies_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <bitset>
#include <string>

class cmMakefile;

#define CM_FOR_EACH_POLICY_TABLE(POLICY, SELECT)                              \
  SELECT(POLICY, CMP0000,                                                     \
         "A minimum required CMake version must be specified.", 2, 6, 0,      \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0001,                                                     \
         "CMAKE_BACKWARDS_COMPATIBILITY should no longer be used.", 2, 6, 0,  \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0002, "Logical target names must be globally unique.", 2, \
         6, 0, cmPolicies::WARN)                                              \
  SELECT(                                                                     \
    POLICY, CMP0003,                                                          \
    "Libraries linked via full path no longer produce linker search paths.",  \
    2, 6, 0, cmPolicies::WARN)                                                \
  SELECT(POLICY, CMP0004,                                                     \
         "Libraries linked may not have leading or trailing whitespace.", 2,  \
         6, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0005,                                                     \
         "Preprocessor definition values are now escaped automatically.", 2,  \
         6, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0006,                                                     \
         "Installing MACOSX_BUNDLE targets requires a BUNDLE DESTINATION.",   \
         2, 6, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0007, "list command no longer ignores empty elements.",   \
         2, 6, 0, cmPolicies::WARN)                                           \
  SELECT(                                                                     \
    POLICY, CMP0008,                                                          \
    "Libraries linked by full-path must have a valid library file name.", 2,  \
    6, 1, cmPolicies::WARN)                                                   \
  SELECT(POLICY, CMP0009,                                                     \
         "FILE GLOB_RECURSE calls should not follow symlinks by default.", 2, \
         6, 2, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0010, "Bad variable reference syntax is an error.", 2, 6, \
         3, cmPolicies::WARN)                                                 \
  SELECT(POLICY, CMP0011,                                                     \
         "Included scripts do automatic cmake_policy PUSH and POP.", 2, 6, 3, \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0012, "if() recognizes numbers and boolean constants.",   \
         2, 8, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0013, "Duplicate binary directories are not allowed.", 2, \
         8, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0014, "Input directories must have CMakeLists.txt.", 2,   \
         8, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0015,                                                     \
         "link_directories() treats paths relative to the source dir.", 2, 8, \
         1, cmPolicies::WARN)                                                 \
  SELECT(POLICY, CMP0016,                                                     \
         "target_link_libraries() reports error if its only argument "        \
         "is not a target.",                                                  \
         2, 8, 3, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0017,                                                     \
         "Prefer files from the CMake module directory when including from "  \
         "there.",                                                            \
         2, 8, 4, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0018,                                                     \
         "Ignore CMAKE_SHARED_LIBRARY_<Lang>_FLAGS variable.", 2, 8, 9,       \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0019,                                                     \
         "Do not re-expand variables in include and link information.", 2, 8, \
         11, cmPolicies::WARN)                                                \
  SELECT(POLICY, CMP0020,                                                     \
         "Automatically link Qt executables to qtmain target on Windows.", 2, \
         8, 11, cmPolicies::WARN)                                             \
  SELECT(                                                                     \
    POLICY, CMP0021,                                                          \
    "Fatal error on relative paths in INCLUDE_DIRECTORIES target property.",  \
    2, 8, 12, cmPolicies::WARN)                                               \
  SELECT(POLICY, CMP0022,                                                     \
         "INTERFACE_LINK_LIBRARIES defines the link interface.", 2, 8, 12,    \
         cmPolicies::WARN)                                                    \
  SELECT(                                                                     \
    POLICY, CMP0023,                                                          \
    "Plain and keyword target_link_libraries signatures cannot be mixed.", 2, \
    8, 12, cmPolicies::WARN)                                                  \
  SELECT(POLICY, CMP0024, "Disallow include export result.", 3, 0, 0,         \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0025, "Compiler id for Apple Clang is now AppleClang.",   \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0026, "Disallow use of the LOCATION target property.", 3, \
         0, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0027,                                                     \
         "Conditionally linked imported targets with missing include "        \
         "directories.",                                                      \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0028,                                                     \
         "Double colon in target name means ALIAS or IMPORTED target.", 3, 0, \
         0, cmPolicies::WARN)                                                 \
  SELECT(POLICY, CMP0029, "The subdir_depends command should not be called.", \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0030,                                                     \
         "The use_mangled_mesa command should not be called.", 3, 0, 0,       \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0031, "The load_command command should not be called.",   \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0032,                                                     \
         "The output_required_files command should not be called.", 3, 0, 0,  \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0033,                                                     \
         "The export_library_dependencies command should not be called.", 3,  \
         0, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0034, "The utility_source command should not be called.", \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0035,                                                     \
         "The variable_requires command should not be called.", 3, 0, 0,      \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0036, "The build_name command should not be called.", 3,  \
         0, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0037,                                                     \
         "Target names should not be reserved and should match a validity "   \
         "pattern.",                                                          \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0038, "Targets may not link directly to themselves.", 3,  \
         0, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0039, "Utility targets may not have link dependencies.",  \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0040,                                                     \
         "The target in the TARGET signature of add_custom_command() must "   \
         "exist.",                                                            \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0041,                                                     \
         "Error on relative include with generator expression.", 3, 0, 0,     \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0042, "MACOSX_RPATH is enabled by default.", 3, 0, 0,     \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0043, "Ignore COMPILE_DEFINITIONS_<Config> properties.",  \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0044,                                                     \
         "Case sensitive <LANG>_COMPILER_ID generator expressions.", 3, 0, 0, \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0045,                                                     \
         "Error on non-existent target in get_target_property.", 3, 0, 0,     \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0046,                                                     \
         "Error on non-existent dependency in add_dependencies.", 3, 0, 0,    \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0047, "Use QCC compiler id for the qcc drivers on QNX.",  \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0048, "project() command manages VERSION variables.", 3,  \
         0, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0049,                                                     \
         "Do not expand variables in target source entries.", 3, 0, 0,        \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0050, "Disallow add_custom_command SOURCE signatures.",   \
         3, 0, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0051, "List TARGET_OBJECTS in SOURCES target property.",  \
         3, 1, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0052, "Reject source and build dirs in installed "        \
                          "INTERFACE_INCLUDE_DIRECTORIES.",                   \
         3, 1, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0053,                                                     \
         "Simplify variable reference and escape sequence evaluation.", 3, 1, \
         0, cmPolicies::WARN)                                                 \
  SELECT(                                                                     \
    POLICY, CMP0054,                                                          \
    "Only interpret if() arguments as variables or keywords when unquoted.",  \
    3, 1, 0, cmPolicies::WARN)                                                \
  SELECT(POLICY, CMP0055, "Strict checking for break() command.", 3, 2, 0,    \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0056,                                                     \
         "Honor link flags in try_compile() source-file signature.", 3, 2, 0, \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0057, "Support new IN_LIST if() operator.", 3, 3, 0,      \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0058,                                                     \
         "Ninja requires custom command byproducts to be explicit.", 3, 3, 0, \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0059,                                                     \
         "Do not treat DEFINITIONS as a built-in directory property.", 3, 3,  \
         0, cmPolicies::WARN)                                                 \
  SELECT(POLICY, CMP0060,                                                     \
         "Link libraries by full path even in implicit directories.", 3, 3,   \
         0, cmPolicies::WARN)                                                 \
  SELECT(POLICY, CMP0061,                                                     \
         "CTest does not by default tell make to ignore errors (-i).", 3, 3,  \
         0, cmPolicies::WARN)                                                 \
  SELECT(POLICY, CMP0062, "Disallow install() of export() result.", 3, 3, 0,  \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0063,                                                     \
         "Honor visibility properties for all target types.", 3, 3, 0,        \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0064, "Support new TEST if() operator.", 3, 4, 0,         \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0065,                                                     \
         "Do not add flags to export symbols from executables without "       \
         "the ENABLE_EXPORTS target property.",                               \
         3, 4, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0066,                                                     \
         "Honor per-config flags in try_compile() source-file signature.", 3, \
         7, 0, cmPolicies::WARN)                                              \
  SELECT(POLICY, CMP0067,                                                     \
         "Honor language standard in try_compile() source-file signature.",   \
         3, 8, 0, cmPolicies::WARN)                                           \
  SELECT(POLICY, CMP0068,                                                     \
         "RPATH settings on macOS do not affect install_name.", 3, 9, 0,      \
         cmPolicies::WARN)                                                    \
  SELECT(POLICY, CMP0069,                                                     \
         "INTERPROCEDURAL_OPTIMIZATION is enforced when enabled.", 3, 9, 0,   \
         cmPolicies::WARN)

#define CM_SELECT_ID(F, A1, A2, A3, A4, A5, A6) F(A1)
#define CM_FOR_EACH_POLICY_ID(POLICY)                                         \
  CM_FOR_EACH_POLICY_TABLE(POLICY, CM_SELECT_ID)

#define CM_FOR_EACH_TARGET_POLICY(F)                                          \
  F(CMP0003)                                                                  \
  F(CMP0004)                                                                  \
  F(CMP0008)                                                                  \
  F(CMP0020)                                                                  \
  F(CMP0021)                                                                  \
  F(CMP0022)                                                                  \
  F(CMP0027)                                                                  \
  F(CMP0038)                                                                  \
  F(CMP0041)                                                                  \
  F(CMP0042)                                                                  \
  F(CMP0046)                                                                  \
  F(CMP0052)                                                                  \
  F(CMP0060)                                                                  \
  F(CMP0063)                                                                  \
  F(CMP0065)                                                                  \
  F(CMP0068)                                                                  \
  F(CMP0069)

/** \class cmPolicies
 * \brief Handles changes in CMake behavior and policies
 *
 * See the cmake wiki section on
 * <a href="https://cmake.org/Wiki/CMake/Policies">policies</a>
 * for an overview of this class's purpose
 */
class cmPolicies
{
public:
  /// Status of a policy
  enum PolicyStatus
  {
    OLD,  ///< Use old behavior
    WARN, ///< Use old behavior but issue a warning
    NEW,  ///< Use new behavior
    /// Issue an error if user doesn't set policy status to NEW and hits the
    /// check
    REQUIRED_IF_USED,
    REQUIRED_ALWAYS ///< Issue an error unless user sets policy status to NEW.
  };

  /// Policy identifiers
  enum PolicyID
  {
#define POLICY_ENUM(POLICY_ID) POLICY_ID,
    CM_FOR_EACH_POLICY_ID(POLICY_ENUM)
#undef POLICY_ENUM

    /** \brief Always the last entry.
     *
     * Useful mostly to avoid adding a comma the last policy when adding a new
     * one.
     */
    CMPCOUNT
  };

  ///! convert a string policy ID into a number
  static bool GetPolicyID(const char* id, /* out */ cmPolicies::PolicyID& pid);

  ///! Get the default status for a policy
  static cmPolicies::PolicyStatus GetPolicyStatus(cmPolicies::PolicyID id);

  ///! Set a policy level for this listfile
  static bool ApplyPolicyVersion(cmMakefile* mf, const char* version);

  ///! return a warning string for a given policy
  static std::string GetPolicyWarning(cmPolicies::PolicyID id);
  static std::string GetPolicyDeprecatedWarning(cmPolicies::PolicyID id);

  ///! return an error string for when a required policy is unspecified
  static std::string GetRequiredPolicyError(cmPolicies::PolicyID id);

  ///! return an error string for when a required policy is unspecified
  static std::string GetRequiredAlwaysPolicyError(cmPolicies::PolicyID id);

  /** Represent a set of policy values.  */
  struct PolicyMap
  {
    PolicyStatus Get(PolicyID id) const;
    void Set(PolicyID id, PolicyStatus status);
    bool IsDefined(PolicyID id) const;
    bool IsEmpty() const;

  private:
#define POLICY_STATUS_COUNT 3
    std::bitset<cmPolicies::CMPCOUNT * POLICY_STATUS_COUNT> Status;
  };
};

#endif
