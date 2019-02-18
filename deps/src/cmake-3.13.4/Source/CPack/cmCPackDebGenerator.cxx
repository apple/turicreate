/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackDebGenerator.h"

#include "cmArchiveWrite.h"
#include "cmCPackComponentGroup.h"
#include "cmCPackGenerator.h"
#include "cmCPackLog.h"
#include "cmCryptoHash.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cm_sys_stat.h"

#include "cmsys/Glob.hxx"
#include <map>
#include <ostream>
#include <set>
#include <string.h>
#include <utility>

namespace {

class DebGenerator
{
public:
  DebGenerator(cmCPackLog* logger, std::string const& outputName,
               std::string const& workDir, std::string const& topLevelDir,
               std::string const& temporaryDir,
               const char* debianCompressionType,
               const char* debianArchiveType,
               std::map<std::string, std::string> const& controlValues,
               bool genShLibs, std::string const& shLibsFilename,
               bool genPostInst, std::string const& postInst, bool genPostRm,
               std::string const& postRm, const char* controlExtra,
               bool permissionStrctPolicy,
               std::vector<std::string> const& packageFiles);

  bool generate() const;

private:
  void generateDebianBinaryFile() const;
  void generateControlFile() const;
  bool generateDataTar() const;
  std::string generateMD5File() const;
  bool generateControlTar(std::string const& md5Filename) const;
  bool generateDeb() const;

  cmCPackLog* Logger;
  const std::string OutputName;
  const std::string WorkDir;
  std::string CompressionSuffix;
  const std::string TopLevelDir;
  const std::string TemporaryDir;
  const char* DebianArchiveType;
  const std::map<std::string, std::string> ControlValues;
  const bool GenShLibs;
  const std::string ShLibsFilename;
  const bool GenPostInst;
  const std::string PostInst;
  const bool GenPostRm;
  const std::string PostRm;
  const char* ControlExtra;
  const bool PermissionStrictPolicy;
  const std::vector<std::string> PackageFiles;
  cmArchiveWrite::Compress TarCompressionType;
};

DebGenerator::DebGenerator(
  cmCPackLog* logger, std::string const& outputName,
  std::string const& workDir, std::string const& topLevelDir,
  std::string const& temporaryDir, const char* debianCompressionType,
  const char* debianArchiveType,
  std::map<std::string, std::string> const& controlValues, bool genShLibs,
  std::string const& shLibsFilename, bool genPostInst,
  std::string const& postInst, bool genPostRm, std::string const& postRm,
  const char* controlExtra, bool permissionStrictPolicy,
  std::vector<std::string> const& packageFiles)
  : Logger(logger)
  , OutputName(outputName)
  , WorkDir(workDir)
  , TopLevelDir(topLevelDir)
  , TemporaryDir(temporaryDir)
  , DebianArchiveType(debianArchiveType ? debianArchiveType : "paxr")
  , ControlValues(controlValues)
  , GenShLibs(genShLibs)
  , ShLibsFilename(shLibsFilename)
  , GenPostInst(genPostInst)
  , PostInst(postInst)
  , GenPostRm(genPostRm)
  , PostRm(postRm)
  , ControlExtra(controlExtra)
  , PermissionStrictPolicy(permissionStrictPolicy)
  , PackageFiles(packageFiles)
{
  if (!debianCompressionType) {
    debianCompressionType = "gzip";
  }

  if (!strcmp(debianCompressionType, "lzma")) {
    CompressionSuffix = ".lzma";
    TarCompressionType = cmArchiveWrite::CompressLZMA;
  } else if (!strcmp(debianCompressionType, "xz")) {
    CompressionSuffix = ".xz";
    TarCompressionType = cmArchiveWrite::CompressXZ;
  } else if (!strcmp(debianCompressionType, "bzip2")) {
    CompressionSuffix = ".bz2";
    TarCompressionType = cmArchiveWrite::CompressBZip2;
  } else if (!strcmp(debianCompressionType, "gzip")) {
    CompressionSuffix = ".gz";
    TarCompressionType = cmArchiveWrite::CompressGZip;
  } else if (!strcmp(debianCompressionType, "none")) {
    CompressionSuffix.clear();
    TarCompressionType = cmArchiveWrite::CompressNone;
  } else {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error unrecognized compression type: "
                    << debianCompressionType << std::endl);
  }
}

