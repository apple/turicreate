/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmVisualStudioSlnParser.h"

#include "cmSystemTools.h"
#include "cmVisualStudioSlnData.h"
#include "cmsys/FStream.hxx"

#include <cassert>
#include <stack>

namespace {
enum LineFormat
{
  LineMultiValueTag,
  LineSingleValueTag,
  LineKeyValuePair,
  LineVerbatim
};
}

class cmVisualStudioSlnParser::ParsedLine
{
public:
  bool IsComment() const;
  bool IsKeyValuePair() const;

  const std::string& GetTag() const { return this->Tag; }
  const std::string& GetArg() const { return this->Arg.first; }
  std::string GetArgVerbatim() const;
  size_t GetValueCount() const { return this->Values.size(); }
  const std::string& GetValue(size_t idxValue) const;
  std::string GetValueVerbatim(size_t idxValue) const;

  void SetTag(const std::string& tag) { this->Tag = tag; }
  void SetArg(const std::string& arg) { this->Arg = StringData(arg, false); }
  void SetQuotedArg(const std::string& arg)
  {
    this->Arg = StringData(arg, true);
  }
  void AddValue(const std::string& value)
  {
    this->Values.push_back(StringData(value, false));
  }
  void AddQuotedValue(const std::string& value)
  {
    this->Values.push_back(StringData(value, true));
  }

  void CopyVerbatim(const std::string& line) { this->Tag = line; }

private:
  typedef std::pair<std::string, bool> StringData;
  std::string Tag;
  StringData Arg;
  std::vector<StringData> Values;
  static const std::string BadString;
  static const std::string Quote;
};

const std::string cmVisualStudioSlnParser::ParsedLine::BadString;
const std::string cmVisualStudioSlnParser::ParsedLine::Quote("\"");

bool cmVisualStudioSlnParser::ParsedLine::IsComment() const
{
  assert(!this->Tag.empty());
  return (this->Tag[0] == '#');
}

bool cmVisualStudioSlnParser::ParsedLine::IsKeyValuePair() const
{
  assert(!this->Tag.empty());
  return this->Arg.first.empty() && this->Values.size() == 1;
}

std::string cmVisualStudioSlnParser::ParsedLine::GetArgVerbatim() const
{
  if (this->Arg.second)
    return Quote + this->Arg.first + Quote;
  else
    return this->Arg.first;
}

const std::string& cmVisualStudioSlnParser::ParsedLine::GetValue(
  size_t idxValue) const
{
  if (idxValue < this->Values.size())
    return this->Values[idxValue].first;
  else
    return BadString;
}

std::string cmVisualStudioSlnParser::ParsedLine::GetValueVerbatim(
  size_t idxValue) const
{
  if (idxValue < this->Values.size()) {
    const StringData& data = this->Values[idxValue];
    if (data.second)
      return Quote + data.first + Quote;
    else
      return data.first;
  } else
    return BadString;
}

class cmVisualStudioSlnParser::State
{
public:
  explicit State(DataGroupSet requestedData);

  size_t GetCurrentLine() const { return this->CurrentLine; }
  bool ReadLine(std::istream& input, std::string& line);

  LineFormat NextLineFormat() const;

  bool Process(const cmVisualStudioSlnParser::ParsedLine& line,
               cmSlnData& output, cmVisualStudioSlnParser::ResultData& result);

  bool Finished(cmVisualStudioSlnParser::ResultData& result);

private:
  enum FileState
  {
    FileStateStart,
    FileStateTopLevel,
    FileStateProject,
    FileStateProjectDependencies,
    FileStateGlobal,
    FileStateSolutionConfigurations,
    FileStateProjectConfigurations,
    FileStateSolutionFilters,
    FileStateGlobalSection,
    FileStateIgnore
  };
  std::stack<FileState> Stack;
  std::string EndIgnoreTag;
  DataGroupSet RequestedData;
  size_t CurrentLine;

  void IgnoreUntilTag(const std::string& endTag);
};

