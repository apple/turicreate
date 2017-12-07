/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestSVN.h"

#include "cmCTest.h"
#include "cmCTestVC.h"
#include "cmProcessTools.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"
#include "cmXMLWriter.h"

#include "cmsys/RegularExpression.hxx"
#include <map>
#include <ostream>
#include <stdlib.h>
#include <string.h>

struct cmCTestSVN::Revision : public cmCTestVC::Revision
{
  cmCTestSVN::SVNInfo* SVNInfo;
};

cmCTestSVN::cmCTestSVN(cmCTest* ct, std::ostream& log)
  : cmCTestGlobalVC(ct, log)
{
  this->PriorRev = this->Unknown;
}

cmCTestSVN::~cmCTestSVN()
{
}

void cmCTestSVN::CleanupImpl()
{
  std::vector<const char*> svn_cleanup;
  svn_cleanup.push_back("cleanup");
  OutputLogger out(this->Log, "cleanup-out> ");
  OutputLogger err(this->Log, "cleanup-err> ");
  this->RunSVNCommand(svn_cleanup, &out, &err);
}

class cmCTestSVN::InfoParser : public cmCTestVC::LineParser
{
public:
  InfoParser(cmCTestSVN* svn, const char* prefix, std::string& rev,
             SVNInfo& svninfo)
    : Rev(rev)
    , SVNRepo(svninfo)
  {
    this->SetLog(&svn->Log, prefix);
    this->RegexRev.compile("^Revision: ([0-9]+)");
    this->RegexURL.compile("^URL: +([^ ]+) *$");
    this->RegexRoot.compile("^Repository Root: +([^ ]+) *$");
  }

private:
  std::string& Rev;
  cmCTestSVN::SVNInfo& SVNRepo;
  cmsys::RegularExpression RegexRev;
  cmsys::RegularExpression RegexURL;
  cmsys::RegularExpression RegexRoot;
  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexRev.find(this->Line)) {
      this->Rev = this->RegexRev.match(1);
    } else if (this->RegexURL.find(this->Line)) {
      this->SVNRepo.URL = this->RegexURL.match(1);
    } else if (this->RegexRoot.find(this->Line)) {
      this->SVNRepo.Root = this->RegexRoot.match(1);
    }
    return true;
  }
};

static bool cmCTestSVNPathStarts(std::string const& p1, std::string const& p2)
{
  // Does path p1 start with path p2?
  if (p1.size() == p2.size()) {
    return p1 == p2;
  }
  if (p1.size() > p2.size() && p1[p2.size()] == '/') {
    return strncmp(p1.c_str(), p2.c_str(), p2.size()) == 0;
  }
  return false;
}

std::string cmCTestSVN::LoadInfo(SVNInfo& svninfo)
{
  // Run "svn info" to get the repository info from the work tree.
  std::vector<const char*> svn_info;
  svn_info.push_back("info");
  svn_info.push_back(svninfo.LocalPath.c_str());
  std::string rev;
  InfoParser out(this, "info-out> ", rev, svninfo);
  OutputLogger err(this->Log, "info-err> ");
  this->RunSVNCommand(svn_info, &out, &err);
  return rev;
}

bool cmCTestSVN::NoteOldRevision()
{
  if (!this->LoadRepositories()) {
    return false;
  }

  std::vector<SVNInfo>::iterator itbeg = this->Repositories.begin();
  std::vector<SVNInfo>::iterator itend = this->Repositories.end();
  for (; itbeg != itend; itbeg++) {
    SVNInfo& svninfo = *itbeg;
    svninfo.OldRevision = this->LoadInfo(svninfo);
    this->Log << "Revision for repository '" << svninfo.LocalPath
              << "' before update: " << svninfo.OldRevision << "\n";
    cmCTestLog(
      this->CTest, HANDLER_OUTPUT, "   Old revision of external repository '"
        << svninfo.LocalPath << "' is: " << svninfo.OldRevision << "\n");
  }

  // Set the global old revision to the one of the root
  this->OldRevision = this->RootInfo->OldRevision;
  this->PriorRev.Rev = this->OldRevision;
  return true;
}