bool DebGenerator::generate() const
{
  generateDebianBinaryFile();
  generateControlFile();
  if (!generateDataTar()) {
    return false;
  }
  std::string md5Filename = generateMD5File();
  if (!generateControlTar(md5Filename)) {
    return false;
  }
  return generateDeb();
}

void DebGenerator::generateDebianBinaryFile() const
{
  // debian-binary file
  const std::string dbfilename = WorkDir + "/debian-binary";
  cmGeneratedFileStream out(dbfilename);
  out << "2.0";
  out << std::endl; // required for valid debian package
}

void DebGenerator::generateControlFile() const
{
  std::string ctlfilename = WorkDir + "/control";

  cmGeneratedFileStream out(ctlfilename);
  for (auto const& kv : ControlValues) {
    out << kv.first << ": " << kv.second << "\n";
  }

  unsigned long totalSize = 0;
  {
    std::string dirName = TemporaryDir;
    dirName += '/';
    for (std::string const& file : PackageFiles) {
      totalSize += cmSystemTools::FileLength(file);
    }
  }
  out << "Installed-Size: " << (totalSize + 1023) / 1024 << "\n";
  out << std::endl;
}

bool DebGenerator::generateDataTar() const
{
  std::string filename_data_tar = WorkDir + "/data.tar" + CompressionSuffix;
  cmGeneratedFileStream fileStream_data_tar;
  fileStream_data_tar.Open(filename_data_tar, false, true);
  if (!fileStream_data_tar) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error opening the file \""
                    << filename_data_tar << "\" for writing" << std::endl);
    return false;
  }
  cmArchiveWrite data_tar(fileStream_data_tar, TarCompressionType,
                          DebianArchiveType);

  // uid/gid should be the one of the root user, and this root user has
  // always uid/gid equal to 0.
  data_tar.SetUIDAndGID(0u, 0u);
  data_tar.SetUNAMEAndGNAME("root", "root");

  // now add all directories which have to be compressed
  // collect all top level install dirs for that
  // e.g. /opt/bin/foo, /usr/bin/bar and /usr/bin/baz would
  // give /usr and /opt
  size_t topLevelLength = WorkDir.length();
  cmCPackLogger(cmCPackLog::LOG_DEBUG,
                "WDIR: \"" << WorkDir << "\", length = " << topLevelLength
                           << std::endl);
  std::set<std::string> orderedFiles;

  // we have to reconstruct the parent folders as well

  for (std::string currentPath : PackageFiles) {
    while (currentPath != WorkDir) {
      // the last one IS WorkDir, but we do not want this one:
      // XXX/application/usr/bin/myprogram with GEN_WDIR=XXX/application
      // should not add XXX/application
      orderedFiles.insert(currentPath);
      currentPath = cmSystemTools::CollapseCombinedPath(currentPath, "..");
    }
  }

  for (std::string const& file : orderedFiles) {
    cmCPackLogger(cmCPackLog::LOG_DEBUG,
                  "FILEIT: \"" << file << "\"" << std::endl);
    std::string::size_type slashPos = file.find('/', topLevelLength + 1);
    std::string relativeDir =
      file.substr(topLevelLength, slashPos - topLevelLength);
    cmCPackLogger(cmCPackLog::LOG_DEBUG,
                  "RELATIVEDIR: \"" << relativeDir << "\"" << std::endl);

#ifdef WIN32
    std::string mode_t_adt_filename = file + ":cmake_mode_t";
    cmsys::ifstream permissionStream(mode_t_adt_filename.c_str());

    mode_t permissions = 0;

    if (permissionStream) {
      permissionStream >> std::oct >> permissions;
    }

    if (permissions != 0) {
      data_tar.SetPermissions(permissions);
    } else if (cmSystemTools::FileIsDirectory(file)) {
      data_tar.SetPermissions(0755);
    } else {
      data_tar.ClearPermissions();
    }
#endif

    // do not recurse because the loop will do it
    if (!data_tar.Add(file, topLevelLength, ".", false)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Problem adding file to tar:"
                      << std::endl
                      << "#top level directory: " << WorkDir << std::endl
                      << "#file: " << file << std::endl
                      << "#error:" << data_tar.GetError() << std::endl);
      return false;
    }
  }
  return true;
}

