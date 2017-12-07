/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestP4.h"

#include "cmCTest.h"
#include "cmCTestVC.h"
#include "cmProcessTools.h"
#include "cmSystemTools.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <ostream>
#include <time.h>
#include <utility>

cmCTestP4::cmCTestP4(cmCTest* ct, std::ostream& log)
  : cmCTestGlobalVC(ct, log)
{
  this->PriorRev = this->Unknown;
}

cmCTestP4::~cmCTestP4()
{
}

class cmCTestP4::IdentifyParser : public cmCTestVC::LineParser
{
public:
  IdentifyParser(cmCTestP4* p4, const char* prefix, std::string& rev)
    : Rev(rev)
  {
    this->SetLog(&p4->Log, prefix);
    this->RegexIdentify.compile("^Change ([0-9]+) on");
  }

private:
  std::string& Rev;
  cmsys::RegularExpression RegexIdentify;

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexIdentify.find(this->Line)) {
      this->Rev = this->RegexIdentify.match(1);
      return false;
    }
    return true;
  }
};

class cmCTestP4::ChangesParser : public cmCTestVC::LineParser
{
public:
  ChangesParser(cmCTestP4* p4, const char* prefix)
    : P4(p4)
  {
    this->SetLog(&P4->Log, prefix);
    this->RegexIdentify.compile("^Change ([0-9]+) on");
  }

private:
  cmsys::RegularExpression RegexIdentify;
  cmCTestP4* P4;

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexIdentify.find(this->Line)) {
      P4->ChangeLists.push_back(this->RegexIdentify.match(1));
    }
    return true;
  }
};

class cmCTestP4::UserParser : public cmCTestVC::LineParser
{
public:
  UserParser(cmCTestP4* p4, const char* prefix)
    : P4(p4)
  {
    this->SetLog(&P4->Log, prefix);
    this->RegexUser.compile("^(.+) <(.*)> \\((.*)\\) accessed (.*)$");
  }

private:
  cmsys::RegularExpression RegexUser;
  cmCTestP4* P4;

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexUser.find(this->Line)) {
      User NewUser;

      NewUser.UserName = this->RegexUser.match(1);
      NewUser.EMail = this->RegexUser.match(2);
      NewUser.Name = this->RegexUser.match(3);
      NewUser.AccessTime = this->RegexUser.match(4);
      P4->Users[this->RegexUser.match(1)] = NewUser;

      return false;
    }
    return true;
  }
};

/* Diff format:
==== //depot/file#rev - /absolute/path/to/file ====
(diff data)
==== //depot/file2#rev - /absolute/path/to/file2 ====
(diff data)
==== //depot/file3#rev - /absolute/path/to/file3 ====
==== //depot/file4#rev - /absolute/path/to/file4 ====
(diff data)
*/
class cmCTestP4::DiffParser : public cmCTestVC::LineParser
{
public:
  DiffParser(cmCTestP4* p4, const char* prefix)
    : P4(p4)
    , AlreadyNotified(false)
  {
    this->SetLog(&P4->Log, prefix);
    this->RegexDiff.compile("^==== (.*)#[0-9]+ - (.*)");
  }

private:
  cmCTestP4* P4;
  bool AlreadyNotified;
  std::string CurrentPath;
  cmsys::RegularExpression RegexDiff;

  bool ProcessLine() CM_OVERRIDE
  {
    if (!this->Line.empty() && this->Line[0] == '=' &&
        this->RegexDiff.find(this->Line)) {
      CurrentPath = this->RegexDiff.match(1);
      AlreadyNotified = false;
    } else {
      if (!AlreadyNotified) {
        P4->DoModification(PathModified, CurrentPath);
        AlreadyNotified = true;
      }
    }
    return true;
  }
};