bool cmCTestSVN::NoteNewRevision()
{
  if (!this->LoadRepositories()) {
    return false;
  }

  std::vector<SVNInfo>::iterator itbeg = this->Repositories.begin();
  std::vector<SVNInfo>::iterator itend = this->Repositories.end();
  for (; itbeg != itend; itbeg++) {
    SVNInfo& svninfo = *itbeg;
    svninfo.NewRevision = this->LoadInfo(svninfo);
    this->Log << "Revision for repository '" << svninfo.LocalPath
              << "' after update: " << svninfo.NewRevision << "\n";
    cmCTestLog(
      this->CTest, HANDLER_OUTPUT, "   New revision of external repository '"
        << svninfo.LocalPath << "' is: " << svninfo.NewRevision << "\n");

    // svninfo.Root = ""; // uncomment to test GuessBase
    this->Log << "Repository '" << svninfo.LocalPath
              << "' URL = " << svninfo.URL << "\n";
    this->Log << "Repository '" << svninfo.LocalPath
              << "' Root = " << svninfo.Root << "\n";

    // Compute the base path the working tree has checked out under
    // the repository root.
    if (!svninfo.Root.empty() &&
        cmCTestSVNPathStarts(svninfo.URL, svninfo.Root)) {
      svninfo.Base =
        cmCTest::DecodeURL(svninfo.URL.substr(svninfo.Root.size()));
      svninfo.Base += "/";
    }
    this->Log << "Repository '" << svninfo.LocalPath
              << "' Base = " << svninfo.Base << "\n";
  }

  // Set the global new revision to the one of the root
  this->NewRevision = this->RootInfo->NewRevision;
  return true;
}

void cmCTestSVN::GuessBase(SVNInfo& svninfo,
                           std::vector<Change> const& changes)
{
  // Subversion did not give us a good repository root so we need to
  // guess the base path from the URL and the paths in a revision with
  // changes under it.

  // Consider each possible URL suffix from longest to shortest.
  for (std::string::size_type slash = svninfo.URL.find('/');
       svninfo.Base.empty() && slash != std::string::npos;
       slash = svninfo.URL.find('/', slash + 1)) {
    // If the URL suffix is a prefix of at least one path then it is the base.
    std::string base = cmCTest::DecodeURL(svninfo.URL.substr(slash));
    for (std::vector<Change>::const_iterator ci = changes.begin();
         svninfo.Base.empty() && ci != changes.end(); ++ci) {
      if (cmCTestSVNPathStarts(ci->Path, base)) {
        svninfo.Base = base;
      }
    }
  }

  // We always append a slash so that we know paths beginning in the
  // base lie under its path.  If no base was found then the working
  // tree must be a checkout of the entire repo and this will match
  // the leading slash in all paths.
  svninfo.Base += "/";

  this->Log << "Guessed Base = " << svninfo.Base << "\n";
}

class cmCTestSVN::UpdateParser : public cmCTestVC::LineParser
{
public:
  UpdateParser(cmCTestSVN* svn, const char* prefix)
    : SVN(svn)
  {
    this->SetLog(&svn->Log, prefix);
    this->RegexUpdate.compile("^([ADUCGE ])([ADUCGE ])[B ] +(.+)$");
  }

private:
  cmCTestSVN* SVN;
  cmsys::RegularExpression RegexUpdate;

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexUpdate.find(this->Line)) {
      this->DoPath(this->RegexUpdate.match(1)[0],
                   this->RegexUpdate.match(2)[0], this->RegexUpdate.match(3));
    }
    return true;
  }

  void DoPath(char path_status, char prop_status, std::string const& path)
  {
    char status = (path_status != ' ') ? path_status : prop_status;
    std::string dir = cmSystemTools::GetFilenamePath(path);
    std::string name = cmSystemTools::GetFilenameName(path);
    // See "svn help update".
    switch (status) {
      case 'G':
        this->SVN->Dirs[dir][name].Status = PathModified;
        break;
      case 'C':
        this->SVN->Dirs[dir][name].Status = PathConflicting;
        break;
      case 'A':
      case 'D':
      case 'U':
        this->SVN->Dirs[dir][name].Status = PathUpdated;
        break;
      case 'E': // TODO?
      case '?':
      case ' ':
      default:
        break;
    }
  }
};

bool cmCTestSVN::UpdateImpl()
{
  // Get user-specified update options.
  std::string opts = this->CTest->GetCTestConfiguration("UpdateOptions");
  if (opts.empty()) {
    opts = this->CTest->GetCTestConfiguration("SVNUpdateOptions");
  }
  std::vector<std::string> args = cmSystemTools::ParseArguments(opts.c_str());

  // Specify the start time for nightly testing.
  if (this->CTest->GetTestModel() == cmCTest::NIGHTLY) {
    args.push_back("-r{" + this->GetNightlyTime() + " +0000}");
  }

  std::vector<char const*> svn_update;
  svn_update.push_back("update");
  for (std::vector<std::string>::const_iterator ai = args.begin();
       ai != args.end(); ++ai) {
    svn_update.push_back(ai->c_str());
  }

  UpdateParser out(this, "up-out> ");
  OutputLogger err(this->Log, "up-err> ");
  return this->RunSVNCommand(svn_update, &out, &err);
}