std::string DebGenerator::generateMD5File() const
{
  std::string md5filename = WorkDir + "/md5sums";

  cmGeneratedFileStream out(md5filename);

  std::string topLevelWithTrailingSlash = TemporaryDir;
  topLevelWithTrailingSlash += '/';
  for (std::string const& file : PackageFiles) {
    // hash only regular files
    if (cmSystemTools::FileIsDirectory(file) ||
        cmSystemTools::FileIsSymlink(file)) {
      continue;
    }

    std::string output =
      cmSystemTools::ComputeFileHash(file, cmCryptoHash::AlgoMD5);
    if (output.empty()) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Problem computing the md5 of " << file << std::endl);
    }

    output += "  " + file + "\n";
    // debian md5sums entries are like this:
    // 014f3604694729f3bf19263bac599765  usr/bin/ccmake
    // thus strip the full path (with the trailing slash)
    cmSystemTools::ReplaceString(output, topLevelWithTrailingSlash.c_str(),
                                 "");
    out << output;
  }
  // each line contains a eol.
  // Do not end the md5sum file with yet another (invalid)
  return md5filename;
}

bool DebGenerator::generateControlTar(std::string const& md5Filename) const
{
  std::string filename_control_tar = WorkDir + "/control.tar.gz";

  cmGeneratedFileStream fileStream_control_tar;
  fileStream_control_tar.Open(filename_control_tar, false, true);
  if (!fileStream_control_tar) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error opening the file \""
                    << filename_control_tar << "\" for writing" << std::endl);
    return false;
  }
  cmArchiveWrite control_tar(fileStream_control_tar,
                             cmArchiveWrite::CompressGZip, DebianArchiveType);

  // sets permissions and uid/gid for the files
  control_tar.SetUIDAndGID(0u, 0u);
  control_tar.SetUNAMEAndGNAME("root", "root");

  /* permissions are set according to
  https://www.debian.org/doc/debian-policy/ch-files.html#s-permissions-owners
  and
  https://lintian.debian.org/tags/control-file-has-bad-permissions.html
  */
  const mode_t permission644 = 0644;
  const mode_t permissionExecute = 0111;
  const mode_t permission755 = permission644 | permissionExecute;

  // for md5sum and control (that we have generated here), we use 644
  // (RW-R--R--)
  // so that deb lintian doesn't warn about it
  control_tar.SetPermissions(permission644);

  // adds control and md5sums
  if (!control_tar.Add(md5Filename, WorkDir.length(), ".") ||
      !control_tar.Add(WorkDir + "/control", WorkDir.length(), ".")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error adding file to tar:"
                    << std::endl
                    << "#top level directory: " << WorkDir << std::endl
                    << "#file: \"control\" or \"md5sums\"" << std::endl
                    << "#error:" << control_tar.GetError() << std::endl);
    return false;
  }

  // adds generated shlibs file
  if (GenShLibs) {
    if (!control_tar.Add(ShLibsFilename, WorkDir.length(), ".")) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error adding file to tar:"
                      << std::endl
                      << "#top level directory: " << WorkDir << std::endl
                      << "#file: \"shlibs\"" << std::endl
                      << "#error:" << control_tar.GetError() << std::endl);
      return false;
    }
  }

  // adds LDCONFIG related files
  if (GenPostInst) {
    control_tar.SetPermissions(permission755);
    if (!control_tar.Add(PostInst, WorkDir.length(), ".")) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error adding file to tar:"
                      << std::endl
                      << "#top level directory: " << WorkDir << std::endl
                      << "#file: \"postinst\"" << std::endl
                      << "#error:" << control_tar.GetError() << std::endl);
      return false;
    }
    control_tar.SetPermissions(permission644);
  }

  if (GenPostRm) {
    control_tar.SetPermissions(permission755);
    if (!control_tar.Add(PostRm, WorkDir.length(), ".")) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error adding file to tar:"
                      << std::endl
                      << "#top level directory: " << WorkDir << std::endl
                      << "#file: \"postinst\"" << std::endl
                      << "#error:" << control_tar.GetError() << std::endl);
      return false;
    }
    control_tar.SetPermissions(permission644);
  }

  // for the other files, we use
  // -either the original permission on the files
  // -either a permission strictly defined by the Debian policies
  if (ControlExtra) {
    // permissions are now controlled by the original file permissions

    static const char* strictFiles[] = { "config", "postinst", "postrm",
                                         "preinst", "prerm" };
    std::set<std::string> setStrictFiles(
      strictFiles, strictFiles + sizeof(strictFiles) / sizeof(strictFiles[0]));

    // default
    control_tar.ClearPermissions();

    std::vector<std::string> controlExtraList;
    cmSystemTools::ExpandListArgument(ControlExtra, controlExtraList);
    for (std::string const& i : controlExtraList) {
      std::string filenamename = cmsys::SystemTools::GetFilenameName(i);
      std::string localcopy = WorkDir + "/" + filenamename;

      if (PermissionStrictPolicy) {
        control_tar.SetPermissions(
          setStrictFiles.count(filenamename) ? permission755 : permission644);
      }

      // if we can copy the file, it means it does exist, let's add it:
      if (cmsys::SystemTools::CopyFileIfDifferent(i, localcopy)) {
        control_tar.Add(localcopy, WorkDir.length(), ".");
      }
    }
  }

  return true;
}

