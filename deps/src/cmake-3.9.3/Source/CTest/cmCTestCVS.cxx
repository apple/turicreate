/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestCVS.h"

#include "cmCTest.h"
#include "cmProcessTools.h"
#include "cmSystemTools.h"
#include "cmXMLWriter.h"

#include "cmsys/FStream.hxx"
#include "cmsys/RegularExpression.hxx"
#include <utility>

cmCTestCVS::cmCTestCVS(cmCTest* ct, std::ostream& log)
  : cmCTestVC(ct, log)
{
}

cmCTestCVS::~cmCTestCVS()
{
}

class cmCTestCVS::UpdateParser : public cmCTestVC::LineParser
{
public:
  UpdateParser(cmCTestCVS* cvs, const char* prefix)
    : CVS(cvs)
  {
    this->SetLog(&cvs->Log, prefix);
    // See "man cvs", section "update output".
    this->RegexFileUpdated.compile("^([UP])  *(.*)");
    this->RegexFileModified.compile("^([MRA])  *(.*)");
    this->RegexFileConflicting.compile("^([C])  *(.*)");
    this->RegexFileRemoved1.compile(
      "cvs[^ ]* update: `?([^']*)'? is no longer in the repository");
    this->RegexFileRemoved2.compile(
      "cvs[^ ]* update: "
      "warning: `?([^']*)'? is not \\(any longer\\) pertinent");
  }

private:
  cmCTestCVS* CVS;
  cmsys::RegularExpression RegexFileUpdated;
  cmsys::RegularExpression RegexFileModified;
  cmsys::RegularExpression RegexFileConflicting;
  cmsys::RegularExpression RegexFileRemoved1;
  cmsys::RegularExpression RegexFileRemoved2;

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->RegexFileUpdated.find(this->Line)) {
      this->DoFile(PathUpdated, this->RegexFileUpdated.match(2));
    } else if (this->RegexFileModified.find(this->Line)) {
      this->DoFile(PathModified, this->RegexFileModified.match(2));
    } else if (this->RegexFileConflicting.find(this->Line)) {
      this->DoFile(PathConflicting, this->RegexFileConflicting.match(2));
    } else if (this->RegexFileRemoved1.find(this->Line)) {
      this->DoFile(PathUpdated, this->RegexFileRemoved1.match(1));
    } else if (this->RegexFileRemoved2.find(this->Line)) {
      this->DoFile(PathUpdated, this->RegexFileRemoved2.match(1));
    }
    return true;
  }

  void DoFile(PathStatus status, std::string const& file)
  {
    std::string dir = cmSystemTools::GetFilenamePath(file);
    std::string name = cmSystemTools::GetFilenameName(file);
    this->CVS->Dirs[dir][name] = status;
  }
};

bool cmCTestCVS::UpdateImpl()
{
  // Get user-specified update options.
  std::string opts = this->CTest->GetCTestConfiguration("UpdateOptions");
  if (opts.empty()) {
    opts = this->CTest->GetCTestConfiguration("CVSUpdateOptions");
    if (opts.empty()) {
      opts = "-dP";
    }
  }
  std::vector<std::string> args = cmSystemTools::ParseArguments(opts.c_str());

  // Specify the start time for nightly testing.
  if (this->CTest->GetTestModel() == cmCTest::NIGHTLY) {
    args.push_back("-D" + this->GetNightlyTime() + " UTC");
  }

  // Run "cvs update" to update the work tree.
  std::vector<char const*> cvs_update;
  cvs_update.push_back(this->CommandLineTool.c_str());
  cvs_update.push_back("-z3");
  cvs_update.push_back("update");
  for (std::vector<std::string>::const_iterator ai = args.begin();
       ai != args.end(); ++ai) {
    cvs_update.push_back(ai->c_str());
  }
  cvs_update.push_back(CM_NULLPTR);

  UpdateParser out(this, "up-out> ");
  UpdateParser err(this, "up-err> ");
  return this->RunUpdateCommand(&cvs_update[0], &out, &err);
}

class cmCTestCVS::LogParser : public cmCTestVC::LineParser
{
public:
  typedef cmCTestCVS::Revision Revision;
  LogParser(cmCTestCVS* cvs, const char* prefix, std::vector<Revision>& revs)
    : CVS(cvs)
    , Revisions(revs)
    , Section(SectionHeader)
  {
    this->SetLog(&cvs->Log, prefix),
      this->RegexRevision.compile("^revision +([^ ]*) *$");
    this->RegexBranches.compile("^branches: .*$");
    this->RegexPerson.compile("^date: +([^;]+); +author: +([^;]+);");
  }

private:
  cmCTestCVS* CVS;
  std::vector<Revision>& Revisions;
  cmsys::RegularExpression RegexRevision;
  cmsys::RegularExpression RegexBranches;
  cmsys::RegularExpression RegexPerson;
  enum SectionType
  {
    SectionHeader,
    SectionRevisions,
    SectionEnd
  };
  SectionType Section;
  Revision Rev;

