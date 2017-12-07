/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestGIT.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Process.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#include "cmAlgorithms.h"
#include "cmCTest.h"
#include "cmCTestVC.h"
#include "cmProcessOutput.h"
#include "cmProcessTools.h"
#include "cmSystemTools.h"

static unsigned int cmCTestGITVersion(unsigned int epic, unsigned int major,
                                      unsigned int minor, unsigned int fix)
{
  // 1.6.5.0 maps to 10605000
  return fix + minor * 1000 + major * 100000 + epic * 10000000;
}

cmCTestGIT::cmCTestGIT(cmCTest* ct, std::ostream& log)
  : cmCTestGlobalVC(ct, log)
{
  this->PriorRev = this->Unknown;
  this->CurrentGitVersion = 0;
}

cmCTestGIT::~cmCTestGIT()
{
}

class cmCTestGIT::OneLineParser : public cmCTestVC::LineParser
{
public:
  OneLineParser(cmCTestGIT* git, const char* prefix, std::string& l)
    : Line1(l)
  {
    this->SetLog(&git->Log, prefix);
  }

private:
  std::string& Line1;
  bool ProcessLine() CM_OVERRIDE
  {
    // Only the first line is of interest.
    this->Line1 = this->Line;
    return false;
  }
};

std::string cmCTestGIT::GetWorkingRevision()
{
  // Run plumbing "git rev-list" to get work tree revision.
  const char* git = this->CommandLineTool.c_str();
  const char* git_rev_list[] = { git,    "rev-list", "-n",      "1",
                                 "HEAD", "--",       CM_NULLPTR };
  std::string rev;
  OneLineParser out(this, "rl-out> ", rev);
  OutputLogger err(this->Log, "rl-err> ");
  this->RunChild(git_rev_list, &out, &err);
  return rev;
}

bool cmCTestGIT::NoteOldRevision()
{
  this->OldRevision = this->GetWorkingRevision();
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   Old revision of repository is: "
               << this->OldRevision << "\n");
  this->PriorRev.Rev = this->OldRevision;
  return true;
}

bool cmCTestGIT::NoteNewRevision()
{
  this->NewRevision = this->GetWorkingRevision();
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "   New revision of repository is: "
               << this->NewRevision << "\n");
  return true;
}

std::string cmCTestGIT::FindGitDir()
{
  std::string git_dir;

  // Run "git rev-parse --git-dir" to locate the real .git directory.
  const char* git = this->CommandLineTool.c_str();
  char const* git_rev_parse[] = { git, "rev-parse", "--git-dir", CM_NULLPTR };
  std::string git_dir_line;
  OneLineParser rev_parse_out(this, "rev-parse-out> ", git_dir_line);
  OutputLogger rev_parse_err(this->Log, "rev-parse-err> ");
  if (this->RunChild(git_rev_parse, &rev_parse_out, &rev_parse_err, CM_NULLPTR,
                     cmProcessOutput::UTF8)) {
    git_dir = git_dir_line;
  }
  if (git_dir.empty()) {
    git_dir = ".git";
  }

  // Git reports a relative path only when the .git directory is in
  // the current directory.
  if (git_dir[0] == '.') {
    git_dir = this->SourceDirectory + "/" + git_dir;
  }
#if defined(_WIN32) && !defined(__CYGWIN__)
  else if (git_dir[0] == '/') {
    // Cygwin Git reports a full path that Cygwin understands, but we
    // are a Windows application.  Run "cygpath" to get Windows path.
    std::string cygpath_exe = cmSystemTools::GetFilenamePath(git);
    cygpath_exe += "/cygpath.exe";
    if (cmSystemTools::FileExists(cygpath_exe.c_str())) {
      char const* cygpath[] = { cygpath_exe.c_str(), "-w", git_dir.c_str(),
                                0 };
      OneLineParser cygpath_out(this, "cygpath-out> ", git_dir_line);
      OutputLogger cygpath_err(this->Log, "cygpath-err> ");
      if (this->RunChild(cygpath, &cygpath_out, &cygpath_err, CM_NULLPTR,
                         cmProcessOutput::UTF8)) {
        git_dir = git_dir_line;
      }
    }
  }
#endif
  return git_dir;
}