cmCTestP4::User cmCTestP4::GetUserData(const std::string& username)
{
  std::map<std::string, cmCTestP4::User>::const_iterator it =
    Users.find(username);

  if (it == Users.end()) {
    std::vector<char const*> p4_users;
    SetP4Options(p4_users);
    p4_users.push_back("users");
    p4_users.push_back("-m");
    p4_users.push_back("1");
    p4_users.push_back(username.c_str());
    p4_users.push_back(CM_NULLPTR);

    UserParser out(this, "users-out> ");
    OutputLogger err(this->Log, "users-err> ");
    RunChild(&p4_users[0], &out, &err);

    // The user should now be added to the map. Search again.
    it = Users.find(username);
    if (it == Users.end()) {
      return cmCTestP4::User();
    }
  }

  return it->second;
}

/* Commit format:

Change 1111111 by user@client on 2013/09/26 11:50:36

        text
        text

Affected files ...

... //path/to/file#rev edit
... //path/to/file#rev add
... //path/to/file#rev delete
... //path/to/file#rev integrate
*/
class cmCTestP4::DescribeParser : public cmCTestVC::LineParser
{
public:
  DescribeParser(cmCTestP4* p4, const char* prefix)
    : LineParser('\n', false)
    , P4(p4)
    , Section(SectionHeader)
  {
    this->SetLog(&P4->Log, prefix);
    this->RegexHeader.compile("^Change ([0-9]+) by (.+)@(.+) on (.*)$");
    this->RegexDiff.compile("^\\.\\.\\. (.*)#[0-9]+ ([^ ]+)$");
  }

private:
  cmsys::RegularExpression RegexHeader;
  cmsys::RegularExpression RegexDiff;
  cmCTestP4* P4;

  typedef cmCTestP4::Revision Revision;
  typedef cmCTestP4::Change Change;
  std::vector<Change> Changes;
  enum SectionType
  {
    SectionHeader,
    SectionBody,
    SectionDiffHeader,
    SectionDiff,
    SectionCount
  };
  SectionType Section;
  Revision Rev;

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->Line.empty()) {
      this->NextSection();
    } else {
      switch (this->Section) {
        case SectionHeader:
          this->DoHeaderLine();
          break;
        case SectionBody:
          this->DoBodyLine();
          break;
        case SectionDiffHeader:
          break; // nothing to do
        case SectionDiff:
          this->DoDiffLine();
          break;
        case SectionCount:
          break; // never happens
      }
    }
    return true;
  }

  void NextSection()
  {
    if (this->Section == SectionDiff) {
      this->P4->DoRevision(this->Rev, this->Changes);
      this->Rev = Revision();
    }

    this->Section = SectionType((this->Section + 1) % SectionCount);
  }

  void DoHeaderLine()
  {
    if (this->RegexHeader.find(this->Line)) {
      this->Rev.Rev = this->RegexHeader.match(1);
      this->Rev.Date = this->RegexHeader.match(4);

      cmCTestP4::User user = P4->GetUserData(this->RegexHeader.match(2));
      this->Rev.Author = user.Name;
      this->Rev.EMail = user.EMail;

      this->Rev.Committer = this->Rev.Author;
      this->Rev.CommitterEMail = this->Rev.EMail;
      this->Rev.CommitDate = this->Rev.Date;
    }
  }

  void DoBodyLine()
  {
    if (this->Line[0] == '\t') {
      this->Rev.Log += this->Line.substr(1);
    }
    this->Rev.Log += "\n";
  }

  void DoDiffLine()
  {
    if (this->RegexDiff.find(this->Line)) {
      Change change;
      std::string Path = this->RegexDiff.match(1);
      if (Path.length() > 2 && Path[0] == '/' && Path[1] == '/') {
        size_t found = Path.find('/', 2);
        if (found != std::string::npos) {
          Path = Path.substr(found + 1);
        }
      }

      change.Path = Path;
      std::string action = this->RegexDiff.match(2);

      if (action == "add") {
        change.Action = 'A';
      } else if (action == "delete") {
        change.Action = 'D';
      } else if (action == "edit" || action == "integrate") {
        change.Action = 'M';
      }

      Changes.push_back(change);
    }
  }
};

