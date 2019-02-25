/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmVisualStudioSlnParser_h
#define cmVisualStudioSlnParser_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <bitset>
#include <iosfwd>
#include <stddef.h>
#include <string>

class cmSlnData;

class cmVisualStudioSlnParser
{
public:
  enum ParseResult
  {
    ResultOK = 0,

    ResultInternalError = -1,
    ResultExternalError = 1,

    ResultErrorOpeningInput = ResultExternalError,
    ResultErrorReadingInput,
    ResultErrorInputStructure,
    ResultErrorInputData,

    ResultErrorBadInternalState = ResultInternalError,
    ResultErrorUnsupportedDataGroup = ResultInternalError - 1
  };

  enum DataGroup
  {
    DataGroupProjectsBit,
    DataGroupProjectDependenciesBit,
    DataGroupSolutionConfigurationsBit,
    DataGroupProjectConfigurationsBit,
    DataGroupSolutionFiltersBit,
    DataGroupGenericGlobalSectionsBit,
    DataGroupCount
  };

  typedef std::bitset<DataGroupCount> DataGroupSet;

  static const DataGroupSet DataGroupProjects;
  static const DataGroupSet DataGroupProjectDependencies;
  static const DataGroupSet DataGroupSolutionConfigurations;
  static const DataGroupSet DataGroupProjectConfigurations;
  static const DataGroupSet DataGroupSolutionFilters;
  static const DataGroupSet DataGroupGenericGlobalSections;
  static const DataGroupSet DataGroupAll;

  bool Parse(std::istream& input, cmSlnData& output,
             DataGroupSet dataGroups = DataGroupAll);

  bool ParseFile(const std::string& file, cmSlnData& output,
                 DataGroupSet dataGroups = DataGroupAll);

  ParseResult GetParseResult() const;

  size_t GetParseResultLine() const;

  bool GetParseHadBOM() const;

protected:
  class State;

  friend class State;
  class ParsedLine;

  struct ResultData
  {
    ParseResult Result;
    size_t ResultLine;
    bool HadBOM;

    ResultData();
    void Clear();
    void SetError(ParseResult error, size_t line);
  } LastResult;

  bool IsDataGroupSetSupported(DataGroupSet dataGroups) const;

  bool ParseImpl(std::istream& input, cmSlnData& output, State& state);

  bool ParseBOM(std::istream& input, std::string& line, State& state);

  bool ParseMultiValueTag(const std::string& line, ParsedLine& parsedLine,
                          State& state);

  bool ParseSingleValueTag(const std::string& line, ParsedLine& parsedLine,
                           State& state);

  bool ParseKeyValuePair(const std::string& line, ParsedLine& parsedLine,
                         State& state);

  bool ParseTag(const std::string& fullTag, ParsedLine& parsedLine,
                State& state);

  bool ParseValue(const std::string& value, ParsedLine& parsedLine);
};

#endif