  bool ProcessLine() CM_OVERRIDE
  {
    if (this->Line == ("======================================="
                       "======================================")) {
      // This line ends the revision list.
      if (this->Section == SectionRevisions) {
        this->FinishRevision();
      }
      this->Section = SectionEnd;
    } else if (this->Line == "----------------------------") {
      // This line divides revisions from the header and each other.
      if (this->Section == SectionHeader) {
        this->Section = SectionRevisions;
      } else if (this->Section == SectionRevisions) {
        this->FinishRevision();
      }
    } else if (this->Section == SectionRevisions) {
      if (!this->Rev.Log.empty()) {
        // Continue the existing log.
        this->Rev.Log += this->Line;
        this->Rev.Log += "\n";
      } else if (this->Rev.Rev.empty() &&
                 this->RegexRevision.find(this->Line)) {
        this->Rev.Rev = this->RegexRevision.match(1);
      } else if (this->Rev.Date.empty() &&
                 this->RegexPerson.find(this->Line)) {
        this->Rev.Date = this->RegexPerson.match(1);
        this->Rev.Author = this->RegexPerson.match(2);
      } else if (!this->RegexBranches.find(this->Line)) {
        // Start the log.
        this->Rev.Log += this->Line;
        this->Rev.Log += "\n";
      }
    }
    return this->Section != SectionEnd;
  }

  void FinishRevision()
  {
    if (!this->Rev.Rev.empty()) {
      // Record this revision.
      /* clang-format off */
      this->CVS->Log << "Found revision " << this->Rev.Rev << "\n"
                     << "  author = " << this->Rev.Author << "\n"
                     << "  date = " << this->Rev.Date << "\n";
      /* clang-format on */
      this->Revisions.push_back(this->Rev);

      // We only need two revisions.
      if (this->Revisions.size() >= 2) {
        this->Section = SectionEnd;
      }
    }
    this->Rev = Revision();
  }
};

std::string cmCTestCVS::ComputeBranchFlag(std::string const& dir)
{
  // Compute the tag file location for this directory.
  std::string tagFile = this->SourceDirectory;
  if (!dir.empty()) {
    tagFile += "/";
    tagFile += dir;
  }
  tagFile += "/CVS/Tag";

  // Lookup the branch in the tag file, if any.
  std::string tagLine;
  cmsys::ifstream tagStream(tagFile.c_str());
  if (tagStream && cmSystemTools::GetLineFromStream(tagStream, tagLine) &&
      tagLine.size() > 1 && tagLine[0] == 'T') {
    // Use the branch specified in the tag file.
    std::string flag = "-r";
    flag += tagLine.substr(1);
    return flag;
  }
  // Use the default branch.
  return "-b";
}

void cmCTestCVS::LoadRevisions(std::string const& file, const char* branchFlag,
                               std::vector<Revision>& revisions)
{
  cmCTestLog(this->CTest, HANDLER_OUTPUT, "." << std::flush);

  // Run "cvs log" to get revisions of this file on this branch.
  const char* cvs = this->CommandLineTool.c_str();
  const char* cvs_log[] = { cvs,        "log",        "-N",
                            branchFlag, file.c_str(), CM_NULLPTR };

  LogParser out(this, "log-out> ", revisions);
  OutputLogger err(this->Log, "log-err> ");
  this->RunChild(cvs_log, &out, &err);
}

void cmCTestCVS::WriteXMLDirectory(cmXMLWriter& xml, std::string const& path,
                                   Directory const& dir)
{
  const char* slash = path.empty() ? "" : "/";
  xml.StartElement("Directory");
  xml.Element("Name", path);

  // Lookup the branch checked out in the working tree.
  std::string branchFlag = this->ComputeBranchFlag(path);

  // Load revisions and write an entry for each file in this directory.
  std::vector<Revision> revisions;
  for (Directory::const_iterator fi = dir.begin(); fi != dir.end(); ++fi) {
    std::string full = path + slash + fi->first;

    // Load two real or unknown revisions.
    revisions.clear();
    if (fi->second != PathUpdated) {
      // For local modifications the current rev is unknown and the
      // prior rev is the latest from cvs.
      revisions.push_back(this->Unknown);
    }
    this->LoadRevisions(full, branchFlag.c_str(), revisions);
    revisions.resize(2, this->Unknown);

    // Write the entry for this file with these revisions.
    File f(fi->second, &revisions[0], &revisions[1]);
    this->WriteXMLEntry(xml, path, fi->first, full, f);
  }
  xml.EndElement(); // Directory
}

bool cmCTestCVS::WriteXMLUpdates(cmXMLWriter& xml)
{
  cmCTestLog(this->CTest, HANDLER_OUTPUT,
             "   Gathering version information (one . per updated file):\n"
             "    "
               << std::flush);

  for (std::map<std::string, Directory>::const_iterator di =
         this->Dirs.begin();
       di != this->Dirs.end(); ++di) {
    this->WriteXMLDirectory(xml, di->first, di->second);
  }

  cmCTestLog(this->CTest, HANDLER_OUTPUT, std::endl);

  return true;
}
