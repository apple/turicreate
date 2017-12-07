/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackDebGenerator.h"

#include "cmArchiveWrite.h"
#include "cmCPackComponentGroup.h"
#include "cmCPackGenerator.h"
#include "cmCPackLog.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cm_sys_stat.h"

#include "cmsys/Glob.hxx"
#include <limits.h>
#include <map>
#include <ostream>
#include <set>
#include <stdio.h>
#include <string.h>
#include <utility>

// NOTE:
// A debian package .deb is simply an 'ar' archive. The only subtle difference
// is that debian uses the BSD ar style archive whereas most Linux distro have
// a GNU ar.
// See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=161593 for more info
// Therefore we provide our own implementation of a BSD-ar:
static int ar_append(const char* archive,
                     const std::vector<std::string>& files);

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
  if (!this->ReadListFile("CPackDeb.cmake")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Error while execution CPackDeb.cmake"
                    << std::endl);
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
  packageFileNames.push_back(packageFileName);
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
    std::map<std::string, cmCPackComponentGroup>::iterator compGIt;
    for (compGIt = this->ComponentGroups.begin();
         compGIt != this->ComponentGroups.end(); ++compGIt) {
      cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Packaging component group: "
                      << compGIt->first << std::endl);
      // Begin the archive for this group
      retval &= PackageOnePack(initialTopLevel, compGIt->first);
    }
    // Handle Orphan components (components not belonging to any groups)
    std::map<std::string, cmCPackComponent>::iterator compIt;
    for (compIt = this->Components.begin(); compIt != this->Components.end();
         ++compIt) {
      // Does the component belong to a group?
      if (compIt->second.Group == CM_NULLPTR) {
        cmCPackLogger(
          cmCPackLog::LOG_VERBOSE, "Component <"
            << compIt->second.Name
            << "> does not belong to any group, package it separately."
            << std::endl);
        // Begin the archive for this orphan component
        retval &= PackageOnePack(initialTopLevel, compIt->first);
      }
    }
  }
  // CPACK_COMPONENTS_IGNORE_GROUPS is set
  // We build 1 package per component
  else {
    std::map<std::string, cmCPackComponent>::iterator compIt;
    for (compIt = this->Components.begin(); compIt != this->Components.end();
         ++compIt) {
      retval &= PackageOnePack(initialTopLevel, compIt->first);
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
  if (!this->ReadListFile("CPackDeb.cmake")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Error while execution CPackDeb.cmake"
                    << std::endl);
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
  packageFileNames.push_back(packageFileName);
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
  // debian-binary file
  const std::string strGenWDIR(this->GetOption("GEN_WDIR"));
  const std::string dbfilename = strGenWDIR + "/debian-binary";
  { // the scope is needed for cmGeneratedFileStream
    cmGeneratedFileStream out(dbfilename.c_str());
    out << "2.0";
    out << std::endl; // required for valid debian package
  }

  // control file
  std::string ctlfilename = strGenWDIR + "/control";

  // debian policy enforce lower case for package name
  // mandatory entries:
  std::string debian_pkg_name = cmsys::SystemTools::LowerCase(
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_NAME"));
  const char* debian_pkg_version =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_VERSION");
  const char* debian_pkg_section =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SECTION");
  const char* debian_pkg_priority =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_PRIORITY");
  const char* debian_pkg_arch =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_ARCHITECTURE");
  const char* maintainer =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_MAINTAINER");
  const char* desc = this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_DESCRIPTION");

  // optional entries
  const char* debian_pkg_dep =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_DEPENDS");
  const char* debian_pkg_rec =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_RECOMMENDS");
  const char* debian_pkg_sug =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SUGGESTS");
  const char* debian_pkg_url =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_HOMEPAGE");
  const char* debian_pkg_predep =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_PREDEPENDS");
  const char* debian_pkg_enhances =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_ENHANCES");
  const char* debian_pkg_breaks =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_BREAKS");
  const char* debian_pkg_conflicts =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_CONFLICTS");
  const char* debian_pkg_provides =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_PROVIDES");
  const char* debian_pkg_replaces =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_REPLACES");
  const char* debian_pkg_source =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SOURCE");

  { // the scope is needed for cmGeneratedFileStream
    cmGeneratedFileStream out(ctlfilename.c_str());
    out << "Package: " << debian_pkg_name << "\n";
    out << "Version: " << debian_pkg_version << "\n";
    out << "Section: " << debian_pkg_section << "\n";
    out << "Priority: " << debian_pkg_priority << "\n";
    out << "Architecture: " << debian_pkg_arch << "\n";
    if (debian_pkg_source && *debian_pkg_source) {
      out << "Source: " << debian_pkg_source << "\n";
    }
    if (debian_pkg_dep && *debian_pkg_dep) {
      out << "Depends: " << debian_pkg_dep << "\n";
    }
    if (debian_pkg_rec && *debian_pkg_rec) {
      out << "Recommends: " << debian_pkg_rec << "\n";
    }
    if (debian_pkg_sug && *debian_pkg_sug) {
      out << "Suggests: " << debian_pkg_sug << "\n";
    }
    if (debian_pkg_url && *debian_pkg_url) {
      out << "Homepage: " << debian_pkg_url << "\n";
    }
    if (debian_pkg_predep && *debian_pkg_predep) {
      out << "Pre-Depends: " << debian_pkg_predep << "\n";
    }
    if (debian_pkg_enhances && *debian_pkg_enhances) {
      out << "Enhances: " << debian_pkg_enhances << "\n";
    }
    if (debian_pkg_breaks && *debian_pkg_breaks) {
      out << "Breaks: " << debian_pkg_breaks << "\n";
    }
    if (debian_pkg_conflicts && *debian_pkg_conflicts) {
      out << "Conflicts: " << debian_pkg_conflicts << "\n";
    }
    if (debian_pkg_provides && *debian_pkg_provides) {
      out << "Provides: " << debian_pkg_provides << "\n";
    }
    if (debian_pkg_replaces && *debian_pkg_replaces) {
      out << "Replaces: " << debian_pkg_replaces << "\n";
    }
    unsigned long totalSize = 0;
    {
      std::string dirName = this->GetOption("CPACK_TEMPORARY_DIRECTORY");
      dirName += '/';
      for (std::vector<std::string>::const_iterator fileIt =
             packageFiles.begin();
           fileIt != packageFiles.end(); ++fileIt) {
        totalSize += cmSystemTools::FileLength(*fileIt);
      }
    }
    out << "Installed-Size: " << (totalSize + 1023) / 1024 << "\n";
    out << "Maintainer: " << maintainer << "\n";
    out << "Description: " << desc << "\n";
    out << std::endl;
  }

  const std::string shlibsfilename = strGenWDIR + "/shlibs";

  const char* debian_pkg_shlibs =
    this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_SHLIBS");
  const bool gen_shibs = this->IsOn("CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS") &&
    debian_pkg_shlibs && *debian_pkg_shlibs;
  if (gen_shibs) {
    cmGeneratedFileStream out(shlibsfilename.c_str());
    out << debian_pkg_shlibs;
    out << std::endl;
  }

  const std::string postinst = strGenWDIR + "/postinst";
  const std::string postrm = strGenWDIR + "/postrm";
  if (this->IsOn("GEN_CPACK_DEBIAN_GENERATE_POSTINST")) {
    cmGeneratedFileStream out(postinst.c_str());
    out << "#!/bin/sh\n\n"
           "set -e\n\n"
           "if [ \"$1\" = \"configure\" ]; then\n"
           "\tldconfig\n"
           "fi\n";
  }
  if (this->IsOn("GEN_CPACK_DEBIAN_GENERATE_POSTRM")) {
    cmGeneratedFileStream out(postrm.c_str());
    out << "#!/bin/sh\n\n"
           "set -e\n\n"
           "if [ \"$1\" = \"remove\" ]; then\n"
           "\tldconfig\n"
           "fi\n";
  }

  cmArchiveWrite::Compress tar_compression_type = cmArchiveWrite::CompressGZip;
  const char* debian_compression_type =
    this->GetOption("GEN_CPACK_DEBIAN_COMPRESSION_TYPE");
  if (!debian_compression_type) {
    debian_compression_type = "gzip";
  }

  std::string compression_suffix;
  if (!strcmp(debian_compression_type, "lzma")) {
    compression_suffix = ".lzma";
    tar_compression_type = cmArchiveWrite::CompressLZMA;
  } else if (!strcmp(debian_compression_type, "xz")) {
    compression_suffix = ".xz";
    tar_compression_type = cmArchiveWrite::CompressXZ;
  } else if (!strcmp(debian_compression_type, "bzip2")) {
    compression_suffix = ".bz2";
    tar_compression_type = cmArchiveWrite::CompressBZip2;
  } else if (!strcmp(debian_compression_type, "gzip")) {
    compression_suffix = ".gz";
    tar_compression_type = cmArchiveWrite::CompressGZip;
  } else if (!strcmp(debian_compression_type, "none")) {
    compression_suffix = "";
    tar_compression_type = cmArchiveWrite::CompressNone;
  } else {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error unrecognized compression type: "
                    << debian_compression_type << std::endl);
  }

  const char* debian_archive_type =
    this->GetOption("GEN_CPACK_DEBIAN_ARCHIVE_TYPE");
  if (!debian_archive_type) {
    debian_archive_type = "paxr";
  }

  std::string filename_data_tar =
    strGenWDIR + "/data.tar" + compression_suffix;

  // atomic file generation for data.tar
  {
    cmGeneratedFileStream fileStream_data_tar;
    fileStream_data_tar.Open(filename_data_tar.c_str(), false, true);
    if (!fileStream_data_tar) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Error opening the file \""
                      << filename_data_tar << "\" for writing" << std::endl);
      return 0;
    }
    cmArchiveWrite data_tar(fileStream_data_tar, tar_compression_type,
                            debian_archive_type);

    // uid/gid should be the one of the root user, and this root user has
    // always uid/gid equal to 0.
    data_tar.SetUIDAndGID(0u, 0u);
    data_tar.SetUNAMEAndGNAME("root", "root");

    // now add all directories which have to be compressed
    // collect all top level install dirs for that
    // e.g. /opt/bin/foo, /usr/bin/bar and /usr/bin/baz would
    // give /usr and /opt
    size_t topLevelLength = strGenWDIR.length();
    cmCPackLogger(cmCPackLog::LOG_DEBUG, "WDIR: \""
                    << strGenWDIR << "\", length = " << topLevelLength
                    << std::endl);
    std::set<std::string> orderedFiles;

    // we have to reconstruct the parent folders as well

    for (std::vector<std::string>::const_iterator fileIt =
           packageFiles.begin();
         fileIt != packageFiles.end(); ++fileIt) {
      std::string currentPath = *fileIt;
      while (currentPath != strGenWDIR) {
        // the last one IS strGenWDIR, but we do not want this one:
        // XXX/application/usr/bin/myprogram with GEN_WDIR=XXX/application
        // should not add XXX/application
        orderedFiles.insert(currentPath);
        currentPath = cmSystemTools::CollapseCombinedPath(currentPath, "..");
      }
    }

    for (std::set<std::string>::const_iterator fileIt = orderedFiles.begin();
         fileIt != orderedFiles.end(); ++fileIt) {
      cmCPackLogger(cmCPackLog::LOG_DEBUG, "FILEIT: \"" << *fileIt << "\""
                                                        << std::endl);
      std::string::size_type slashPos = fileIt->find('/', topLevelLength + 1);
      std::string relativeDir =
        fileIt->substr(topLevelLength, slashPos - topLevelLength);
      cmCPackLogger(cmCPackLog::LOG_DEBUG, "RELATIVEDIR: \""
                      << relativeDir << "\"" << std::endl);

      // do not recurse because the loop will do it
      if (!data_tar.Add(*fileIt, topLevelLength, ".", false)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR, "Problem adding file to tar:"
                        << std::endl
                        << "#top level directory: " << strGenWDIR << std::endl
                        << "#file: " << *fileIt << std::endl
                        << "#error:" << data_tar.GetError() << std::endl);
        return 0;
      }
    }
  } // scope for file generation

  std::string md5filename = strGenWDIR + "/md5sums";
  {
    // the scope is needed for cmGeneratedFileStream
    cmGeneratedFileStream out(md5filename.c_str());

    std::string topLevelWithTrailingSlash =
      this->GetOption("CPACK_TEMPORARY_DIRECTORY");
    topLevelWithTrailingSlash += '/';
    for (std::vector<std::string>::const_iterator fileIt =
           packageFiles.begin();
         fileIt != packageFiles.end(); ++fileIt) {
      // hash only regular files
      if (cmSystemTools::FileIsDirectory(*fileIt) ||
          cmSystemTools::FileIsSymlink(*fileIt)) {
        continue;
      }

      char md5sum[33];
      if (!cmSystemTools::ComputeFileMD5(*fileIt, md5sum)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR, "Problem computing the md5 of "
                        << *fileIt << std::endl);
      }

      md5sum[32] = 0;

      std::string output(md5sum);
      output += "  " + *fileIt + "\n";
      // debian md5sums entries are like this:
      // 014f3604694729f3bf19263bac599765  usr/bin/ccmake
      // thus strip the full path (with the trailing slash)
      cmSystemTools::ReplaceString(output, topLevelWithTrailingSlash.c_str(),
                                   "");
      out << output;
    }
    // each line contains a eol.
    // Do not end the md5sum file with yet another (invalid)
  }

  std::string filename_control_tar = strGenWDIR + "/control.tar.gz";
  // atomic file generation for control.tar
  {
    cmGeneratedFileStream fileStream_control_tar;
    fileStream_control_tar.Open(filename_control_tar.c_str(), false, true);
    if (!fileStream_control_tar) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Error opening the file \""
                      << filename_control_tar << "\" for writing"
                      << std::endl);
      return 0;
    }
    cmArchiveWrite control_tar(fileStream_control_tar,
                               cmArchiveWrite::CompressGZip,
                               debian_archive_type);

    // sets permissions and uid/gid for the files
    control_tar.SetUIDAndGID(0u, 0u);
    control_tar.SetUNAMEAndGNAME("root", "root");

    /* permissions are set according to
    https://www.debian.org/doc/debian-policy/ch-files.html#s-permissions-owners
    and
    https://lintian.debian.org/tags/control-file-has-bad-permissions.html
    */
    const mode_t permission644 = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    const mode_t permissionExecute = S_IXUSR | S_IXGRP | S_IXOTH;
    const mode_t permission755 = permission644 | permissionExecute;

    // for md5sum and control (that we have generated here), we use 644
    // (RW-R--R--)
    // so that deb lintian doesn't warn about it
    control_tar.SetPermissions(permission644);

    // adds control and md5sums
    if (!control_tar.Add(md5filename, strGenWDIR.length(), ".") ||
        !control_tar.Add(strGenWDIR + "/control", strGenWDIR.length(), ".")) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Error adding file to tar:"
                      << std::endl
                      << "#top level directory: " << strGenWDIR << std::endl
                      << "#file: \"control\" or \"md5sums\"" << std::endl
                      << "#error:" << control_tar.GetError() << std::endl);
      return 0;
    }

    // adds generated shlibs file
    if (gen_shibs) {
      if (!control_tar.Add(shlibsfilename, strGenWDIR.length(), ".")) {
        cmCPackLogger(cmCPackLog::LOG_ERROR, "Error adding file to tar:"
                        << std::endl
                        << "#top level directory: " << strGenWDIR << std::endl
                        << "#file: \"shlibs\"" << std::endl
                        << "#error:" << control_tar.GetError() << std::endl);
        return 0;
      }
    }

    // adds LDCONFIG related files
    if (this->IsOn("GEN_CPACK_DEBIAN_GENERATE_POSTINST")) {
      control_tar.SetPermissions(permission755);
      if (!control_tar.Add(postinst, strGenWDIR.length(), ".")) {
        cmCPackLogger(cmCPackLog::LOG_ERROR, "Error adding file to tar:"
                        << std::endl
                        << "#top level directory: " << strGenWDIR << std::endl
                        << "#file: \"postinst\"" << std::endl
                        << "#error:" << control_tar.GetError() << std::endl);
        return 0;
      }
      control_tar.SetPermissions(permission644);
    }

    if (this->IsOn("GEN_CPACK_DEBIAN_GENERATE_POSTRM")) {
      control_tar.SetPermissions(permission755);
      if (!control_tar.Add(postrm, strGenWDIR.length(), ".")) {
        cmCPackLogger(cmCPackLog::LOG_ERROR, "Error adding file to tar:"
                        << std::endl
                        << "#top level directory: " << strGenWDIR << std::endl
                        << "#file: \"postinst\"" << std::endl
                        << "#error:" << control_tar.GetError() << std::endl);
        return 0;
      }
      control_tar.SetPermissions(permission644);
    }

    // for the other files, we use
    // -either the original permission on the files
    // -either a permission strictly defined by the Debian policies
    const char* controlExtra =
      this->GetOption("GEN_CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA");
    if (controlExtra) {
      // permissions are now controlled by the original file permissions

      const bool permissionStrictPolicy =
        this->IsSet("GEN_CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION");

      static const char* strictFiles[] = { "config", "postinst", "postrm",
                                           "preinst", "prerm" };
      std::set<std::string> setStrictFiles(
        strictFiles,
        strictFiles + sizeof(strictFiles) / sizeof(strictFiles[0]));

      // default
      control_tar.ClearPermissions();

      std::vector<std::string> controlExtraList;
      cmSystemTools::ExpandListArgument(controlExtra, controlExtraList);
      for (std::vector<std::string>::iterator i = controlExtraList.begin();
           i != controlExtraList.end(); ++i) {
        std::string filenamename = cmsys::SystemTools::GetFilenameName(*i);
        std::string localcopy = strGenWDIR + "/" + filenamename;

        if (permissionStrictPolicy) {
          control_tar.SetPermissions(setStrictFiles.count(filenamename)
                                       ? permission755
                                       : permission644);
        }

        // if we can copy the file, it means it does exist, let's add it:
        if (cmsys::SystemTools::CopyFileIfDifferent(*i, localcopy)) {
          control_tar.Add(localcopy, strGenWDIR.length(), ".");
        }
      }
    }
  }

  // ar -r your-package-name.deb debian-binary control.tar.* data.tar.*
  // since debian packages require BSD ar (most Linux distros and even
  // FreeBSD and NetBSD ship GNU ar) we use a copy of OpenBSD ar here.
  std::vector<std::string> arFiles;
  std::string topLevelString = strGenWDIR + "/";
  arFiles.push_back(topLevelString + "debian-binary");
  arFiles.push_back(topLevelString + "control.tar.gz");
  arFiles.push_back(topLevelString + "data.tar" + compression_suffix);
  std::string outputFileName = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  outputFileName += "/";
  outputFileName += this->GetOption("GEN_CPACK_OUTPUT_FILE_NAME");
  int res = ar_append(outputFileName.c_str(), arFiles);
  if (res != 0) {
    std::string tmpFile =
      this->GetOption("GEN_CPACK_TEMPORARY_PACKAGE_FILE_NAME");
    tmpFile += "/Deb.log";
    cmGeneratedFileStream ofs(tmpFile.c_str());
    ofs << "# Problem creating archive using: " << res << std::endl;
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
  if (CM_NULLPTR != GetOption(groupVar)) {
    return std::string(GetOption(groupVar));
  }
  return componentName;
}

