/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmParseJacocoCoverage_h
#define cmParseJacocoCoverage_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>
#include <vector>

class cmCTest;
class cmCTestCoverageHandlerContainer;

/** \class cmParseJacocoCoverage
 * \brief Parse JaCoCO coverage information
 *
 * This class is used to parse coverage information for
 * java using the JaCoCo tool:
 *
 * http://www.eclemma.org/jacoco/trunk/index.html
 */
class cmParseJacocoCoverage
{
public:
  cmParseJacocoCoverage(cmCTestCoverageHandlerContainer& cont, cmCTest* ctest);
  bool LoadCoverageData(std::vector<std::string> const& files);

  std::string PackageName;
  std::string FileName;
  std::string ModuleName;
  std::string CurFileName;

private:
  // implement virtual from parent
  // remove files with no coverage
  void RemoveUnCoveredFiles();
  // Read a single mcov file
  bool ReadJacocoXML(const char* f);
  // split a string based on ,
  bool SplitString(std::vector<std::string>& args, std::string const& line);
  bool FindJavaFile(std::string const& routine, std::string& filepath);
  void InitializeJavaFile(std::string& file);
  bool LoadSource(std::string d);

  class XMLParser;

  std::map<std::string, std::string> RoutineToDirectory;
  cmCTestCoverageHandlerContainer& Coverage;
  cmCTest* CTest;
};

#endif
