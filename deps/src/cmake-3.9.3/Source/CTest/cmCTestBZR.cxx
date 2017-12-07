/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestBZR.h"

#include "cmCTest.h"
#include "cmCTestVC.h"
#include "cmProcessTools.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"

#include "cm_expat.h"
#include "cmsys/RegularExpression.hxx"
#include <list>
#include <map>
#include <ostream>
#include <stdlib.h>
#include <vector>

extern "C" int cmBZRXMLParserUnknownEncodingHandler(void* /*unused*/,
                                                    const XML_Char* name,
                                                    XML_Encoding* info)
{
  static const int latin1[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
    0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010, 0x0011,
    0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A,
    0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023,
    0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C,
    0x002D, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
    0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E,
    0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050,
    0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
    0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 0x0060, 0x0061, 0x0062,
    0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B,
    0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
    0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D,
    0x007E, 0x007F, 0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020,
    0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC,
    0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178, 0x00A0, 0x00A1,
    0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA,
    0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF, 0x00B0, 0x00B1, 0x00B2, 0x00B3,
    0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC,
    0x00BD, 0x00BE, 0x00BF, 0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5,
    0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE,
    0x00CF, 0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
    0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF, 0x00E0,
    0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9,
    0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F0, 0x00F1, 0x00F2,
    0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB,
    0x00FC, 0x00FD, 0x00FE, 0x00FF
  };

  // The BZR xml output plugin can use some encodings that are not
  // recognized by expat.  This will lead to an error, e.g. "Error
  // parsing bzr log xml: unknown encoding", the following is a
  // workaround for these unknown encodings.
  if (name == std::string("ascii") || name == std::string("cp1252") ||
      name == std::string("ANSI_X3.4-1968")) {
    for (unsigned int i = 0; i < 256; ++i) {
      info->map[i] = latin1[i];
    }
    return 1;
  }

  return 0;
}

cmCTestBZR::cmCTestBZR(cmCTest* ct, std::ostream& log)
  : cmCTestGlobalVC(ct, log)
{
  this->PriorRev = this->Unknown;
  // Even though it is specified in the documentation, with bzr 1.13
  // BZR_PROGRESS_BAR has no effect. In the future this bug might be fixed.
  // Since it doesn't hurt, we specify this environment variable.
  cmSystemTools::PutEnv("BZR_PROGRESS_BAR=none");
}

cmCTestBZR::~cmCTestBZR()
{
}

class cmCTestBZR::InfoParser : public cmCTestVC::LineParser
{
public:
  InfoParser(cmCTestBZR* bzr, const char* prefix)
    : BZR(bzr)
    , CheckOutFound(false)
  {
    this->SetLog(&bzr->Log, prefix);
    this->RegexCheckOut.compile("checkout of branch: *([^\t\r\n]+)$");
    this->RegexParent.compile("parent branch: *([^\t\r\n]+)$");
  }

private:
  cmCTestBZR* BZR;
  bool CheckOutFound;
  cmsys::RegularExpression RegexCheckOut;
  cmsys::RegularExpression RegexParent;
  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexCheckOut.find(this->Line)) {
      this->BZR->URL = this->RegexCheckOut.match(1);
      CheckOutFound = true;
    } else if (!CheckOutFound && this->RegexParent.find(this->Line)) {
      this->BZR->URL = this->RegexParent.match(1);
    }
    return true;
  }
};

class cmCTestBZR::RevnoParser : public cmCTestVC::LineParser
{
public:
  RevnoParser(cmCTestBZR* bzr, const char* prefix, std::string& rev)
    : Rev(rev)
  {
    this->SetLog(&bzr->Log, prefix);
    this->RegexRevno.compile("^([0-9]+)$");
  }

private:
  std::string& Rev;
  cmsys::RegularExpression RegexRevno;
  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexRevno.find(this->Line)) {
      this->Rev = this->RegexRevno.match(1);
    }
    return true;
  }
};

