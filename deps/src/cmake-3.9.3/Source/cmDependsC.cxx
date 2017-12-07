/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmDependsC.h"

#include "cmsys/FStream.hxx"
#include <utility>

#include "cmAlgorithms.h"
#include "cmFileTimeComparison.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

#define INCLUDE_REGEX_LINE                                                    \
  "^[ \t]*[#%][ \t]*(include|import)[ \t]*[<\"]([^\">]+)([\">])"

#define INCLUDE_REGEX_LINE_MARKER "#IncludeRegexLine: "
#define INCLUDE_REGEX_SCAN_MARKER "#IncludeRegexScan: "
#define INCLUDE_REGEX_COMPLAIN_MARKER "#IncludeRegexComplain: "
#define INCLUDE_REGEX_TRANSFORM_MARKER "#IncludeRegexTransform: "

cmDependsC::cmDependsC()
  : ValidDeps(CM_NULLPTR)
{
}

cmDependsC::cmDependsC(
  cmLocalGenerator* lg, const char* targetDir, const std::string& lang,
  const std::map<std::string, DependencyVector>* validDeps)
  : cmDepends(lg, targetDir)
  , ValidDeps(validDeps)
{
  cmMakefile* mf = lg->GetMakefile();

  // Configure the include file search path.
  this->SetIncludePathFromLanguage(lang);

  // Configure regular expressions.
  std::string scanRegex = "^.*$";
  std::string complainRegex = "^$";
  {
    std::string scanRegexVar = "CMAKE_";
    scanRegexVar += lang;
    scanRegexVar += "_INCLUDE_REGEX_SCAN";
    if (const char* sr = mf->GetDefinition(scanRegexVar)) {
      scanRegex = sr;
    }
    std::string complainRegexVar = "CMAKE_";
    complainRegexVar += lang;
    complainRegexVar += "_INCLUDE_REGEX_COMPLAIN";
    if (const char* cr = mf->GetDefinition(complainRegexVar)) {
      complainRegex = cr;
    }
  }

  this->IncludeRegexLine.compile(INCLUDE_REGEX_LINE);
  this->IncludeRegexScan.compile(scanRegex.c_str());
  this->IncludeRegexComplain.compile(complainRegex.c_str());
  this->IncludeRegexLineString = INCLUDE_REGEX_LINE_MARKER INCLUDE_REGEX_LINE;
  this->IncludeRegexScanString = INCLUDE_REGEX_SCAN_MARKER;
  this->IncludeRegexScanString += scanRegex;
  this->IncludeRegexComplainString = INCLUDE_REGEX_COMPLAIN_MARKER;
  this->IncludeRegexComplainString += complainRegex;

  this->SetupTransforms();

  this->CacheFileName = this->TargetDirectory;
  this->CacheFileName += "/";
  this->CacheFileName += lang;
  this->CacheFileName += ".includecache";

  this->ReadCacheFile();
}

cmDependsC::~cmDependsC()
{
  this->WriteCacheFile();
  cmDeleteAll(this->FileCache);
}