bool cmCTestSVN::RunSVNCommand(std::vector<char const*> const& parameters,
                               OutputParser* out, OutputParser* err)
{
  if (parameters.empty()) {
    return false;
  }

  std::vector<char const*> args;
  args.push_back(this->CommandLineTool.c_str());

  args.insert(args.end(), parameters.begin(), parameters.end());

  args.push_back("--non-interactive");

  std::string userOptions = this->CTest->GetCTestConfiguration("SVNOptions");

  std::vector<std::string> parsedUserOptions =
    cmSystemTools::ParseArguments(userOptions.c_str());
  for (std::vector<std::string>::iterator i = parsedUserOptions.begin();
       i != parsedUserOptions.end(); ++i) {
    args.push_back(i->c_str());
  }

  args.push_back(CM_NULLPTR);

  if (strcmp(parameters[0], "update") == 0) {
    return RunUpdateCommand(&args[0], out, err);
  }
  return RunChild(&args[0], out, err);
}

class cmCTestSVN::LogParser : public cmCTestVC::OutputLogger,
                              private cmXMLParser
{
public:
  LogParser(cmCTestSVN* svn, const char* prefix, SVNInfo& svninfo)
    : OutputLogger(svn->Log, prefix)
    , SVN(svn)
    , SVNRepo(svninfo)
  {
    this->InitializeParser();
  }
  ~LogParser() CM_OVERRIDE { this->CleanupParser(); }
private:
  cmCTestSVN* SVN;
  cmCTestSVN::SVNInfo& SVNRepo;

  typedef cmCTestSVN::Revision Revision;
  typedef cmCTestSVN::Change Change;
  Revision Rev;
  std::vector<Change> Changes;
  Change CurChange;
  std::vector<char> CData;

  bool ProcessChunk(const char* data, int length) CM_OVERRIDE
  {
    this->OutputLogger::ProcessChunk(data, length);
    this->ParseChunk(data, length);
    return true;
  }

  void StartElement(const std::string& name, const char** atts) CM_OVERRIDE
  {
    this->CData.clear();
    if (name == "logentry") {
      this->Rev = Revision();
      this->Rev.SVNInfo = &SVNRepo;
      if (const char* rev = this->FindAttribute(atts, "revision")) {
        this->Rev.Rev = rev;
      }
      this->Changes.clear();
    } else if (name == "path") {
      this->CurChange = Change();
      if (const char* action = this->FindAttribute(atts, "action")) {
        this->CurChange.Action = action[0];
      }
    }
  }

  void CharacterDataHandler(const char* data, int length) CM_OVERRIDE
  {
    this->CData.insert(this->CData.end(), data, data + length);
  }

  void EndElement(const std::string& name) CM_OVERRIDE
  {
    if (name == "logentry") {
      this->SVN->DoRevisionSVN(this->Rev, this->Changes);
    } else if (!this->CData.empty() && name == "path") {
      std::string orig_path(&this->CData[0], this->CData.size());
      std::string new_path = SVNRepo.BuildLocalPath(orig_path);
      this->CurChange.Path.assign(new_path);
      this->Changes.push_back(this->CurChange);
    } else if (!this->CData.empty() && name == "author") {
      this->Rev.Author.assign(&this->CData[0], this->CData.size());
    } else if (!this->CData.empty() && name == "date") {
      this->Rev.Date.assign(&this->CData[0], this->CData.size());
    } else if (!this->CData.empty() && name == "msg") {
      this->Rev.Log.assign(&this->CData[0], this->CData.size());
    }
    this->CData.clear();
  }

  void ReportError(int /*line*/, int /*column*/, const char* msg) CM_OVERRIDE
  {
    this->SVN->Log << "Error parsing svn log xml: " << msg << "\n";
  }
};

bool cmCTestSVN::LoadRevisions()
{
  bool result = true;
  // Get revisions for all the external repositories
  std::vector<SVNInfo>::iterator itbeg = this->Repositories.begin();
  std::vector<SVNInfo>::iterator itend = this->Repositories.end();
  for (; itbeg != itend; itbeg++) {
    SVNInfo& svninfo = *itbeg;
    result = this->LoadRevisions(svninfo) && result;
  }
  return result;
}

bool cmCTestSVN::LoadRevisions(SVNInfo& svninfo)
{
  // We are interested in every revision included in the update.
  std::string revs;
  if (atoi(svninfo.OldRevision.c_str()) < atoi(svninfo.NewRevision.c_str())) {
    revs = "-r" + svninfo.OldRevision + ":" + svninfo.NewRevision;
  } else {
    revs = "-r" + svninfo.NewRevision;
  }

  // Run "svn log" to get all global revisions of interest.
  std::vector<const char*> svn_log;
  svn_log.push_back("log");
  svn_log.push_back("--xml");
  svn_log.push_back("-v");
  svn_log.push_back(revs.c_str());
  svn_log.push_back(svninfo.LocalPath.c_str());
  LogParser out(this, "log-out> ", svninfo);
  OutputLogger err(this->Log, "log-err> ");
  return this->RunSVNCommand(svn_log, &out, &err);
}