std::string cmCTestBZR::LoadInfo()
{
  // Run "bzr info" to get the repository info from the work tree.
  const char* bzr = this->CommandLineTool.c_str();
  const char* bzr_info[] = { bzr, "info", CM_NULLPTR };
  InfoParser iout(this, "info-out> ");
  OutputLogger ierr(this->Log, "info-err> ");
  this->RunChild(bzr_info, &iout, &ierr);

  // Run "bzr revno" to get the repository revision number from the work tree.
  const char* bzr_revno[] = { bzr, "revno", CM_NULLPTR };
  std::string rev;
  RevnoParser rout(this, "revno-out> ", rev);
  OutputLogger rerr(this->Log, "revno-err> ");
  this->RunChild(bzr_revno, &rout, &rerr);

  return rev;
}

bool cmCTestBZR::NoteOldRevision()
{
  this->OldRevision = this->LoadInfo();
  this->Log << "Revision before update: " << this->OldRevision << "\n";
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   Old revision of repository is: "
               << this->OldRevision << "\n");
  this->PriorRev.Rev = this->OldRevision;
  return true;
}

bool cmCTestBZR::NoteNewRevision()
{
  this->NewRevision = this->LoadInfo();
  this->Log << "Revision after update: " << this->NewRevision << "\n";
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   New revision of repository is: "
               << this->NewRevision << "\n");
  this->Log << "URL = " << this->URL << "\n";
  return true;
}

class cmCTestBZR::LogParser : public cmCTestVC::OutputLogger,
                              private cmXMLParser
{
public:
  LogParser(cmCTestBZR* bzr, const char* prefix)
    : OutputLogger(bzr->Log, prefix)
    , BZR(bzr)
    , EmailRegex("(.*) <([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+)>")
  {
    this->InitializeParser();
  }
  ~LogParser() CM_OVERRIDE { this->CleanupParser(); }

  int InitializeParser() CM_OVERRIDE
  {
    int res = cmXMLParser::InitializeParser();
    if (res) {
      XML_SetUnknownEncodingHandler(static_cast<XML_Parser>(this->Parser),
                                    cmBZRXMLParserUnknownEncodingHandler,
                                    CM_NULLPTR);
    }
    return res;
  }

private:
  cmCTestBZR* BZR;

  typedef cmCTestBZR::Revision Revision;
  typedef cmCTestBZR::Change Change;
  Revision Rev;
  std::vector<Change> Changes;
  Change CurChange;
  std::vector<char> CData;

  cmsys::RegularExpression EmailRegex;

  bool ProcessChunk(const char* data, int length) CM_OVERRIDE
  {
    this->OutputLogger::ProcessChunk(data, length);
    this->ParseChunk(data, length);
    return true;
  }

  void StartElement(const std::string& name, const char** /*atts*/) CM_OVERRIDE
  {
    this->CData.clear();
    if (name == "log") {
      this->Rev = Revision();
      this->Changes.clear();
    }
    // affected-files can contain blocks of
    // modified, unknown, renamed, kind-changed, removed, conflicts, added
    else if (name == "modified" || name == "renamed" ||
             name == "kind-changed") {
      this->CurChange = Change();
      this->CurChange.Action = 'M';
    } else if (name == "added") {
      this->CurChange = Change();
      this->CurChange = 'A';
    } else if (name == "removed") {
      this->CurChange = Change();
      this->CurChange = 'D';
    } else if (name == "unknown" || name == "conflicts") {
      // Should not happen here
      this->CurChange = Change();
    }
  }

  void CharacterDataHandler(const char* data, int length) CM_OVERRIDE
  {
    this->CData.insert(this->CData.end(), data, data + length);
  }

  void EndElement(const std::string& name) CM_OVERRIDE
  {
    if (name == "log") {
      this->BZR->DoRevision(this->Rev, this->Changes);
    } else if (!this->CData.empty() &&
               (name == "file" || name == "directory")) {
      this->CurChange.Path.assign(&this->CData[0], this->CData.size());
      cmSystemTools::ConvertToUnixSlashes(this->CurChange.Path);
      this->Changes.push_back(this->CurChange);
    } else if (!this->CData.empty() && name == "symlink") {
      // symlinks have an arobase at the end in the log
      this->CurChange.Path.assign(&this->CData[0], this->CData.size() - 1);
      cmSystemTools::ConvertToUnixSlashes(this->CurChange.Path);
      this->Changes.push_back(this->CurChange);
    } else if (!this->CData.empty() && name == "committer") {
      this->Rev.Author.assign(&this->CData[0], this->CData.size());
      if (this->EmailRegex.find(this->Rev.Author)) {
        this->Rev.Author = this->EmailRegex.match(1);
        this->Rev.EMail = this->EmailRegex.match(2);
      }
    } else if (!this->CData.empty() && name == "timestamp") {
      this->Rev.Date.assign(&this->CData[0], this->CData.size());
    } else if (!this->CData.empty() && name == "message") {
      this->Rev.Log.assign(&this->CData[0], this->CData.size());
    } else if (!this->CData.empty() && name == "revno") {
      this->Rev.Rev.assign(&this->CData[0], this->CData.size());
    }
    this->CData.clear();
  }

  void ReportError(int /*line*/, int /*column*/, const char* msg) CM_OVERRIDE
  {
    this->BZR->Log << "Error parsing bzr log xml: " << msg << "\n";
  }
};