std::string cmCTestGIT::FindTopDir()
{
  std::string top_dir = this->SourceDirectory;

  // Run "git rev-parse --show-cdup" to locate the top of the tree.
  const char* git = this->CommandLineTool.c_str();
  char const* git_rev_parse[] = { git, "rev-parse", "--show-cdup",
                                  CM_NULLPTR };
  std::string cdup;
  OneLineParser rev_parse_out(this, "rev-parse-out> ", cdup);
  OutputLogger rev_parse_err(this->Log, "rev-parse-err> ");
  if (this->RunChild(git_rev_parse, &rev_parse_out, &rev_parse_err, CM_NULLPTR,
                     cmProcessOutput::UTF8) &&
      !cdup.empty()) {
    top_dir += "/";
    top_dir += cdup;
    top_dir = cmSystemTools::CollapseFullPath(top_dir);
  }
  return top_dir;
}

bool cmCTestGIT::UpdateByFetchAndReset()
{
  const char* git = this->CommandLineTool.c_str();

  // Use "git fetch" to get remote commits.
  std::vector<char const*> git_fetch;
  git_fetch.push_back(git);
  git_fetch.push_back("fetch");

  // Add user-specified update options.
  std::string opts = this->CTest->GetCTestConfiguration("UpdateOptions");
  if (opts.empty()) {
    opts = this->CTest->GetCTestConfiguration("GITUpdateOptions");
  }
  std::vector<std::string> args = cmSystemTools::ParseArguments(opts.c_str());
  for (std::vector<std::string>::const_iterator ai = args.begin();
       ai != args.end(); ++ai) {
    git_fetch.push_back(ai->c_str());
  }

  // Sentinel argument.
  git_fetch.push_back(CM_NULLPTR);

  // Fetch upstream refs.
  OutputLogger fetch_out(this->Log, "fetch-out> ");
  OutputLogger fetch_err(this->Log, "fetch-err> ");
  if (!this->RunUpdateCommand(&git_fetch[0], &fetch_out, &fetch_err)) {
    return false;
  }

  // Identify the merge head that would be used by "git pull".
  std::string sha1;
  {
    std::string fetch_head = this->FindGitDir() + "/FETCH_HEAD";
    cmsys::ifstream fin(fetch_head.c_str(), std::ios::in | std::ios::binary);
    if (!fin) {
      this->Log << "Unable to open " << fetch_head << "\n";
      return false;
    }
    std::string line;
    while (sha1.empty() && cmSystemTools::GetLineFromStream(fin, line)) {
      this->Log << "FETCH_HEAD> " << line << "\n";
      if (line.find("\tnot-for-merge\t") == std::string::npos) {
        std::string::size_type pos = line.find('\t');
        if (pos != std::string::npos) {
          sha1 = line.substr(0, pos);
        }
      }
    }
    if (sha1.empty()) {
      this->Log << "FETCH_HEAD has no upstream branch candidate!\n";
      return false;
    }
  }

  // Reset the local branch to point at that tracked from upstream.
  char const* git_reset[] = { git, "reset", "--hard", sha1.c_str(),
                              CM_NULLPTR };
  OutputLogger reset_out(this->Log, "reset-out> ");
  OutputLogger reset_err(this->Log, "reset-err> ");
  return this->RunChild(&git_reset[0], &reset_out, &reset_err);
}

bool cmCTestGIT::UpdateByCustom(std::string const& custom)
{
  std::vector<std::string> git_custom_command;
  cmSystemTools::ExpandListArgument(custom, git_custom_command, true);
  std::vector<char const*> git_custom;
  for (std::vector<std::string>::const_iterator i = git_custom_command.begin();
       i != git_custom_command.end(); ++i) {
    git_custom.push_back(i->c_str());
  }
  git_custom.push_back(CM_NULLPTR);

  OutputLogger custom_out(this->Log, "custom-out> ");
  OutputLogger custom_err(this->Log, "custom-err> ");
  return this->RunUpdateCommand(&git_custom[0], &custom_out, &custom_err);
}

bool cmCTestGIT::UpdateInternal()
{
  std::string custom = this->CTest->GetCTestConfiguration("GITUpdateCustom");
  if (!custom.empty()) {
    return this->UpdateByCustom(custom);
  }
  return this->UpdateByFetchAndReset();
}