void cmCTestP4::SetP4Options(std::vector<char const*>& CommandOptions)
{
  if (P4Options.empty()) {
    const char* p4 = this->CommandLineTool.c_str();
    P4Options.push_back(p4);

    // The CTEST_P4_CLIENT variable sets the P4 client used when issuing
    // Perforce commands, if it's different from the default one.
    std::string client = this->CTest->GetCTestConfiguration("P4Client");
    if (!client.empty()) {
      P4Options.push_back("-c");
      P4Options.push_back(client);
    }

    // Set the message language to be English, in case the P4 admin
    // has localized them
    P4Options.push_back("-L");
    P4Options.push_back("en");

    // The CTEST_P4_OPTIONS variable adds additional Perforce command line
    // options before the main command
    std::string opts = this->CTest->GetCTestConfiguration("P4Options");
    std::vector<std::string> args =
      cmSystemTools::ParseArguments(opts.c_str());

    P4Options.insert(P4Options.end(), args.begin(), args.end());
  }

  CommandOptions.clear();
  for (std::vector<std::string>::iterator i = P4Options.begin();
       i != P4Options.end(); ++i) {
    CommandOptions.push_back(i->c_str());
  }
}

std::string cmCTestP4::GetWorkingRevision()
{
  std::vector<char const*> p4_identify;
  SetP4Options(p4_identify);

  p4_identify.push_back("changes");
  p4_identify.push_back("-m");
  p4_identify.push_back("1");
  p4_identify.push_back("-t");

  std::string source = this->SourceDirectory + "/...#have";
  p4_identify.push_back(source.c_str());
  p4_identify.push_back(CM_NULLPTR);

  std::string rev;
  IdentifyParser out(this, "p4_changes-out> ", rev);
  OutputLogger err(this->Log, "p4_changes-err> ");

  bool result = RunChild(&p4_identify[0], &out, &err);

  // If there was a problem contacting the server return "<unknown>"
  if (!result) {
    return "<unknown>";
  }

  if (rev.empty()) {
    return "0";
  }
  return rev;
}

bool cmCTestP4::NoteOldRevision()
{
  this->OldRevision = this->GetWorkingRevision();

  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   Old revision of repository is: "
               << this->OldRevision << "\n");
  this->PriorRev.Rev = this->OldRevision;
  return true;
}

bool cmCTestP4::NoteNewRevision()
{
  this->NewRevision = this->GetWorkingRevision();

  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   New revision of repository is: "
               << this->NewRevision << "\n");
  return true;
}

bool cmCTestP4::LoadRevisions()
{
  std::vector<char const*> p4_changes;
  SetP4Options(p4_changes);

  // Use 'p4 changes ...@old,new' to get a list of changelists
  std::string range = this->SourceDirectory + "/...";

  // If any revision is unknown it means we couldn't contact the server.
  // Do not process updates
  if (this->OldRevision == "<unknown>" || this->NewRevision == "<unknown>") {
    cmCTestLog(this->CTest, HANDLER_OUTPUT, "   At least one of the revisions "
                 << "is unknown. No repository changes will be reported.\n");
    return false;
  }

  range.append("@")
    .append(this->OldRevision)
    .append(",")
    .append(this->NewRevision);

  p4_changes.push_back("changes");
  p4_changes.push_back(range.c_str());
  p4_changes.push_back(CM_NULLPTR);

  ChangesParser out(this, "p4_changes-out> ");
  OutputLogger err(this->Log, "p4_changes-err> ");

  ChangeLists.clear();
  this->RunChild(&p4_changes[0], &out, &err);

  if (ChangeLists.empty()) {
    return true;
  }

  // p4 describe -s ...@1111111,2222222
  std::vector<char const*> p4_describe;
  for (std::vector<std::string>::reverse_iterator i = ChangeLists.rbegin();
       i != ChangeLists.rend(); ++i) {
    SetP4Options(p4_describe);
    p4_describe.push_back("describe");
    p4_describe.push_back("-s");
    p4_describe.push_back(i->c_str());
    p4_describe.push_back(CM_NULLPTR);

    DescribeParser outDescribe(this, "p4_describe-out> ");
    OutputLogger errDescribe(this->Log, "p4_describe-err> ");
    this->RunChild(&p4_describe[0], &outDescribe, &errDescribe);
  }
  return true;
}