// The following code is taken from OpenBSD ar:
// http://www.openbsd.org/cgi-bin/cvsweb/src/usr.bin/ar/
// It has been slightly modified:
// -return error codes instead exit() in functions
// -use the stdio file I/O functions instead the file descriptor based ones
// -merged into one cxx file
// -no additional options supported
// The coding style hasn't been modified.

/*-
 * Copyright (c) 1990, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hugh Smith at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define ARMAG "!<arch>\n" /* ar "magic number" */
#define SARMAG 8          /* strlen(ARMAG); */

#define AR_EFMT1 "#1/" /* extended format #1 */
#define ARFMAG "`\n"

/* Header format strings. */
#define HDR1 "%s%-13d%-12ld%-6u%-6u%-8o%-10lld%2s"
#define HDR2 "%-16.16s%-12ld%-6u%-6u%-8o%-10lld%2s"

struct ar_hdr
{
  char ar_name[16]; /* name */
  char ar_date[12]; /* modification time */
  char ar_uid[6];   /* user id */
  char ar_gid[6];   /* group id */
  char ar_mode[8];  /* octal file permissions */
  char ar_size[10]; /* size in bytes */
  char ar_fmag[2];  /* consistency check */
};

/* Set up file copy. */
#define SETCF(from, fromname, to, toname, pad)                                \
  {                                                                           \
    cf.rFile = from;                                                          \
    cf.rname = fromname;                                                      \
    cf.wFile = to;                                                            \
    cf.wname = toname;                                                        \
    cf.flags = pad;                                                           \
  }