bool cmCTestGIT::UpdateImpl()
{
  if (!this->UpdateInternal()) {
    return false;
  }

  std::string top_dir = this->FindTopDir();
  const char* git = this->CommandLineTool.c_str();
  const char* recursive = "--recursive";
  const char* sync_recursive = "--recursive";

  // Git < 1.6.5 did not support submodule --recursive
  if (this->GetGitVersion() < cmCTestGITVersion(1, 6, 5, 0)) {
    recursive = CM_NULLPTR;
    // No need to require >= 1.6.5 if there are no submodules.
    if (cmSystemTools::FileExists((top_dir + "/.gitmodules").c_str())) {
      this->Log << "Git < 1.6.5 cannot update submodules recursively\n";
    }
  }

  // Git < 1.8.1 did not support sync --recursive
  if (this->GetGitVersion() < cmCTestGITVersion(1, 8, 1, 0)) {
    sync_recursive = CM_NULLPTR;
    // No need to require >= 1.8.1 if there are no submodules.
    if (cmSystemTools::FileExists((top_dir + "/.gitmodules").c_str())) {
      this->Log << "Git < 1.8.1 cannot synchronize submodules recursively\n";
    }
  }

  OutputLogger submodule_out(this->Log, "submodule-out> ");
  OutputLogger submodule_err(this->Log, "submodule-err> ");

  bool ret;

  std::string init_submodules =
    this->CTest->GetCTestConfiguration("GITInitSubmodules");
  if (cmSystemTools::IsOn(init_submodules.c_str())) {
    char const* git_submodule_init[] = { git, "submodule", "init",
                                         CM_NULLPTR };
    ret = this->RunChild(git_submodule_init, &submodule_out, &submodule_err,
                         top_dir.c_str());

    if (!ret) {
      return false;
    }
  }

  char const* git_submodule_sync[] = { git, "submodule", "sync",
                                       sync_recursive, CM_NULLPTR };
  ret = this->RunChild(git_submodule_sync, &submodule_out, &submodule_err,
                       top_dir.c_str());

  if (!ret) {
    return false;
  }

  char const* git_submodule[] = { git, "submodule", "update", recursive,
                                  CM_NULLPTR };
  return this->RunChild(git_submodule, &submodule_out, &submodule_err,
                        top_dir.c_str());
}

unsigned int cmCTestGIT::GetGitVersion()
{
  if (!this->CurrentGitVersion) {
    const char* git = this->CommandLineTool.c_str();
    char const* git_version[] = { git, "--version", CM_NULLPTR };
    std::string version;
    OneLineParser version_out(this, "version-out> ", version);
    OutputLogger version_err(this->Log, "version-err> ");
    unsigned int v[4] = { 0, 0, 0, 0 };
    if (this->RunChild(git_version, &version_out, &version_err) &&
        sscanf(version.c_str(), "git version %u.%u.%u.%u", &v[0], &v[1], &v[2],
               &v[3]) >= 3) {
      this->CurrentGitVersion = cmCTestGITVersion(v[0], v[1], v[2], v[3]);
    }
  }
  return this->CurrentGitVersion;
}

/* Diff format:

   :src-mode dst-mode src-sha1 dst-sha1 status\0
   src-path\0
   [dst-path\0]

   The format is repeated for every file changed.  The [dst-path\0]
   line appears only for lines with status 'C' or 'R'.  See 'git help
   diff-tree' for details.
*/
class cmCTestGIT::DiffParser : public cmCTestVC::LineParser
{
public:
  DiffParser(cmCTestGIT* git, const char* prefix)
    : LineParser('\0', false)
    , GIT(git)
    , DiffField(DiffFieldNone)
  {
    this->SetLog(&git->Log, prefix);
  }

  typedef cmCTestGIT::Change Change;
  std::vector<Change> Changes;

protected:
  cmCTestGIT* GIT;
  enum DiffFieldType
  {
    DiffFieldNone,
    DiffFieldChange,
    DiffFieldSrc,
    DiffFieldDst
  };
  DiffFieldType DiffField;
  Change CurChange;

