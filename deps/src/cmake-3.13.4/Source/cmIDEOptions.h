/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmIDEOptions_h
#define cmIDEOptions_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>
#include <vector>

struct cmIDEFlagTable;

/** \class cmIDEOptions
 * \brief Superclass for IDE option processing
 */
class cmIDEOptions
{
public:
  cmIDEOptions();
  virtual ~cmIDEOptions();

  // Store definitions, includes and flags.
  void AddDefine(const std::string& define);
  void AddDefines(std::string const& defines);
  void AddDefines(const std::vector<std::string>& defines);
  std::vector<std::string> const& GetDefines() const;

  void AddInclude(const std::string& includes);
  void AddIncludes(std::string const& includes);
  void AddIncludes(const std::vector<std::string>& includes);
  std::vector<std::string> const& GetIncludes() const;

  void AddFlag(std::string const& flag, std::string const& value);
  void AddFlag(std::string const& flag, std::vector<std::string> const& value);
  void AppendFlag(std::string const& flag, std::string const& value);
  void AppendFlag(std::string const& flag,
                  std::vector<std::string> const& value);
  void AppendFlagString(std::string const& flag, std::string const& value);
  void RemoveFlag(std::string const& flag);
  bool HasFlag(std::string const& flag) const;
  const char* GetFlag(std::string const& flag) const;

protected:
  // create a map of xml tags to the values they should have in the output
  // for example, "BufferSecurityCheck" = "TRUE"
  // first fill this table with the values for the configuration
  // Debug, Release, etc,
  // Then parse the command line flags specified in CMAKE_CXX_FLAGS
  // and CMAKE_C_FLAGS
  // and overwrite or add new values to this map
  class FlagValue : public std::vector<std::string>
  {
    typedef std::vector<std::string> derived;

  public:
    FlagValue& operator=(std::string const& r)
    {
      this->resize(1);
      this->operator[](0) = r;
      return *this;
    }
    FlagValue& operator=(std::vector<std::string> const& r)
    {
      this->derived::operator=(r);
      return *this;
    }
    FlagValue& append_with_space(std::string const& r)
    {
      this->resize(1);
      std::string& l = this->operator[](0);
      if (!l.empty()) {
        l += " ";
      }
      l += r;
      return *this;
    }
  };
  std::map<std::string, FlagValue> FlagMap;

  // Preprocessor definitions.
  std::vector<std::string> Defines;

  // Include directories.
  std::vector<std::string> Includes;

  bool DoingDefine;
  bool AllowDefine;
  bool DoingInclude;
  bool AllowInclude;
  bool AllowSlash;
  cmIDEFlagTable const* DoingFollowing;
  enum
  {
    FlagTableCount = 16
  };
  cmIDEFlagTable const* FlagTable[FlagTableCount];
  void HandleFlag(std::string const& flag);
  bool CheckFlagTable(cmIDEFlagTable const* table, std::string const& flag,
                      bool& flag_handled);
  void FlagMapUpdate(cmIDEFlagTable const* entry,
                     std::string const& new_value);
  virtual void StoreUnknownFlag(std::string const& flag) = 0;
};

#endif