cmVisualStudioSlnParser::State::State(DataGroupSet requestedData)
  : RequestedData(requestedData)
  , CurrentLine(0)
{
  if (this->RequestedData.test(DataGroupProjectDependenciesBit))
    this->RequestedData.set(DataGroupProjectsBit);
  this->Stack.push(FileStateStart);
}

bool cmVisualStudioSlnParser::State::ReadLine(std::istream& input,
                                              std::string& line)
{
  ++this->CurrentLine;
  return !std::getline(input, line).fail();
}

LineFormat cmVisualStudioSlnParser::State::NextLineFormat() const
{
  switch (this->Stack.top()) {
    case FileStateStart:
      return LineVerbatim;
    case FileStateTopLevel:
      return LineMultiValueTag;
    case FileStateProject:
      return LineSingleValueTag;
    case FileStateProjectDependencies:
      return LineKeyValuePair;
    case FileStateGlobal:
      return LineSingleValueTag;
    case FileStateSolutionConfigurations:
      return LineKeyValuePair;
    case FileStateProjectConfigurations:
      return LineKeyValuePair;
    case FileStateSolutionFilters:
      return LineKeyValuePair;
    case FileStateGlobalSection:
      return LineKeyValuePair;
    case FileStateIgnore:
      return LineVerbatim;
    default:
      assert(false);
      return LineVerbatim;
  }
}