/* File copy structure. */
typedef struct
{
  FILE* rFile;        /* read file descriptor */
  const char* rname;  /* read name */
  FILE* wFile;        /* write file descriptor */
  const char* wname;  /* write name */
#define NOPAD 0x00    /* don't pad */
#define WPAD 0x02     /* pad on writes */
  unsigned int flags; /* pad flags */
} CF;

/* misc.c */

static const char* ar_rname(const char* path)
{
  const char* ind = strrchr(path, '/');
  return (ind) ? ind + 1 : path;
}

/* archive.c */

typedef struct ar_hdr HDR;
static char ar_hb[sizeof(HDR) + 1]; /* real header */

static size_t ar_already_written;

/* copy_ar --
 *      Copy size bytes from one file to another - taking care to handle the
 *      extra byte (for odd size files) when reading archives and writing an
 *      extra byte if necessary when adding files to archive.  The length of
 *      the object is the long name plus the object itself; the variable
 *      already_written gets set if a long name was written.
 *
 *      The padding is really unnecessary, and is almost certainly a remnant
 *      of early archive formats where the header included binary data which
 *      a PDP-11 required to start on an even byte boundary.  (Or, perhaps,
 *      because 16-bit word addressed copies were faster?)  Anyhow, it should
 *      have been ripped out long ago.
 */