bool DebGenerator::generateDeb() const
{
  // ar -r your-package-name.deb debian-binary control.tar.* data.tar.*
  // A debian package .deb is simply an 'ar' archive. The only subtle
  // difference is that debian uses the BSD ar style archive whereas most
  // Linux distro have a GNU ar.
  // See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=161593 for more info
  std::string const outputPath = TopLevelDir + "/" + OutputName;
  std::string const tlDir = WorkDir + "/";
  cmGeneratedFileStream debStream;
  debStream.Open(outputPath, false, true);
  cmArchiveWrite deb(debStream, cmArchiveWrite::CompressNone, "arbsd");

  // uid/gid should be the one of the root user, and this root user has
  // always uid/gid equal to 0.
  deb.SetUIDAndGID(0u, 0u);
  deb.SetUNAMEAndGNAME("root", "root");

  if (!deb.Add(tlDir + "debian-binary", tlDir.length()) ||
      !deb.Add(tlDir + "control.tar.gz", tlDir.length()) ||
      !deb.Add(tlDir + "data.tar" + CompressionSuffix, tlDir.length())) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error creating debian package:"
                    << std::endl
                    << "#top level directory: " << TopLevelDir << std::endl
                    << "#file: " << OutputName << std::endl
                    << "#error:" << deb.GetError() << std::endl);
    return false;
  }
  return true;
}

} // end anonymous namespace

cmCPackDebGenerator::cmCPackDebGenerator()
{
}

cmCPackDebGenerator::~cmCPackDebGenerator()
{
}

int cmCPackDebGenerator::InitializeInternal()
{
  this->SetOptionIfNotSet("CPACK_PACKAGING_INSTALL_PREFIX", "/usr");
  if (cmSystemTools::IsOff(this->GetOption("CPACK_SET_DESTDIR"))) {
    this->SetOption("CPACK_SET_DESTDIR", "I_ON");
  }
  return this->Superclass::InitializeInternal();
}