bool cmDependsC::WriteDependencies(const std::set<std::string>& sources,
                                   const std::string& obj,
                                   std::ostream& makeDepends,
                                   std::ostream& internalDepends)
{
  // Make sure this is a scanning instance.
  if (sources.empty() || sources.begin()->empty()) {
    cmSystemTools::Error("Cannot scan dependencies without a source file.");
    return false;
  }
  if (obj.empty()) {
    cmSystemTools::Error("Cannot scan dependencies without an object file.");
    return false;
  }

  std::set<std::string> dependencies;
  bool haveDeps = false;

  if (this->ValidDeps != CM_NULLPTR) {
    std::map<std::string, DependencyVector>::const_iterator tmpIt =
      this->ValidDeps->find(obj);
    if (tmpIt != this->ValidDeps->end()) {
      dependencies.insert(tmpIt->second.begin(), tmpIt->second.end());
      haveDeps = true;
    }
  }

  if (!haveDeps) {
    // Walk the dependency graph starting with the source file.
    int srcFiles = (int)sources.size();
    this->Encountered.clear();

    for (std::set<std::string>::const_iterator srcIt = sources.begin();
         srcIt != sources.end(); ++srcIt) {
      UnscannedEntry root;
      root.FileName = *srcIt;
      this->Unscanned.push(root);
      this->Encountered.insert(*srcIt);
    }

    std::set<std::string> scanned;

    // Use reserve to allocate enough memory for tempPathStr
    // so that during the loops no memory is allocated or freed
    std::string tempPathStr;
    tempPathStr.reserve(4 * 1024);

    while (!this->Unscanned.empty()) {
      // Get the next file to scan.
      UnscannedEntry current = this->Unscanned.front();
      this->Unscanned.pop();

      // If not a full path, find the file in the include path.
      std::string fullName;
      if ((srcFiles > 0) ||
          cmSystemTools::FileIsFullPath(current.FileName.c_str())) {
        if (cmSystemTools::FileExists(current.FileName.c_str(), true)) {
          fullName = current.FileName;
        }
      } else if (!current.QuotedLocation.empty() &&
                 cmSystemTools::FileExists(current.QuotedLocation.c_str(),
                                           true)) {
        // The include statement producing this entry was a double-quote
        // include and the included file is present in the directory of
        // the source containing the include statement.
        fullName = current.QuotedLocation;
      } else {
        std::map<std::string, std::string>::iterator headerLocationIt =
          this->HeaderLocationCache.find(current.FileName);
        if (headerLocationIt != this->HeaderLocationCache.end()) {
          fullName = headerLocationIt->second;
        } else {
          for (std::vector<std::string>::const_iterator i =
                 this->IncludePath.begin();
               i != this->IncludePath.end(); ++i) {
            // Construct the name of the file as if it were in the current
            // include directory.  Avoid using a leading "./".

            tempPathStr =
              cmSystemTools::CollapseCombinedPath(*i, current.FileName);

            // Look for the file in this location.
            if (cmSystemTools::FileExists(tempPathStr.c_str(), true)) {
              fullName = tempPathStr;
              HeaderLocationCache[current.FileName] = fullName;
              break;
            }
          }
        }
      }

      // Complain if the file cannot be found and matches the complain
      // regex.
      if (fullName.empty() &&
          this->IncludeRegexComplain.find(current.FileName.c_str())) {
        cmSystemTools::Error("Cannot find file \"", current.FileName.c_str(),
                             "\".");
        return false;
      }

      // Scan the file if it was found and has not been scanned already.
      if (!fullName.empty() && (scanned.find(fullName) == scanned.end())) {
        // Record scanned files.
        scanned.insert(fullName);

        // Check whether this file is already in the cache
        std::map<std::string, cmIncludeLines*>::iterator fileIt =
          this->FileCache.find(fullName);
        if (fileIt != this->FileCache.end()) {
          fileIt->second->Used = true;
          dependencies.insert(fullName);
          for (std::vector<UnscannedEntry>::const_iterator incIt =
                 fileIt->second->UnscannedEntries.begin();
               incIt != fileIt->second->UnscannedEntries.end(); ++incIt) {
            if (this->Encountered.find(incIt->FileName) ==
                this->Encountered.end()) {
              this->Encountered.insert(incIt->FileName);
              this->Unscanned.push(*incIt);
            }
          }
        } else {

          // Try to scan the file.  Just leave it out if we cannot find
          // it.
          cmsys::ifstream fin(fullName.c_str());
          if (fin) {
            cmsys::FStream::BOM bom = cmsys::FStream::ReadBOM(fin);
            if (bom == cmsys::FStream::BOM_None ||
                bom == cmsys::FStream::BOM_UTF8) {
              // Add this file as a dependency.
              dependencies.insert(fullName);

              // Scan this file for new dependencies.  Pass the directory
              // containing the file to handle double-quote includes.
              std::string dir = cmSystemTools::GetFilenamePath(fullName);
              this->Scan(fin, dir.c_str(), fullName);
            } else {
              // Skip file with encoding we do not implement.
            }
          }
        }
      }

      srcFiles--;
    }
  }

  // Write the dependencies to the output stream.  Makefile rules
  // written by the original local generator for this directory
  // convert the dependencies to paths relative to the home output
  // directory.  We must do the same here.
  std::string binDir = this->LocalGenerator->GetBinaryDirectory();
  std::string obj_i = this->LocalGenerator->ConvertToRelativePath(binDir, obj);
  std::string obj_m = cmSystemTools::ConvertToOutputPath(obj_i.c_str());
  internalDepends << obj_i << std::endl;

  for (std::set<std::string>::const_iterator i = dependencies.begin();
       i != dependencies.end(); ++i) {
    makeDepends
      << obj_m << ": "
      << cmSystemTools::ConvertToOutputPath(
           this->LocalGenerator->ConvertToRelativePath(binDir, *i).c_str())
      << std::endl;
    internalDepends << " " << *i << std::endl;
  }
  makeDepends << std::endl;

  return true;
}