  void DiffReset()
  {
    this->DiffField = DiffFieldNone;
    this->Changes.clear();
  }

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->Line[0] == ':') {
      this->DiffField = DiffFieldChange;
      this->CurChange = Change();
    }
    if (this->DiffField == DiffFieldChange) {
      // :src-mode dst-mode src-sha1 dst-sha1 status
      if (this->Line[0] != ':') {
        this->DiffField = DiffFieldNone;
        return true;
      }
      const char* src_mode_first = this->Line.c_str() + 1;
      const char* src_mode_last = this->ConsumeField(src_mode_first);
      const char* dst_mode_first = this->ConsumeSpace(src_mode_last);
      const char* dst_mode_last = this->ConsumeField(dst_mode_first);
      const char* src_sha1_first = this->ConsumeSpace(dst_mode_last);
      const char* src_sha1_last = this->ConsumeField(src_sha1_first);
      const char* dst_sha1_first = this->ConsumeSpace(src_sha1_last);
      const char* dst_sha1_last = this->ConsumeField(dst_sha1_first);
      const char* status_first = this->ConsumeSpace(dst_sha1_last);
      const char* status_last = this->ConsumeField(status_first);
      if (status_first != status_last) {
        this->CurChange.Action = *status_first;
        this->DiffField = DiffFieldSrc;
      } else {
        this->DiffField = DiffFieldNone;
      }
    } else if (this->DiffField == DiffFieldSrc) {
      // src-path
      if (this->CurChange.Action == 'C') {
        // Convert copy to addition of destination.
        this->CurChange.Action = 'A';
        this->DiffField = DiffFieldDst;
      } else if (this->CurChange.Action == 'R') {
        // Convert rename to deletion of source and addition of destination.
        this->CurChange.Action = 'D';
        this->CurChange.Path = this->Line;
        this->Changes.push_back(this->CurChange);

        this->CurChange = Change('A');
        this->DiffField = DiffFieldDst;
      } else {
        this->CurChange.Path = this->Line;
        this->Changes.push_back(this->CurChange);
        this->DiffField = this->DiffFieldNone;
      }
    } else if (this->DiffField == DiffFieldDst) {
      // dst-path
      this->CurChange.Path = this->Line;
      this->Changes.push_back(this->CurChange);
      this->DiffField = this->DiffFieldNone;
    }
    return true;
  }

  const char* ConsumeSpace(const char* c)
  {
    while (*c && isspace(*c)) {
      ++c;
    }
    return c;
  }
  const char* ConsumeField(const char* c)
  {
    while (*c && !isspace(*c)) {
      ++c;
    }
    return c;
  }
};

/* Commit format:

   commit ...\n
   tree ...\n
   parent ...\n
   author ...\n
   committer ...\n
   \n
       Log message indented by (4) spaces\n
       (even blank lines have the spaces)\n
 [[
   \n
   [Diff format]
 OR
   \0
 ]]

   The header may have more fields.  See 'git help diff-tree'.
*/
class cmCTestGIT::CommitParser : public cmCTestGIT::DiffParser
{
public:
  CommitParser(cmCTestGIT* git, const char* prefix)
    : DiffParser(git, prefix)
    , Section(SectionHeader)
  {
    this->Separator = SectionSep[this->Section];
  }

private:
  typedef cmCTestGIT::Revision Revision;
  enum SectionType
  {
    SectionHeader,
    SectionBody,
    SectionDiff,
    SectionCount
  };
  static char const SectionSep[SectionCount];
  SectionType Section;
  Revision Rev;

  struct Person
  {
    std::string Name;
    std::string EMail;
    unsigned long Time;
    long TimeZone;
    Person()
      : Name()
      , EMail()
      , Time(0)
      , TimeZone(0)
    {
    }
  };