int cmCPackDebGenerator::PackageOnePack(std::string const& initialTopLevel,
                                        std::string const& packageName)
{
  int retval = 1;
  // Begin the archive for this pack
  std::string localToplevel(initialTopLevel);
  std::string packageFileName(cmSystemTools::GetParentDirectory(toplevel));
  std::string outputFileName(
    std::string(this->GetOption("CPACK_PACKAGE_FILE_NAME")) + "-" +
    packageName + this->GetOutputExtension());

  localToplevel += "/" + packageName;
  /* replace the TEMP DIRECTORY with the component one */
  this->SetOption("CPACK_TEMPORARY_DIRECTORY", localToplevel.c_str());
  packageFileName += "/" + outputFileName;
  /* replace proposed CPACK_OUTPUT_FILE_NAME */
  this->SetOption("CPACK_OUTPUT_FILE_NAME", outputFileName.c_str());
  /* replace the TEMPORARY package file name */
  this->SetOption("CPACK_TEMPORARY_PACKAGE_FILE_NAME",
                  packageFileName.c_str());
  // Tell CPackDeb.cmake the name of the component GROUP.
  this->SetOption("CPACK_DEB_PACKAGE_COMPONENT", packageName.c_str());
  // Tell CPackDeb.cmake the path where the component is.
  std::string component_path = "/";
  component_path += packageName;
  this->SetOption("CPACK_DEB_PACKAGE_COMPONENT_PART_PATH",
                  component_path.c_str());
  if (!this->ReadListFile("Internal/CPack/CPackDeb.cmake")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error while execution CPackDeb.cmake" << std::endl);
    retval = 0;
    return retval;
  }

  { // Isolate globbing of binaries vs. dbgsyms
    cmsys::Glob gl;
    std::string findExpr(this->GetOption("GEN_WDIR"));
    findExpr += "/*";
    gl.RecurseOn();
    gl.SetRecurseListDirs(true);
    if (!gl.FindFiles(findExpr)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Cannot find any files in the installed directory"
                      << std::endl);
      return 0;
    }
    packageFiles = gl.GetFiles();
  }

  int res = createDeb();
  if (res != 1) {
    retval = 0;
  }
  // add the generated package to package file names list
  packageFileName = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  packageFileName += "/";
  packageFileName += this->GetOption("GEN_CPACK_OUTPUT_FILE_NAME");
  packageFileNames.push_back(std::move(packageFileName));

  if (this->IsOn("GEN_CPACK_DEBIAN_DEBUGINFO_PACKAGE")) {
    cmsys::Glob gl;
    std::string findExpr(this->GetOption("GEN_DBGSYMDIR"));
    findExpr += "/*";
    gl.RecurseOn();
    gl.SetRecurseListDirs(true);
    if (!gl.FindFiles(findExpr)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Cannot find any files in the installed directory"
                      << std::endl);
      return 0;
    }
    packageFiles = gl.GetFiles();

    res = createDbgsymDDeb();
    if (res != 1) {
      retval = 0;
    }
    // add the generated package to package file names list
    packageFileName = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
    packageFileName += "/";
    packageFileName += this->GetOption("GEN_CPACK_DBGSYM_OUTPUT_FILE_NAME");
    packageFileNames.push_back(std::move(packageFileName));
  }

  return retval;
}

int cmCPackDebGenerator::PackageComponents(bool ignoreGroup)
{
  int retval = 1;
  /* Reset package file name list it will be populated during the
   * component packaging run*/
  packageFileNames.clear();
  std::string initialTopLevel(this->GetOption("CPACK_TEMPORARY_DIRECTORY"));

  // The default behavior is to have one package by component group
  // unless CPACK_COMPONENTS_IGNORE_GROUP is specified.
  if (!ignoreGroup) {
    for (auto const& compG : this->ComponentGroups) {
      cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                    "Packaging component group: " << compG.first << std::endl);
      // Begin the archive for this group
      retval &= PackageOnePack(initialTopLevel, compG.first);
    }
    // Handle Orphan components (components not belonging to any groups)
    for (auto const& comp : this->Components) {
      // Does the component belong to a group?
      if (comp.second.Group == nullptr) {
        cmCPackLogger(
          cmCPackLog::LOG_VERBOSE,
          "Component <"
            << comp.second.Name
            << "> does not belong to any group, package it separately."
            << std::endl);
        // Begin the archive for this orphan component
        retval &= PackageOnePack(initialTopLevel, comp.first);
      }
    }
  }
  // CPACK_COMPONENTS_IGNORE_GROUPS is set
  // We build 1 package per component
  else {
    for (auto const& comp : this->Components) {
      retval &= PackageOnePack(initialTopLevel, comp.first);
    }
  }
  return retval;
}