void cmDependsC::ReadCacheFile()
{
  if (this->CacheFileName.empty()) {
    return;
  }
  cmsys::ifstream fin(this->CacheFileName.c_str());
  if (!fin) {
    return;
  }

  std::string line;
  cmIncludeLines* cacheEntry = CM_NULLPTR;
  bool haveFileName = false;

  while (cmSystemTools::GetLineFromStream(fin, line)) {
    if (line.empty()) {
      cacheEntry = CM_NULLPTR;
      haveFileName = false;
      continue;
    }
    // the first line after an empty line is the name of the parsed file
    if (!haveFileName) {
      haveFileName = true;
      int newer = 0;
      cmFileTimeComparison comp;
      bool res = comp.FileTimeCompare(this->CacheFileName.c_str(),
                                      line.c_str(), &newer);

      if (res && newer == 1) // cache is newer than the parsed file
      {
        cacheEntry = new cmIncludeLines;
        this->FileCache[line] = cacheEntry;
      }
      // file doesn't exist, check that the regular expressions
      // haven't changed
      else if (!res) {
        if (line.find(INCLUDE_REGEX_LINE_MARKER) == 0) {
          if (line != this->IncludeRegexLineString) {
            return;
          }
        } else if (line.find(INCLUDE_REGEX_SCAN_MARKER) == 0) {
          if (line != this->IncludeRegexScanString) {
            return;
          }
        } else if (line.find(INCLUDE_REGEX_COMPLAIN_MARKER) == 0) {
          if (line != this->IncludeRegexComplainString) {
            return;
          }
        } else if (line.find(INCLUDE_REGEX_TRANSFORM_MARKER) == 0) {
          if (line != this->IncludeRegexTransformString) {
            return;
          }
        }
      }
    } else if (cacheEntry != CM_NULLPTR) {
      UnscannedEntry entry;
      entry.FileName = line;
      if (cmSystemTools::GetLineFromStream(fin, line)) {
        if (line != "-") {
          entry.QuotedLocation = line;
        }
        cacheEntry->UnscannedEntries.push_back(entry);
      }
    }
  }
}

void cmDependsC::WriteCacheFile() const
{
  if (this->CacheFileName.empty()) {
    return;
  }
  cmsys::ofstream cacheOut(this->CacheFileName.c_str());
  if (!cacheOut) {
    return;
  }

  cacheOut << this->IncludeRegexLineString << "\n\n";
  cacheOut << this->IncludeRegexScanString << "\n\n";
  cacheOut << this->IncludeRegexComplainString << "\n\n";
  cacheOut << this->IncludeRegexTransformString << "\n\n";

  for (std::map<std::string, cmIncludeLines*>::const_iterator fileIt =
         this->FileCache.begin();
       fileIt != this->FileCache.end(); ++fileIt) {
    if (fileIt->second->Used) {
      cacheOut << fileIt->first << std::endl;

      for (std::vector<UnscannedEntry>::const_iterator incIt =
             fileIt->second->UnscannedEntries.begin();
           incIt != fileIt->second->UnscannedEntries.end(); ++incIt) {
        cacheOut << incIt->FileName << std::endl;
        if (incIt->QuotedLocation.empty()) {
          cacheOut << "-" << std::endl;
        } else {
          cacheOut << incIt->QuotedLocation << std::endl;
        }
      }
      cacheOut << std::endl;
    }
  }
}