static int copy_ar(CF* cfp, off_t size)
{
  static char pad = '\n';
  off_t sz = size;
  size_t nr, nw;
  char buf[8 * 1024];

  if (sz == 0) {
    return 0;
  }

  FILE* from = cfp->rFile;
  FILE* to = cfp->wFile;
  while (sz &&
         (nr = fread(buf, 1, sz < static_cast<off_t>(sizeof(buf))
                       ? static_cast<size_t>(sz)
                       : sizeof(buf),
                     from)) > 0) {
    sz -= nr;
    for (size_t off = 0; off < nr; nr -= off, off += nw) {
      if ((nw = fwrite(buf + off, 1, nr, to)) < nr) {
        return -1;
      }
    }
  }
  if (sz) {
    return -2;
  }

  if (cfp->flags & WPAD && (size + ar_already_written) & 1 &&
      fwrite(&pad, 1, 1, to) != 1) {
    return -4;
  }

  return 0;
}

/* put_arobj --  Write an archive member to a file. */
static int put_arobj(CF* cfp, struct stat* sb)
{
  int result = 0;
  struct ar_hdr* hdr;

  /* If passed an sb structure, reading a file from disk.  Get stat(2)
   * information, build a name and construct a header.  (Files are named
   * by their last component in the archive.) */
  const char* name = ar_rname(cfp->rname);
  (void)stat(cfp->rname, sb);

  /* If not truncating names and the name is too long or contains
   * a space, use extended format 1.   */
  size_t lname = strlen(name);
  uid_t uid = sb->st_uid;
  gid_t gid = sb->st_gid;
  if (uid > USHRT_MAX) {
    uid = USHRT_MAX;
  }
  if (gid > USHRT_MAX) {
    gid = USHRT_MAX;
  }
  if (lname > sizeof(hdr->ar_name) || strchr(name, ' ')) {
    (void)sprintf(ar_hb, HDR1, AR_EFMT1, (int)lname, (long int)sb->st_mtime,
                  (unsigned)uid, (unsigned)gid, (unsigned)sb->st_mode,
                  (long long)sb->st_size + lname, ARFMAG);
  } else {
    lname = 0;
    (void)sprintf(ar_hb, HDR2, name, (long int)sb->st_mtime, (unsigned)uid,
                  (unsigned)gid, (unsigned)sb->st_mode, (long long)sb->st_size,
                  ARFMAG);
  }
  off_t size = sb->st_size;

  if (fwrite(ar_hb, 1, sizeof(HDR), cfp->wFile) != sizeof(HDR)) {
    return -1;
  }

  if (lname) {
    if (fwrite(name, 1, lname, cfp->wFile) != lname) {
      return -2;
    }
    ar_already_written = lname;
  }
  result = copy_ar(cfp, size);
  ar_already_written = 0;
  return result;
}

