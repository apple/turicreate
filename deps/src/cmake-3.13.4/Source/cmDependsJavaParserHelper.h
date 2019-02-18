/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmDependsJavaParserHelper_h
#define cmDependsJavaParserHelper_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

/** \class cmDependsJavaParserHelper
 * \brief Helper class for parsing java source files
 *
 * Finds dependencies for java file and list of outputs
 */
class cmDependsJavaParserHelper
{
public:
  struct ParserType
  {
    char* str;
  };

  cmDependsJavaParserHelper();
  ~cmDependsJavaParserHelper();

  int ParseString(const char* str, int verb);
  int ParseFile(const char* file);

  // For the lexer:
  void AllocateParserType(cmDependsJavaParserHelper::ParserType* pt,
                          const char* str, int len = 0);

  int LexInput(char* buf, int maxlen);
  void Error(const char* str);

  // For yacc
  void AddClassFound(const char* sclass);
  void PrepareElement(ParserType* me);
  void DeallocateParserType(char** pt);
  void CheckEmpty(int line, int cnt, ParserType* pt);
  void StartClass(const char* cls);
  void EndClass();
  void AddPackagesImport(const char* sclass);
  void SetCurrentPackage(const char* pkg) { this->CurrentPackage = pkg; }
  const char* GetCurrentPackage() { return this->CurrentPackage.c_str(); }
  void SetCurrentCombine(const char* cmb) { this->CurrentCombine = cmb; }
  const char* GetCurrentCombine() { return this->CurrentCombine.c_str(); }
  void UpdateCombine(const char* str1, const char* str2);

  std::vector<std::string>& GetClassesFound() { return this->ClassesFound; }

  std::vector<std::string> GetFilesProduced();

private:
  class CurrentClass
  {
  public:
    std::string Name;
    std::vector<CurrentClass> NestedClasses;
    void AddFileNamesForPrinting(std::vector<std::string>* files,
                                 const char* prefix, const char* sep) const;
  };
  std::string CurrentPackage;
  std::string::size_type InputBufferPos;
  std::string InputBuffer;
  std::vector<char> OutputBuffer;
  std::vector<std::string> ClassesFound;
  std::vector<std::string> PackagesImport;
  std::string CurrentCombine;

  std::vector<CurrentClass> ClassStack;

  int CurrentLine;
  int UnionsAvailable;
  int LastClassId;
  int CurrentDepth;
  int Verbose;

  std::vector<char*> Allocates;

  void PrintClasses();

  void Print(const char* place, const char* str);
  void CombineUnions(char** out, const char* in1, char** in2, const char* sep);
  void SafePrintMissing(const char* str, int line, int cnt);

  void CleanupParser();
};

#define YYSTYPE cmDependsJavaParserHelper::ParserType
#define YYSTYPE_IS_DECLARED
#define YY_EXTRA_TYPE cmDependsJavaParserHelper*
#define YY_DECL int cmDependsJava_yylex(YYSTYPE* yylvalp, yyscan_t yyscanner)

#endif