void cmCTestSVN::DoRevisionSVN(Revision const& revision,
                               std::vector<Change> const& changes)
{
  // Guess the base checkout path from the changes if necessary.
  if (this->RootInfo->Base.empty() && !changes.empty()) {
    this->GuessBase(*this->RootInfo, changes);
  }

  // Ignore changes in the old revision for external repositories
  if (revision.Rev == revision.SVNInfo->OldRevision &&
      revision.SVNInfo->LocalPath != "") {
    return;
  }

  this->cmCTestGlobalVC::DoRevision(revision, changes);
}

class cmCTestSVN::StatusParser : public cmCTestVC::LineParser
{
public:
  StatusParser(cmCTestSVN* svn, const char* prefix)
    : SVN(svn)
  {
    this->SetLog(&svn->Log, prefix);
    this->RegexStatus.compile("^([ACDIMRX?!~ ])([CM ])[ L]... +(.+)$");
  }

private:
  cmCTestSVN* SVN;
  cmsys::RegularExpression RegexStatus;
  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexStatus.find(this->Line)) {
      this->DoPath(this->RegexStatus.match(1)[0],
                   this->RegexStatus.match(2)[0], this->RegexStatus.match(3));
    }
    return true;
  }

  void DoPath(char path_status, char prop_status, std::string const& path)
  {
    char status = (path_status != ' ') ? path_status : prop_status;
    // See "svn help status".
    switch (status) {
      case 'M':
      case '!':
      case 'A':
      case 'D':
      case 'R':
        this->SVN->DoModification(PathModified, path);
        break;
      case 'C':
      case '~':
        this->SVN->DoModification(PathConflicting, path);
        break;
      case 'X':
      case 'I':
      case '?':
      case ' ':
      default:
        break;
    }
  }
};

bool cmCTestSVN::LoadModifications()
{
  // Run "svn status" which reports local modifications.
  std::vector<const char*> svn_status;
  svn_status.push_back("status");
  StatusParser out(this, "status-out> ");
  OutputLogger err(this->Log, "status-err> ");
  this->RunSVNCommand(svn_status, &out, &err);
  return true;
}

void cmCTestSVN::WriteXMLGlobal(cmXMLWriter& xml)
{
  this->cmCTestGlobalVC::WriteXMLGlobal(xml);

  xml.Element("SVNPath", this->RootInfo->Base);
}

class cmCTestSVN::ExternalParser : public cmCTestVC::LineParser
{
public:
  ExternalParser(cmCTestSVN* svn, const char* prefix)
    : SVN(svn)
  {
    this->SetLog(&svn->Log, prefix);
    this->RegexExternal.compile("^X..... +(.+)$");
  }

private:
  cmCTestSVN* SVN;
  cmsys::RegularExpression RegexExternal;
  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexExternal.find(this->Line)) {
      this->DoPath(this->RegexExternal.match(1));
    }
    return true;
  }

  void DoPath(std::string const& path)
  {
    // Get local path relative to the source directory
    std::string local_path;
    if (path.size() > this->SVN->SourceDirectory.size() &&
        strncmp(path.c_str(), this->SVN->SourceDirectory.c_str(),
                this->SVN->SourceDirectory.size()) == 0) {
      local_path = path.c_str() + this->SVN->SourceDirectory.size() + 1;
    } else {
      local_path = path;
    }
    this->SVN->Repositories.push_back(SVNInfo(local_path.c_str()));
  }
};

bool cmCTestSVN::LoadRepositories()
{
  if (!this->Repositories.empty()) {
    return true;
  }

  // Info for root repository
  this->Repositories.push_back(SVNInfo(""));
  this->RootInfo = &(this->Repositories.back());

  // Run "svn status" to get the list of external repositories
  std::vector<const char*> svn_status;
  svn_status.push_back("status");
  ExternalParser out(this, "external-out> ");
  OutputLogger err(this->Log, "external-err> ");
  return this->RunSVNCommand(svn_status, &out, &err);
}

std::string cmCTestSVN::SVNInfo::BuildLocalPath(std::string const& path) const
{
  std::string local_path;

  // Add local path prefix if not empty
  if (!this->LocalPath.empty()) {
    local_path += this->LocalPath;
    local_path += "/";
  }

  // Add path with base prefix removed
  if (path.size() > this->Base.size() &&
      strncmp(path.c_str(), this->Base.c_str(), this->Base.size()) == 0) {
    local_path += (path.c_str() + this->Base.size());
  } else {
    local_path += path;
  }

  return local_path;
}