bool cmCTestP4::LoadModifications()
{
  std::vector<char const*> p4_diff;
  SetP4Options(p4_diff);

  p4_diff.push_back("diff");

  // Ideally we would use -Od but not all clients support it
  p4_diff.push_back("-dn");
  std::string source = this->SourceDirectory + "/...";
  p4_diff.push_back(source.c_str());
  p4_diff.push_back(CM_NULLPTR);

  DiffParser out(this, "p4_diff-out> ");
  OutputLogger err(this->Log, "p4_diff-err> ");
  this->RunChild(&p4_diff[0], &out, &err);
  return true;
}

bool cmCTestP4::UpdateCustom(const std::string& custom)
{
  std::vector<std::string> p4_custom_command;
  cmSystemTools::ExpandListArgument(custom, p4_custom_command, true);

  std::vector<char const*> p4_custom;
  for (std::vector<std::string>::const_iterator i = p4_custom_command.begin();
       i != p4_custom_command.end(); ++i) {
    p4_custom.push_back(i->c_str());
  }
  p4_custom.push_back(CM_NULLPTR);

  OutputLogger custom_out(this->Log, "p4_customsync-out> ");
  OutputLogger custom_err(this->Log, "p4_customsync-err> ");

  return this->RunUpdateCommand(&p4_custom[0], &custom_out, &custom_err);
}

bool cmCTestP4::UpdateImpl()
{
  std::string custom = this->CTest->GetCTestConfiguration("P4UpdateCustom");
  if (!custom.empty()) {
    return this->UpdateCustom(custom);
  }

  // If we couldn't get a revision number before updating, abort.
  if (this->OldRevision == "<unknown>") {
    this->UpdateCommandLine = "Unknown current revision";
    cmCTestLog(this->CTest, ERROR_MESSAGE, "   Unknown current revision\n");
    return false;
  }

  std::vector<char const*> p4_sync;
  SetP4Options(p4_sync);

  p4_sync.push_back("sync");

  // Get user-specified update options.
  std::string opts = this->CTest->GetCTestConfiguration("UpdateOptions");
  if (opts.empty()) {
    opts = this->CTest->GetCTestConfiguration("P4UpdateOptions");
  }
  std::vector<std::string> args = cmSystemTools::ParseArguments(opts.c_str());
  for (std::vector<std::string>::const_iterator ai = args.begin();
       ai != args.end(); ++ai) {
    p4_sync.push_back(ai->c_str());
  }

  std::string source = this->SourceDirectory + "/...";

  // Specify the start time for nightly testing.
  if (this->CTest->GetTestModel() == cmCTest::NIGHTLY) {
    std::string date = this->GetNightlyTime();
    // CTest reports the date as YYYY-MM-DD, Perforce needs it as YYYY/MM/DD
    std::replace(date.begin(), date.end(), '-', '/');

    // Revision specification: /...@"YYYY/MM/DD HH:MM:SS"
    source.append("@\"").append(date).append("\"");
  }

  p4_sync.push_back(source.c_str());
  p4_sync.push_back(CM_NULLPTR);

  OutputLogger out(this->Log, "p4_sync-out> ");
  OutputLogger err(this->Log, "p4_sync-err> ");

  return this->RunUpdateCommand(&p4_sync[0], &out, &err);
}
