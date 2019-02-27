/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestVC_h
#define cmCTestVC_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>

#include "cmProcessOutput.h"
#include "cmProcessTools.h"

class cmCTest;
class cmXMLWriter;

/** \class cmCTestVC
 * \brief Base class for version control system handlers
 *
 */
class cmCTestVC : public cmProcessTools
{
public:
  /** Construct with a CTest instance and update log stream.  */
  cmCTestVC(cmCTest* ctest, std::ostream& log);

  virtual ~cmCTestVC();

  /** Command line tool to invoke.  */
  void SetCommandLineTool(std::string const& tool);

  /** Top-level source directory.  */
  void SetSourceDirectory(std::string const& dir);

  /** Get the date/time specification for the current nightly start time.  */
  std::string GetNightlyTime();

  /** Prepare the work tree.  */
  bool InitialCheckout(const char* command);

  /** Perform cleanup operations on the work tree.  */
  void Cleanup();

  /** Update the working tree to the new revision.  */
  bool Update();

  /** Get the command line used by the Update method.  */
  std::string const& GetUpdateCommandLine() const
  {
    return this->UpdateCommandLine;
  }

  /** Write Update.xml entries for the updates found.  */
  bool WriteXML(cmXMLWriter& xml);

  /** Enumerate non-trivial working tree states during update.  */
  enum PathStatus
  {
    PathUpdated,
    PathModified,
    PathConflicting
  };

  /** Get the number of working tree paths in each state after update.  */
  int GetPathCount(PathStatus s) const { return this->PathCount[s]; }

protected:
  // Internal API to be implemented by subclasses.
  virtual void CleanupImpl();
  virtual bool NoteOldRevision();
  virtual bool UpdateImpl();
  virtual bool NoteNewRevision();
  virtual bool WriteXMLUpdates(cmXMLWriter& xml);

#if defined(__SUNPRO_CC) && __SUNPRO_CC <= 0x510
  // Sun CC 5.1 needs help to allow cmCTestSVN::Revision to see this
public:
#endif
  /** Basic information about one revision of a tree or file.  */
  struct Revision
  {
    std::string Rev;
    std::string Date;
    std::string Author;
    std::string EMail;
    std::string Committer;
    std::string CommitterEMail;
    std::string CommitDate;
    std::string Log;
  };

protected:
  friend struct File;

  /** Represent change to one file.  */
  struct File
  {
    PathStatus Status;
    Revision const* Rev;
    Revision const* PriorRev;
    File()
      : Status(PathUpdated)
      , Rev(nullptr)
      , PriorRev(nullptr)
    {
    }
    File(PathStatus status, Revision const* rev, Revision const* priorRev)
      : Status(status)
      , Rev(rev)
      , PriorRev(priorRev)
    {
    }
  };

  /** Convert a list of arguments to a human-readable command line.  */
  static std::string ComputeCommandLine(char const* const* cmd);

  /** Run a command line and send output to given parsers.  */
  bool RunChild(char const* const* cmd, OutputParser* out, OutputParser* err,
                const char* workDir = nullptr,
                Encoding encoding = cmProcessOutput::Auto);

  /** Run VC update command line and send output to given parsers.  */
  bool RunUpdateCommand(char const* const* cmd, OutputParser* out,
                        OutputParser* err = nullptr,
                        Encoding encoding = cmProcessOutput::Auto);

  /** Write xml element for one file.  */
  void WriteXMLEntry(cmXMLWriter& xml, std::string const& path,
                     std::string const& name, std::string const& full,
                     File const& f);

  // Instance of cmCTest running the script.
  cmCTest* CTest;

  // A stream to which we write log information.
  std::ostream& Log;

  // Basic information about the working tree.
  std::string CommandLineTool;
  std::string SourceDirectory;

  // Record update command info.
  std::string UpdateCommandLine;

  // Placeholder for unknown revisions.
  Revision Unknown;

  // Count paths reported with each PathStatus value.
  int PathCount[3];
};

#endif