bool cmVisualStudioSlnParser::State::Process(
  const cmVisualStudioSlnParser::ParsedLine& line, cmSlnData& output,
  cmVisualStudioSlnParser::ResultData& result)
{
  assert(!line.IsComment());
  switch (this->Stack.top()) {
    case FileStateStart:
      if (!cmSystemTools::StringStartsWith(
            line.GetTag().c_str(), "Microsoft Visual Studio Solution File")) {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      this->Stack.pop();
      this->Stack.push(FileStateTopLevel);
      break;
    case FileStateTopLevel:
      if (line.GetTag().compare("Project") == 0) {
        if (line.GetValueCount() != 3) {
          result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
          return false;
        }
        if (this->RequestedData.test(DataGroupProjectsBit)) {
          if (!output.AddProject(line.GetValue(2), line.GetValue(0),
                                 line.GetValue(1))) {
            result.SetError(ResultErrorInputData, this->GetCurrentLine());
            return false;
          }
          this->Stack.push(FileStateProject);
        } else
          this->IgnoreUntilTag("EndProject");
      } else if (line.GetTag().compare("Global") == 0)
        this->Stack.push(FileStateGlobal);
      else {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      break;
    case FileStateProject:
      if (line.GetTag().compare("EndProject") == 0)
        this->Stack.pop();
      else if (line.GetTag().compare("ProjectSection") == 0) {
        if (line.GetArg().compare("ProjectDependencies") == 0 &&
            line.GetValue(0).compare("postProject") == 0) {
          if (this->RequestedData.test(DataGroupProjectDependenciesBit))
            this->Stack.push(FileStateProjectDependencies);
          else
            this->IgnoreUntilTag("EndProjectSection");
        } else
          this->IgnoreUntilTag("EndProjectSection");
      } else {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      break;
    case FileStateProjectDependencies:
      if (line.GetTag().compare("EndProjectSection") == 0)
        this->Stack.pop();
      else if (line.IsKeyValuePair())
        // implement dependency storing here, once needed
        ;
      else {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      break;
    case FileStateGlobal:
      if (line.GetTag().compare("EndGlobal") == 0)
        this->Stack.pop();
      else if (line.GetTag().compare("GlobalSection") == 0) {
        if (line.GetArg().compare("SolutionConfigurationPlatforms") == 0 &&
            line.GetValue(0).compare("preSolution") == 0) {
          if (this->RequestedData.test(DataGroupSolutionConfigurationsBit))
            this->Stack.push(FileStateSolutionConfigurations);
          else
            this->IgnoreUntilTag("EndGlobalSection");
        } else if (line.GetArg().compare("ProjectConfigurationPlatforms") ==
                     0 &&
                   line.GetValue(0).compare("postSolution") == 0) {
          if (this->RequestedData.test(DataGroupProjectConfigurationsBit))
            this->Stack.push(FileStateProjectConfigurations);
          else
            this->IgnoreUntilTag("EndGlobalSection");
        } else if (line.GetArg().compare("NestedProjects") == 0 &&
                   line.GetValue(0).compare("preSolution") == 0) {
          if (this->RequestedData.test(DataGroupSolutionFiltersBit))
            this->Stack.push(FileStateSolutionFilters);
          else
            this->IgnoreUntilTag("EndGlobalSection");
        } else if (this->RequestedData.test(DataGroupGenericGlobalSectionsBit))
          this->Stack.push(FileStateGlobalSection);
        else
          this->IgnoreUntilTag("EndGlobalSection");
      } else {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      break;
    case FileStateSolutionConfigurations:
      if (line.GetTag().compare("EndGlobalSection") == 0)
        this->Stack.pop();
      else if (line.IsKeyValuePair())
        // implement configuration storing here, once needed
        ;
      else {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      break;
    case FileStateProjectConfigurations:
      if (line.GetTag().compare("EndGlobalSection") == 0)
        this->Stack.pop();
      else if (line.IsKeyValuePair())
        // implement configuration storing here, once needed
        ;
      else {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      break;
    case FileStateSolutionFilters:
      if (line.GetTag().compare("EndGlobalSection") == 0)
        this->Stack.pop();
      else if (line.IsKeyValuePair())
        // implement filter storing here, once needed
        ;
      else {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      break;
    case FileStateGlobalSection:
      if (line.GetTag().compare("EndGlobalSection") == 0)
        this->Stack.pop();
      else if (line.IsKeyValuePair())
        // implement section storing here, once needed
        ;
      else {
        result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
        return false;
      }
      break;
    case FileStateIgnore:
      if (line.GetTag() == this->EndIgnoreTag) {
        this->Stack.pop();
        this->EndIgnoreTag.clear();
      }
      break;
    default:
      result.SetError(ResultErrorBadInternalState, this->GetCurrentLine());
      return false;
  }
  return true;
}

bool cmVisualStudioSlnParser::State::Finished(
  cmVisualStudioSlnParser::ResultData& result)
{
  if (this->Stack.top() != FileStateTopLevel) {
    result.SetError(ResultErrorInputStructure, this->GetCurrentLine());
    return false;
  }
  result.Result = ResultOK;
  return true;
}

void cmVisualStudioSlnParser::State::IgnoreUntilTag(const std::string& endTag)
{
  this->Stack.push(FileStateIgnore);
  this->EndIgnoreTag = endTag;
}

cmVisualStudioSlnParser::ResultData::ResultData()
  : Result(ResultOK)
  , ResultLine(0)
{
}

void cmVisualStudioSlnParser::ResultData::Clear()
{
  *this = ResultData();
}

void cmVisualStudioSlnParser::ResultData::SetError(ParseResult error,
                                                   size_t line)
{
  this->Result = error;
  this->ResultLine = line;
}

const cmVisualStudioSlnParser::DataGroupSet
  cmVisualStudioSlnParser::DataGroupProjects(
    1 << cmVisualStudioSlnParser::DataGroupProjectsBit);

const cmVisualStudioSlnParser::DataGroupSet
  cmVisualStudioSlnParser::DataGroupProjectDependencies(
    1 << cmVisualStudioSlnParser::DataGroupProjectDependenciesBit);

const cmVisualStudioSlnParser::DataGroupSet
  cmVisualStudioSlnParser::DataGroupSolutionConfigurations(
    1 << cmVisualStudioSlnParser::DataGroupSolutionConfigurationsBit);

const cmVisualStudioSlnParser::DataGroupSet
  cmVisualStudioSlnParser::DataGroupProjectConfigurations(
    1 << cmVisualStudioSlnParser::DataGroupProjectConfigurationsBit);

const cmVisualStudioSlnParser::DataGroupSet
  cmVisualStudioSlnParser::DataGroupSolutionFilters(
    1 << cmVisualStudioSlnParser::DataGroupSolutionFiltersBit);

const cmVisualStudioSlnParser::DataGroupSet
  cmVisualStudioSlnParser::DataGroupGenericGlobalSections(
    1 << cmVisualStudioSlnParser::DataGroupGenericGlobalSectionsBit);

const cmVisualStudioSlnParser::DataGroupSet
  cmVisualStudioSlnParser::DataGroupAll(~0);

bool cmVisualStudioSlnParser::Parse(std::istream& input, cmSlnData& output,
                                    DataGroupSet dataGroups)
{
  this->LastResult.Clear();
  if (!this->IsDataGroupSetSupported(dataGroups)) {
    this->LastResult.SetError(ResultErrorUnsupportedDataGroup, 0);
    return false;
  }
  State state(dataGroups);
  return this->ParseImpl(input, output, state);
}

bool cmVisualStudioSlnParser::ParseFile(const std::string& file,
                                        cmSlnData& output,
                                        DataGroupSet dataGroups)
{
  this->LastResult.Clear();
  if (!this->IsDataGroupSetSupported(dataGroups)) {
    this->LastResult.SetError(ResultErrorUnsupportedDataGroup, 0);
    return false;
  }
  cmsys::ifstream f(file.c_str());
  if (!f) {
    this->LastResult.SetError(ResultErrorOpeningInput, 0);
    return false;
  }
  State state(dataGroups);
  return this->ParseImpl(f, output, state);
}

cmVisualStudioSlnParser::ParseResult cmVisualStudioSlnParser::GetParseResult()
  const
{
  return this->LastResult.Result;
}

size_t cmVisualStudioSlnParser::GetParseResultLine() const
{
  return this->LastResult.ResultLine;
}

bool cmVisualStudioSlnParser::GetParseHadBOM() const
{
  return this->LastResult.HadBOM;
}

bool cmVisualStudioSlnParser::IsDataGroupSetSupported(
  DataGroupSet dataGroups) const
{
  return (dataGroups & DataGroupProjects) == dataGroups;
  // only supporting DataGroupProjects for now
}

bool cmVisualStudioSlnParser::ParseImpl(std::istream& input, cmSlnData& output,
                                        State& state)
{
  std::string line;
  // Does the .sln start with a Byte Order Mark?
  if (!this->ParseBOM(input, line, state))
    return false;
  do {
    line = cmSystemTools::TrimWhitespace(line);
    if (line.empty())
      continue;
    ParsedLine parsedLine;
    switch (state.NextLineFormat()) {
      case LineMultiValueTag:
        if (!this->ParseMultiValueTag(line, parsedLine, state))
          return false;
        break;
      case LineSingleValueTag:
        if (!this->ParseSingleValueTag(line, parsedLine, state))
          return false;
        break;
      case LineKeyValuePair:
        if (!this->ParseKeyValuePair(line, parsedLine, state))
          return false;
        break;
      case LineVerbatim:
        parsedLine.CopyVerbatim(line);
        break;
    }
    if (parsedLine.IsComment())
      continue;
    if (!state.Process(parsedLine, output, this->LastResult))
      return false;
  } while (state.ReadLine(input, line));
  return state.Finished(this->LastResult);
}

bool cmVisualStudioSlnParser::ParseBOM(std::istream& input, std::string& line,
                                       State& state)
{
  char bom[4];
  if (!input.get(bom, 4)) {
    this->LastResult.SetError(ResultErrorReadingInput, 1);
    return false;
  }
  this->LastResult.HadBOM =
    (bom[0] == char(0xEF) && bom[1] == char(0xBB) && bom[2] == char(0xBF));
  if (!state.ReadLine(input, line)) {
    this->LastResult.SetError(ResultErrorReadingInput, 1);
    return false;
  }
  if (!this->LastResult.HadBOM)
    line = bom + line; // it wasn't a BOM, prepend it to first line
  return true;
}

bool cmVisualStudioSlnParser::ParseMultiValueTag(const std::string& line,
                                                 ParsedLine& parsedLine,
                                                 State& state)
{
  size_t idxEqualSign = line.find('=');
  const std::string& fullTag = line.substr(0, idxEqualSign);
  if (!this->ParseTag(fullTag, parsedLine, state))
    return false;
  if (idxEqualSign != line.npos) {
    size_t idxFieldStart = idxEqualSign + 1;
    if (idxFieldStart < line.size()) {
      size_t idxParsing = idxFieldStart;
      bool inQuotes = false;
      for (;;) {
        idxParsing = line.find_first_of(",\"", idxParsing);
        bool fieldOver = false;
        if (idxParsing == line.npos) {
          fieldOver = true;
          if (inQuotes) {
            this->LastResult.SetError(ResultErrorInputStructure,
                                      state.GetCurrentLine());
            return false;
          }
        } else if (line[idxParsing] == ',' && !inQuotes)
          fieldOver = true;
        else if (line[idxParsing] == '"')
          inQuotes = !inQuotes;
        if (fieldOver) {
          if (!this->ParseValue(
                line.substr(idxFieldStart, idxParsing - idxFieldStart),
                parsedLine))
            return false;
          if (idxParsing == line.npos)
            break; // end of last field
          idxFieldStart = idxParsing + 1;
        }
        ++idxParsing;
      }
    }
  }
  return true;
}

bool cmVisualStudioSlnParser::ParseSingleValueTag(const std::string& line,
                                                  ParsedLine& parsedLine,
                                                  State& state)
{
  size_t idxEqualSign = line.find('=');
  const std::string& fullTag = line.substr(0, idxEqualSign);
  if (!this->ParseTag(fullTag, parsedLine, state))
    return false;
  if (idxEqualSign != line.npos) {
    if (!this->ParseValue(line.substr(idxEqualSign + 1), parsedLine))
      return false;
  }
  return true;
}

bool cmVisualStudioSlnParser::ParseKeyValuePair(const std::string& line,
                                                ParsedLine& parsedLine,
                                                State& /*state*/)
{
  size_t idxEqualSign = line.find('=');
  if (idxEqualSign == line.npos) {
    parsedLine.CopyVerbatim(line);
    return true;
  }
  const std::string& key = line.substr(0, idxEqualSign);
  parsedLine.SetTag(cmSystemTools::TrimWhitespace(key));
  const std::string& value = line.substr(idxEqualSign + 1);
  parsedLine.AddValue(cmSystemTools::TrimWhitespace(value));
  return true;
}

bool cmVisualStudioSlnParser::ParseTag(const std::string& fullTag,
                                       ParsedLine& parsedLine, State& state)
{
  size_t idxLeftParen = fullTag.find('(');
  if (idxLeftParen == fullTag.npos) {
    parsedLine.SetTag(cmSystemTools::TrimWhitespace(fullTag));
    return true;
  }
  parsedLine.SetTag(
    cmSystemTools::TrimWhitespace(fullTag.substr(0, idxLeftParen)));
  size_t idxRightParen = fullTag.rfind(')');
  if (idxRightParen == fullTag.npos) {
    this->LastResult.SetError(ResultErrorInputStructure,
                              state.GetCurrentLine());
    return false;
  }
  const std::string& arg = cmSystemTools::TrimWhitespace(
    fullTag.substr(idxLeftParen + 1, idxRightParen - idxLeftParen - 1));
  if (arg[0] == '"') {
    if (arg[arg.size() - 1] != '"') {
      this->LastResult.SetError(ResultErrorInputStructure,
                                state.GetCurrentLine());
      return false;
    }
    parsedLine.SetQuotedArg(arg.substr(1, arg.size() - 2));
  } else
    parsedLine.SetArg(arg);
  return true;
}

bool cmVisualStudioSlnParser::ParseValue(const std::string& value,
                                         ParsedLine& parsedLine)
{
  const std::string& trimmed = cmSystemTools::TrimWhitespace(value);
  if (trimmed.empty())
    parsedLine.AddValue(trimmed);
  else if (trimmed[0] == '"' && trimmed[trimmed.size() - 1] == '"')
    parsedLine.AddQuotedValue(trimmed.substr(1, trimmed.size() - 2));
  else
    parsedLine.AddValue(trimmed);
  return true;
}
