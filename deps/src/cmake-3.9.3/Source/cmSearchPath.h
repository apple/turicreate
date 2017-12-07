/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSearchPath_h
#define cmSearchPath_h

#include "cmConfigure.h"

#include <set>
#include <string>
#include <vector>

class cmFindCommon;

/** \class cmSearchPath
 * \brief Container for encapsulating a set of search paths
 *
 * cmSearchPath is a container that encapsulates search path construction and
 * management
 */
class cmSearchPath
{
public:
  // cmSearchPath must be initialized from a valid pointer.  The only reason
  // for the default is to allow it to be easily used in stl containers.
  // Attempting to initialize with a NULL value will fail an assertion
  cmSearchPath(cmFindCommon* findCmd = CM_NULLPTR);
  ~cmSearchPath();

  const std::vector<std::string>& GetPaths() const { return this->Paths; }

  void ExtractWithout(const std::set<std::string>& ignore,
                      std::vector<std::string>& outPaths,
                      bool clear = false) const;

  void AddPath(const std::string& path);
  void AddUserPath(const std::string& path);
  void AddCMakePath(const std::string& variable);
  void AddEnvPath(const std::string& variable);
  void AddCMakePrefixPath(const std::string& variable);
  void AddEnvPrefixPath(const std::string& variable, bool stripBin = false);
  void AddSuffixes(const std::vector<std::string>& suffixes);

protected:
  void AddPrefixPaths(const std::vector<std::string>& paths,
                      const char* base = CM_NULLPTR);
  void AddPathInternal(const std::string& path, const char* base = CM_NULLPTR);

  cmFindCommon* FC;
  std::vector<std::string> Paths;
};

#endif