class cmCTestBZR::UpdateParser : public cmCTestVC::LineParser
{
public:
  UpdateParser(cmCTestBZR* bzr, const char* prefix)
    : BZR(bzr)
  {
    this->SetLog(&bzr->Log, prefix);
    this->RegexUpdate.compile("^([-+R?XCP ])([NDKM ])([* ]) +(.+)$");
  }

private:
  cmCTestBZR* BZR;
  cmsys::RegularExpression RegexUpdate;

  bool ProcessChunk(const char* first, int length) CM_OVERRIDE
  {
    bool last_is_new_line = (*first == '\r' || *first == '\n');

    const char* const last = first + length;
    for (const char* c = first; c != last; ++c) {
      if (*c == '\r' || *c == '\n') {
        if (!last_is_new_line) {
          // Log this line.
          if (this->Log && this->Prefix) {
            *this->Log << this->Prefix << this->Line << "\n";
          }

          // Hand this line to the subclass implementation.
          if (!this->ProcessLine()) {
            this->Line = "";
            return false;
          }

          this->Line = "";
          last_is_new_line = true;
        }
      } else {
        // Append this character to the line under construction.
        this->Line.append(1, *c);
        last_is_new_line = false;
      }
    }
    return true;
  }

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexUpdate.find(this->Line)) {
      this->DoPath(this->RegexUpdate.match(1)[0],
                   this->RegexUpdate.match(2)[0],
                   this->RegexUpdate.match(3)[0], this->RegexUpdate.match(4));
    }
    return true;
  }

  void DoPath(char c0, char c1, char c2, std::string path)
  {
    if (path.empty()) {
      return;
    }
    cmSystemTools::ConvertToUnixSlashes(path);

    const std::string dir = cmSystemTools::GetFilenamePath(path);
    const std::string name = cmSystemTools::GetFilenameName(path);

    if (c0 == 'C') {
      this->BZR->Dirs[dir][name].Status = PathConflicting;
      return;
    }

    if (c1 == 'M' || c1 == 'K' || c1 == 'N' || c1 == 'D' || c2 == '*') {
      this->BZR->Dirs[dir][name].Status = PathUpdated;
      return;
    }
  }
};