void cmDependsC::Scan(std::istream& is, const char* directory,
                      const std::string& fullName)
{
  cmIncludeLines* newCacheEntry = new cmIncludeLines;
  newCacheEntry->Used = true;
  this->FileCache[fullName] = newCacheEntry;

  // Read one line at a time.
  std::string line;
  while (cmSystemTools::GetLineFromStream(is, line)) {
    // Transform the line content first.
    if (!this->TransformRules.empty()) {
      this->TransformLine(line);
    }

    // Match include directives.
    if (this->IncludeRegexLine.find(line.c_str())) {
      // Get the file being included.
      UnscannedEntry entry;
      entry.FileName = this->IncludeRegexLine.match(2);
      cmSystemTools::ConvertToUnixSlashes(entry.FileName);
      if (this->IncludeRegexLine.match(3) == "\"" &&
          !cmSystemTools::FileIsFullPath(entry.FileName.c_str())) {
        // This was a double-quoted include with a relative path.  We
        // must check for the file in the directory containing the
        // file we are scanning.
        entry.QuotedLocation =
          cmSystemTools::CollapseCombinedPath(directory, entry.FileName);
      }

      // Queue the file if it has not yet been encountered and it
      // matches the regular expression for recursive scanning.  Note
      // that this check does not account for the possibility of two
      // headers with the same name in different directories when one
      // is included by double-quotes and the other by angle brackets.
      // It also does not work properly if two header files with the same
      // name exist in different directories, and both are included from a
      // file their own directory by simply using "filename.h" (#12619)
      // This kind of problem will be fixed when a more
      // preprocessor-like implementation of this scanner is created.
      if (this->IncludeRegexScan.find(entry.FileName.c_str())) {
        newCacheEntry->UnscannedEntries.push_back(entry);
        if (this->Encountered.find(entry.FileName) ==
            this->Encountered.end()) {
          this->Encountered.insert(entry.FileName);
          this->Unscanned.push(entry);
        }
      }
    }
  }
}

void cmDependsC::SetupTransforms()
{
  // Get the transformation rules.
  std::vector<std::string> transformRules;
  cmMakefile* mf = this->LocalGenerator->GetMakefile();
  if (const char* xform = mf->GetDefinition("CMAKE_INCLUDE_TRANSFORMS")) {
    cmSystemTools::ExpandListArgument(xform, transformRules, true);
  }
  for (std::vector<std::string>::const_iterator tri = transformRules.begin();
       tri != transformRules.end(); ++tri) {
    this->ParseTransform(*tri);
  }

  this->IncludeRegexTransformString = INCLUDE_REGEX_TRANSFORM_MARKER;
  if (!this->TransformRules.empty()) {
    // Construct the regular expression to match lines to be
    // transformed.
    std::string xform = "^([ \t]*[#%][ \t]*(include|import)[ \t]*)(";
    const char* sep = "";
    for (TransformRulesType::const_iterator tri = this->TransformRules.begin();
         tri != this->TransformRules.end(); ++tri) {
      xform += sep;
      xform += tri->first;
      sep = "|";
    }
    xform += ")[ \t]*\\(([^),]*)\\)";
    this->IncludeRegexTransform.compile(xform.c_str());

    // Build a string that encodes all transformation rules and will
    // change when rules are changed.
    this->IncludeRegexTransformString += xform;
    for (TransformRulesType::const_iterator tri = this->TransformRules.begin();
         tri != this->TransformRules.end(); ++tri) {
      this->IncludeRegexTransformString += " ";
      this->IncludeRegexTransformString += tri->first;
      this->IncludeRegexTransformString += "(%)=";
      this->IncludeRegexTransformString += tri->second;
    }
  }
}

void cmDependsC::ParseTransform(std::string const& xform)
{
  // A transform rule is of the form SOME_MACRO(%)=value-with-%
  // We can simply separate with "(%)=".
  std::string::size_type pos = xform.find("(%)=");
  if (pos == std::string::npos || pos == 0) {
    return;
  }
  std::string name = xform.substr(0, pos);
  std::string value = xform.substr(pos + 4);
  this->TransformRules[name] = value;
}

void cmDependsC::TransformLine(std::string& line)
{
  // Check for a transform rule match.  Return if none.
  if (!this->IncludeRegexTransform.find(line.c_str())) {
    return;
  }
  TransformRulesType::const_iterator tri =
    this->TransformRules.find(this->IncludeRegexTransform.match(3));
  if (tri == this->TransformRules.end()) {
    return;
  }

  // Construct the transformed line.
  std::string newline = this->IncludeRegexTransform.match(1);
  std::string arg = this->IncludeRegexTransform.match(4);
  for (const char* c = tri->second.c_str(); *c; ++c) {
    if (*c == '%') {
      newline += arg;
    } else {
      newline += *c;
    }
  }

  // Return the transformed line.
  line = newline;
}