//----------------------------------------------------------------------
int cmCPackDebGenerator::PackageComponentsAllInOne(
  const std::string& compInstDirName)
{
  int retval = 1;
  /* Reset package file name list it will be populated during the
   * component packaging run*/
  packageFileNames.clear();
  std::string initialTopLevel(this->GetOption("CPACK_TEMPORARY_DIRECTORY"));

  cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                "Packaging all groups in one package..."
                "(CPACK_COMPONENTS_ALL_[GROUPS_]IN_ONE_PACKAGE is set)"
                  << std::endl);

  // The ALL GROUPS in ONE package case
  std::string localToplevel(initialTopLevel);
  std::string packageFileName(cmSystemTools::GetParentDirectory(toplevel));
  std::string outputFileName(
    std::string(this->GetOption("CPACK_PACKAGE_FILE_NAME")) +
    this->GetOutputExtension());
  // all GROUP in one vs all COMPONENT in one
  // if must be here otherwise non component paths have a trailing / while
  // components don't
  if (!compInstDirName.empty()) {
    localToplevel += "/" + compInstDirName;
  }

  /* replace the TEMP DIRECTORY with the component one */
  this->SetOption("CPACK_TEMPORARY_DIRECTORY", localToplevel.c_str());
  packageFileName += "/" + outputFileName;
  /* replace proposed CPACK_OUTPUT_FILE_NAME */
  this->SetOption("CPACK_OUTPUT_FILE_NAME", outputFileName.c_str());
  /* replace the TEMPORARY package file name */
  this->SetOption("CPACK_TEMPORARY_PACKAGE_FILE_NAME",
                  packageFileName.c_str());

  if (!compInstDirName.empty()) {
    // Tell CPackDeb.cmake the path where the component is.
    std::string component_path = "/";
    component_path += compInstDirName;
    this->SetOption("CPACK_DEB_PACKAGE_COMPONENT_PART_PATH",
                    component_path.c_str());
  }
  if (!this->ReadListFile("Internal/CPack/CPackDeb.cmake")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error while execution CPackDeb.cmake" << std::endl);
    retval = 0;
    return retval;
  }

  cmsys::Glob gl;
  std::string findExpr(this->GetOption("GEN_WDIR"));
  findExpr += "/*";
  gl.RecurseOn();
  gl.SetRecurseListDirs(true);
  if (!gl.FindFiles(findExpr)) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Cannot find any files in the installed directory"
                    << std::endl);
    return 0;
  }
  packageFiles = gl.GetFiles();

  int res = createDeb();
  if (res != 1) {
    retval = 0;
  }
  // add the generated package to package file names list
  packageFileName = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  packageFileName += "/";
  packageFileName += this->GetOption("GEN_CPACK_OUTPUT_FILE_NAME");
  packageFileNames.push_back(std::move(packageFileName));
  return retval;
}

int cmCPackDebGenerator::PackageFiles()
{
  /* Are we in the component packaging case */
  if (WantsComponentInstallation()) {
    // CASE 1 : COMPONENT ALL-IN-ONE package
    // If ALL GROUPS or ALL COMPONENTS in ONE package has been requested
    // then the package file is unique and should be open here.
    if (componentPackageMethod == ONE_PACKAGE) {
      return PackageComponentsAllInOne("ALL_COMPONENTS_IN_ONE");
    }
    // CASE 2 : COMPONENT CLASSICAL package(s) (i.e. not all-in-one)
    // There will be 1 package for each component group
    // however one may require to ignore component group and
    // in this case you'll get 1 package for each component.
    return PackageComponents(componentPackageMethod ==
                             ONE_PACKAGE_PER_COMPONENT);
  }
  // CASE 3 : NON COMPONENT package.
  return PackageComponentsAllInOne("");
}