bool cmCTestBZR::UpdateImpl()
{
  // Get user-specified update options.
  std::string opts = this->CTest->GetCTestConfiguration("UpdateOptions");
  if (opts.empty()) {
    opts = this->CTest->GetCTestConfiguration("BZRUpdateOptions");
  }
  std::vector<std::string> args = cmSystemTools::ParseArguments(opts.c_str());

  // TODO: if(this->CTest->GetTestModel() == cmCTest::NIGHTLY)

  // Use "bzr pull" to update the working tree.
  std::vector<char const*> bzr_update;
  bzr_update.push_back(this->CommandLineTool.c_str());
  bzr_update.push_back("pull");

  for (std::vector<std::string>::const_iterator ai = args.begin();
       ai != args.end(); ++ai) {
    bzr_update.push_back(ai->c_str());
  }

  bzr_update.push_back(this->URL.c_str());

  bzr_update.push_back(CM_NULLPTR);

  // For some reason bzr uses stderr to display the update status.
  OutputLogger out(this->Log, "pull-out> ");
  UpdateParser err(this, "pull-err> ");
  return this->RunUpdateCommand(&bzr_update[0], &out, &err);
}

bool cmCTestBZR::LoadRevisions()
{
  cmCTestLog(this->CTest, HANDLER_OUTPUT,
             "   Gathering version information (one . per revision):\n"
             "    "
               << std::flush);

  // We are interested in every revision included in the update.
  this->Revisions.clear();
  std::string revs;
  if (atoi(this->OldRevision.c_str()) <= atoi(this->NewRevision.c_str())) {
    // DoRevision takes care of discarding the information about OldRevision
    revs = this->OldRevision + ".." + this->NewRevision;
  } else {
    return true;
  }

  // Run "bzr log" to get all global revisions of interest.
  const char* bzr = this->CommandLineTool.c_str();
  const char* bzr_log[] = {
    bzr,       "log", "-v", "-r", revs.c_str(), "--xml", this->URL.c_str(),
    CM_NULLPTR
  };
  {
    LogParser out(this, "log-out> ");
    OutputLogger err(this->Log, "log-err> ");
    this->RunChild(bzr_log, &out, &err);
  }
  cmCTestLog(this->CTest, HANDLER_OUTPUT, std::endl);
  return true;
}

class cmCTestBZR::StatusParser : public cmCTestVC::LineParser
{
public:
  StatusParser(cmCTestBZR* bzr, const char* prefix)
    : BZR(bzr)
  {
    this->SetLog(&bzr->Log, prefix);
    this->RegexStatus.compile("^([-+R?XCP ])([NDKM ])([* ]) +(.+)$");
  }

private:
  cmCTestBZR* BZR;
  cmsys::RegularExpression RegexStatus;
  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexStatus.find(this->Line)) {
      this->DoPath(this->RegexStatus.match(1)[0],
                   this->RegexStatus.match(2)[0],
                   this->RegexStatus.match(3)[0], this->RegexStatus.match(4));
    }
    return true;
  }

  void DoPath(char c0, char c1, char c2, std::string path)
  {
    if (path.empty()) {
      return;
    }
    cmSystemTools::ConvertToUnixSlashes(path);

    if (c0 == 'C') {
      this->BZR->DoModification(PathConflicting, path);
      return;
    }

    if (c0 == '+' || c0 == 'R' || c0 == 'P' || c1 == 'M' || c1 == 'K' ||
        c1 == 'N' || c1 == 'D' || c2 == '*') {
      this->BZR->DoModification(PathModified, path);
      return;
    }
  }
};

bool cmCTestBZR::LoadModifications()
{
  // Run "bzr status" which reports local modifications.
  const char* bzr = this->CommandLineTool.c_str();
  const char* bzr_status[] = { bzr, "status", "-SV", CM_NULLPTR };
  StatusParser out(this, "status-out> ");
  OutputLogger err(this->Log, "status-err> ");
  this->RunChild(bzr_status, &out, &err);
  return true;
}