  void ParsePerson(const char* str, Person& person)
  {
    // Person Name <person@domain.com> 1234567890 +0000
    const char* c = str;
    while (*c && isspace(*c)) {
      ++c;
    }

    const char* name_first = c;
    while (*c && *c != '<') {
      ++c;
    }
    const char* name_last = c;
    while (name_last != name_first && isspace(*(name_last - 1))) {
      --name_last;
    }
    person.Name.assign(name_first, name_last - name_first);

    const char* email_first = *c ? ++c : c;
    while (*c && *c != '>') {
      ++c;
    }
    const char* email_last = *c ? c++ : c;
    person.EMail.assign(email_first, email_last - email_first);

    person.Time = strtoul(c, (char**)&c, 10);
    person.TimeZone = strtol(c, (char**)&c, 10);
  }

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->Line.empty()) {
      if (this->Section == SectionBody && this->LineEnd == '\0') {
        // Skip SectionDiff
        this->NextSection();
      }
      this->NextSection();
    } else {
      switch (this->Section) {
        case SectionHeader:
          this->DoHeaderLine();
          break;
        case SectionBody:
          this->DoBodyLine();
          break;
        case SectionDiff:
          this->DiffParser::ProcessLine();
          break;
        case SectionCount:
          break; // never happens
      }
    }
    return true;
  }

  void NextSection()
  {
    this->Section = SectionType((this->Section + 1) % SectionCount);
    this->Separator = SectionSep[this->Section];
    if (this->Section == SectionHeader) {
      this->GIT->DoRevision(this->Rev, this->Changes);
      this->Rev = Revision();
      this->DiffReset();
    }
  }

  void DoHeaderLine()
  {
    // Look for header fields that we need.
    if (cmHasLiteralPrefix(this->Line.c_str(), "commit ")) {
      this->Rev.Rev = this->Line.c_str() + 7;
    } else if (cmHasLiteralPrefix(this->Line.c_str(), "author ")) {
      Person author;
      this->ParsePerson(this->Line.c_str() + 7, author);
      this->Rev.Author = author.Name;
      this->Rev.EMail = author.EMail;
      this->Rev.Date = this->FormatDateTime(author);
    } else if (cmHasLiteralPrefix(this->Line.c_str(), "committer ")) {
      Person committer;
      this->ParsePerson(this->Line.c_str() + 10, committer);
      this->Rev.Committer = committer.Name;
      this->Rev.CommitterEMail = committer.EMail;
      this->Rev.CommitDate = this->FormatDateTime(committer);
    }
  }

  void DoBodyLine()
  {
    // Commit log lines are indented by 4 spaces.
    if (this->Line.size() >= 4) {
      this->Rev.Log += this->Line.substr(4);
    }
    this->Rev.Log += "\n";
  }

  std::string FormatDateTime(Person const& person)
  {
    // Convert the time to a human-readable format that is also easy
    // to machine-parse: "CCYY-MM-DD hh:mm:ss".
    time_t seconds = static_cast<time_t>(person.Time);
    struct tm* t = gmtime(&seconds);
    char dt[1024];
    sprintf(dt, "%04d-%02d-%02d %02d:%02d:%02d", t->tm_year + 1900,
            t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    std::string out = dt;

    // Add the time-zone field "+zone" or "-zone".
    char tz[32];
    if (person.TimeZone >= 0) {
      sprintf(tz, " +%04ld", person.TimeZone);
    } else {
      sprintf(tz, " -%04ld", -person.TimeZone);
    }
    out += tz;
    return out;
  }
};

char const cmCTestGIT::CommitParser::SectionSep[SectionCount] = { '\n', '\n',
                                                                  '\0' };

bool cmCTestGIT::LoadRevisions()
{
  // Use 'git rev-list ... | git diff-tree ...' to get revisions.
  std::string range = this->OldRevision + ".." + this->NewRevision;
  const char* git = this->CommandLineTool.c_str();
  const char* git_rev_list[] = { git,           "rev-list", "--reverse",
                                 range.c_str(), "--",       CM_NULLPTR };
  const char* git_diff_tree[] = {
    git,  "diff-tree",    "--stdin",          "--always", "-z",
    "-r", "--pretty=raw", "--encoding=utf-8", CM_NULLPTR
  };
  this->Log << this->ComputeCommandLine(git_rev_list) << " | "
            << this->ComputeCommandLine(git_diff_tree) << "\n";

  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_AddCommand(cp, git_rev_list);
  cmsysProcess_AddCommand(cp, git_diff_tree);
  cmsysProcess_SetWorkingDirectory(cp, this->SourceDirectory.c_str());

  CommitParser out(this, "dt-out> ");
  OutputLogger err(this->Log, "dt-err> ");
  this->RunProcess(cp, &out, &err, cmProcessOutput::UTF8);

  // Send one extra zero-byte to terminate the last record.
  out.Process("", 1);

  cmsysProcess_Delete(cp);
  return true;
}

bool cmCTestGIT::LoadModifications()
{
  const char* git = this->CommandLineTool.c_str();

  // Use 'git update-index' to refresh the index w.r.t. the work tree.
  const char* git_update_index[] = { git, "update-index", "--refresh",
                                     CM_NULLPTR };
  OutputLogger ui_out(this->Log, "ui-out> ");
  OutputLogger ui_err(this->Log, "ui-err> ");
  this->RunChild(git_update_index, &ui_out, &ui_err, CM_NULLPTR,
                 cmProcessOutput::UTF8);

  // Use 'git diff-index' to get modified files.
  const char* git_diff_index[] = { git,    "diff-index", "-z",
                                   "HEAD", "--",         CM_NULLPTR };
  DiffParser out(this, "di-out> ");
  OutputLogger err(this->Log, "di-err> ");
  this->RunChild(git_diff_index, &out, &err, CM_NULLPTR,
                 cmProcessOutput::UTF8);

  for (std::vector<Change>::const_iterator ci = out.Changes.begin();
       ci != out.Changes.end(); ++ci) {
    this->DoModification(PathModified, ci->Path);
  }
  return true;
}