/* append.c */

/* append --
 *      Append files to the archive - modifies original archive or creates
 *      a new archive if named archive does not exist.
 */
static int ar_append(const char* archive,
                     const std::vector<std::string>& files)
{
  int eval = 0;
  FILE* aFile = cmSystemTools::Fopen(archive, "wb+");
  if (aFile != CM_NULLPTR) {
    fwrite(ARMAG, SARMAG, 1, aFile);
    if (fseek(aFile, 0, SEEK_END) != -1) {
      CF cf;
      struct stat sb;
      /* Read from disk, write to an archive; pad on write. */
      SETCF(CM_NULLPTR, CM_NULLPTR, aFile, archive, WPAD);
      for (std::vector<std::string>::const_iterator fileIt = files.begin();
           fileIt != files.end(); ++fileIt) {
        const char* filename = fileIt->c_str();
        FILE* file = cmSystemTools::Fopen(filename, "rb");
        if (file == CM_NULLPTR) {
          eval = -1;
          continue;
        }
        cf.rFile = file;
        cf.rname = filename;
        int result = put_arobj(&cf, &sb);
        (void)fclose(file);
        if (result != 0) {
          eval = -2;
          break;
        }
      }
    } else {
      eval = -3;
    }
    fclose(aFile);
  } else {
    eval = -4;
  }
  return eval;
}