int cmCPackDebGenerator::createDeb()
{
  std::map<std::string, std::string> controlValues;

  // debian policy enforce lower case for package name
  controlValues["Package"] = cmsys::SystemTools::LowerCase(
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_NAME"));
  controlValues["Version"] =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_VERSION");
  controlValues["Section"] =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SECTION");
  controlValues["Priority"] =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_PRIORITY");
  controlValues["Architecture"] =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_ARCHITECTURE");
  controlValues["Maintainer"] =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_MAINTAINER");
  controlValues["Description"] =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_DESCRIPTION");

  const char* debian_pkg_source =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SOURCE");
  if (debian_pkg_source && *debian_pkg_source) {
    controlValues["Source"] = debian_pkg_source;
  }
  const char* debian_pkg_dep =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_DEPENDS");
  if (debian_pkg_dep && *debian_pkg_dep) {
    controlValues["Depends"] = debian_pkg_dep;
  }
  const char* debian_pkg_rec =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_RECOMMENDS");
  if (debian_pkg_rec && *debian_pkg_rec) {
    controlValues["Recommends"] = debian_pkg_rec;
  }
  const char* debian_pkg_sug =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SUGGESTS");
  if (debian_pkg_sug && *debian_pkg_sug) {
    controlValues["Suggests"] = debian_pkg_sug;
  }
  const char* debian_pkg_url =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_HOMEPAGE");
  if (debian_pkg_url && *debian_pkg_url) {
    controlValues["Homepage"] = debian_pkg_url;
  }
  const char* debian_pkg_predep =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_PREDEPENDS");
  if (debian_pkg_predep && *debian_pkg_predep) {
    controlValues["Pre-Depends"] = debian_pkg_predep;
  }
  const char* debian_pkg_enhances =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_ENHANCES");
  if (debian_pkg_enhances && *debian_pkg_enhances) {
    controlValues["Enhances"] = debian_pkg_enhances;
  }
  const char* debian_pkg_breaks =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_BREAKS");
  if (debian_pkg_breaks && *debian_pkg_breaks) {
    controlValues["Breaks"] = debian_pkg_breaks;
  }
  const char* debian_pkg_conflicts =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_CONFLICTS");
  if (debian_pkg_conflicts && *debian_pkg_conflicts) {
    controlValues["Conflicts"] = debian_pkg_conflicts;
  }
  const char* debian_pkg_provides =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_PROVIDES");
  if (debian_pkg_provides && *debian_pkg_provides) {
    controlValues["Provides"] = debian_pkg_provides;
  }
  const char* debian_pkg_replaces =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_REPLACES");
  if (debian_pkg_replaces && *debian_pkg_replaces) {
    controlValues["Replaces"] = debian_pkg_replaces;
  }

  const std::string strGenWDIR(this->GetOption("GEN_WDIR"));
  const std::string shlibsfilename = strGenWDIR + "/shlibs";

  const char* debian_pkg_shlibs =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SHLIBS");
  const bool gen_shibs = this->IsOn("CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS") &&
    debian_pkg_shlibs && *debian_pkg_shlibs;
  if (gen_shibs) {
    cmGeneratedFileStream out(shlibsfilename);
    out << debian_pkg_shlibs;
    out << std::endl;
  }

  const std::string postinst = strGenWDIR + "/postinst";
  const std::string postrm = strGenWDIR + "/postrm";
  if (this->IsOn("GEN_CPACK_DEBIAN_GENERATE_POSTINST")) {
    cmGeneratedFileStream out(postinst);
    out << "#!/bin/sh\n\n"
           "set -e\n\n"
           "if [ \"$1\" = \"configure\" ]; then\n"
           "\tldconfig\n"
           "fi\n";
  }
  if (this->IsOn("GEN_CPACK_DEBIAN_GENERATE_POSTRM")) {
    cmGeneratedFileStream out(postrm);
    out << "#!/bin/sh\n\n"
           "set -e\n\n"
           "if [ \"$1\" = \"remove\" ]; then\n"
           "\tldconfig\n"
           "fi\n";
  }

  DebGenerator gen(
    Logger, this->GetOption("GEN_CPACK_OUTPUT_FILE_NAME"), strGenWDIR,
    this->GetOption("CPACK_TOPLEVEL_DIRECTORY"),
    this->GetOption("CPACK_TEMPORARY_DIRECTORY"),
    this->GetOption("GEN_CPACK_DEBIAN_COMPRESSION_TYPE"),
    this->GetOption("GEN_CPACK_DEBIAN_ARCHIVE_TYPE"), controlValues, gen_shibs,
    shlibsfilename, this->IsOn("GEN_CPACK_DEBIAN_GENERATE_POSTINST"), postinst,
    this->IsOn("GEN_CPACK_DEBIAN_GENERATE_POSTRM"), postrm,
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA"),
    this->IsSet("GEN_CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION"),
    packageFiles);

  if (!gen.generate()) {
    return 0;
  }
  return 1;
}

int cmCPackDebGenerator::createDbgsymDDeb()
{
  // Packages containing debug symbols follow the same structure as .debs
  // but have different metadata and content.

  std::map<std::string, std::string> controlValues;
  // debian policy enforce lower case for package name
  std::string packageNameLower = cmsys::SystemTools::LowerCase(
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_NAME"));
  const char* debian_pkg_version =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_VERSION");

  controlValues["Package"] = packageNameLower + "-dbgsym";
  controlValues["Package-Type"] = "ddeb";
  controlValues["Version"] = debian_pkg_version;
  controlValues["Auto-Built-Package"] = "debug-symbols";
  controlValues["Depends"] = this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_NAME") +
    std::string(" (= ") + debian_pkg_version + ")";
  controlValues["Section"] = "debug";
  controlValues["Priority"] = "optional";
  controlValues["Architecture"] =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_ARCHITECTURE");
  controlValues["Maintainer"] =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_MAINTAINER");
  controlValues["Description"] =
    std::string("debug symbols for ") + packageNameLower;

  const char* debian_pkg_source =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SOURCE");
  if (debian_pkg_source && *debian_pkg_source) {
    controlValues["Source"] = debian_pkg_source;
  }
  const char* debian_build_ids = this->GetOption("GEN_BUILD_IDS");
  if (debian_build_ids && *debian_build_ids) {
    controlValues["Build-Ids"] = debian_build_ids;
  }

  DebGenerator gen(
    Logger, this->GetOption("GEN_CPACK_DBGSYM_OUTPUT_FILE_NAME"),
    this->GetOption("GEN_DBGSYMDIR"),

    this->GetOption("CPACK_TOPLEVEL_DIRECTORY"),
    this->GetOption("CPACK_TEMPORARY_DIRECTORY"),
    this->GetOption("GEN_CPACK_DEBIAN_COMPRESSION_TYPE"),
    this->GetOption("GEN_CPACK_DEBIAN_ARCHIVE_TYPE"), controlValues, false, "",
    false, "", false, "", nullptr,
    this->IsSet("GEN_CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION"),
    packageFiles);

  if (!gen.generate()) {
    return 0;
  }
  return 1;
}

bool cmCPackDebGenerator::SupportsComponentInstallation() const
{
  return IsOn("CPACK_DEB_COMPONENT_INSTALL");
}

std::string cmCPackDebGenerator::GetComponentInstallDirNameSuffix(
  const std::string& componentName)
{
  if (componentPackageMethod == ONE_PACKAGE_PER_COMPONENT) {
    return componentName;
  }

  if (componentPackageMethod == ONE_PACKAGE) {
    return std::string("ALL_COMPONENTS_IN_ONE");
  }
  // We have to find the name of the COMPONENT GROUP
  // the current COMPONENT belongs to.
  std::string groupVar =
    "CPACK_COMPONENT_" + cmSystemTools::UpperCase(componentName) + "_GROUP";
  if (nullptr != GetOption(groupVar)) {
    return std::string(GetOption(groupVar));
  }
  return componentName;
}
