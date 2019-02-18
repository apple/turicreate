/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackFreeBSDGenerator.h"

#include "cmArchiveWrite.h"
#include "cmCPackArchiveGenerator.h"
#include "cmCPackLog.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"

// Needed for ::open() and ::stat()
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <pkg.h>

#include <algorithm>

cmCPackFreeBSDGenerator::cmCPackFreeBSDGenerator()
  : cmCPackArchiveGenerator(cmArchiveWrite::CompressXZ, "paxr")
{
}

int cmCPackFreeBSDGenerator::InitializeInternal()
{
  this->SetOptionIfNotSet("CPACK_PACKAGING_INSTALL_PREFIX", "/usr/local");
  this->SetOption("CPACK_INCLUDE_TOPLEVEL_DIRECTORY", "0");
  return this->Superclass::InitializeInternal();
}

cmCPackFreeBSDGenerator::~cmCPackFreeBSDGenerator()
{
}

// This is a wrapper, for use only in stream-based output,
// that will output a string in UCL escaped fashion (in particular,
// quotes and backslashes are escaped). The list of characters
// to escape is taken from https://github.com/vstakhov/libucl
// (which is the reference implementation pkg(8) refers to).
class EscapeQuotes
{
public:
  const std::string& value;

  EscapeQuotes(const std::string& s)
    : value(s)
  {
  }
};

// Output a string as "string" with escaping applied.
cmGeneratedFileStream& operator<<(cmGeneratedFileStream& s,
                                  const EscapeQuotes& v)
{
  s << '"';
  for (std::string::size_type i = 0; i < v.value.length(); ++i) {
    char c = v.value[i];
    switch (c) {
      case '\n':
        s << "\\n";
        break;
      case '\r':
        s << "\\r";
        break;
      case '\b':
        s << "\\b";
        break;
      case '\t':
        s << "\\t";
        break;
      case '\f':
        s << "\\f";
        break;
      case '\\':
        s << "\\\\";
        break;
      case '"':
        s << "\\\"";
        break;
      default:
        s << c;
        break;
    }
  }
  s << '"';
  return s;
}

// The following classes are all helpers for writing out the UCL
// manifest file (it also looks like JSON). ManifestKey just has
// a (string-valued) key; subclasses add a specific kind of
// value-type to the key, and implement write_value() to output
// the corresponding UCL.
class ManifestKey
{
public:
  std::string key;

  ManifestKey(const std::string& k)
    : key(k)
  {
  }

  virtual ~ManifestKey() {}

  // Output the value associated with this key to the stream @p s.
  // Format is to be decided by subclasses.
  virtual void write_value(cmGeneratedFileStream& s) const = 0;
};

// Basic string-value (e.g. "name": "cmake")
class ManifestKeyValue : public ManifestKey
{
public:
  std::string value;

  ManifestKeyValue(const std::string& k, const std::string& v)
    : ManifestKey(k)
    , value(v)
  {
  }

  void write_value(cmGeneratedFileStream& s) const override
  {
    s << EscapeQuotes(value);
  }
};

// List-of-strings values (e.g. "licenses": ["GPLv2", "LGPLv2"])
class ManifestKeyListValue : public ManifestKey
{
public:
  typedef std::vector<std::string> VList;
  VList value;

  ManifestKeyListValue(const std::string& k)
    : ManifestKey(k)
  {
  }

  ManifestKeyListValue& operator<<(const std::string& v)
  {
    value.push_back(v);
    return *this;
  }

  ManifestKeyListValue& operator<<(const std::vector<std::string>& v)
  {
    for (VList::const_iterator it = v.begin(); it != v.end(); ++it) {
      (*this) << (*it);
    }
    return *this;
  }

  void write_value(cmGeneratedFileStream& s) const override
  {
    bool with_comma = false;

    s << '[';
    for (VList::const_iterator it = value.begin(); it != value.end(); ++it) {
      s << (with_comma ? ',' : ' ');
      s << EscapeQuotes(*it);
      with_comma = true;
    }
    s << " ]";
  }
};

// Deps: actually a dictionary, but we'll treat it as a
// list so we only name the deps, and produce dictionary-
// like output via write_value()
class ManifestKeyDepsValue : public ManifestKeyListValue
{
public:
  ManifestKeyDepsValue(const std::string& k)
    : ManifestKeyListValue(k)
  {
  }

  void write_value(cmGeneratedFileStream& s) const override
  {
    s << "{\n";
    for (VList::const_iterator it = value.begin(); it != value.end(); ++it) {
      s << "  \"" << *it << "\": {\"origin\": \"" << *it << "\"},\n";
    }
    s << '}';
  }
};

// Write one of the key-value classes (above) to the stream @p s
cmGeneratedFileStream& operator<<(cmGeneratedFileStream& s,
                                  const ManifestKey& v)
{
  s << '"' << v.key << "\": ";
  v.write_value(s);
  s << ",\n";
  return s;
}

// Look up variable; if no value is set, returns an empty string;
// basically a wrapper that handles the NULL-ptr return from GetOption().
std::string cmCPackFreeBSDGenerator::var_lookup(const char* var_name)
{
  const char* pv = this->GetOption(var_name);
  if (!pv) {
    return std::string();
  } else {
    return pv;
  }
}

// Produce UCL in the given @p manifest file for the common
// manifest fields (common to the compact and regular formats),
// by reading the CPACK_FREEBSD_* variables.
void cmCPackFreeBSDGenerator::write_manifest_fields(
  cmGeneratedFileStream& manifest)
{
  manifest << ManifestKeyValue("name",
                               var_lookup("CPACK_FREEBSD_PACKAGE_NAME"));
  manifest << ManifestKeyValue("origin",
                               var_lookup("CPACK_FREEBSD_PACKAGE_ORIGIN"));
  manifest << ManifestKeyValue("version",
                               var_lookup("CPACK_FREEBSD_PACKAGE_VERSION"));
  manifest << ManifestKeyValue("maintainer",
                               var_lookup("CPACK_FREEBSD_PACKAGE_MAINTAINER"));
  manifest << ManifestKeyValue("comment",
                               var_lookup("CPACK_FREEBSD_PACKAGE_COMMENT"));
  manifest << ManifestKeyValue(
    "desc", var_lookup("CPACK_FREEBSD_PACKAGE_DESCRIPTION"));
  manifest << ManifestKeyValue("www", var_lookup("CPACK_FREEBSD_PACKAGE_WWW"));
  std::vector<std::string> licenses;
  cmSystemTools::ExpandListArgument(
    var_lookup("CPACK_FREEBSD_PACKAGE_LICENSE"), licenses);
  std::string licenselogic("single");
  if (licenses.size() < 1) {
    cmSystemTools::SetFatalErrorOccured();
  } else if (licenses.size() > 1) {
    licenselogic = var_lookup("CPACK_FREEBSD_PACKAGE_LICENSE_LOGIC");
  }
  manifest << ManifestKeyValue("licenselogic", licenselogic);
  manifest << (ManifestKeyListValue("licenses") << licenses);
  std::vector<std::string> categories;
  cmSystemTools::ExpandListArgument(
    var_lookup("CPACK_FREEBSD_PACKAGE_CATEGORIES"), categories);
  manifest << (ManifestKeyListValue("categories") << categories);
  manifest << ManifestKeyValue("prefix", var_lookup("CMAKE_INSTALL_PREFIX"));
  std::vector<std::string> deps;
  cmSystemTools::ExpandListArgument(var_lookup("CPACK_FREEBSD_PACKAGE_DEPS"),
                                    deps);
  if (deps.size() > 0) {
    manifest << (ManifestKeyDepsValue("deps") << deps);
  }
}

// Package only actual files; others are ignored (in particular,
// intermediate subdirectories are ignored).
static bool ignore_file(const std::string& filename)
{
  struct stat statbuf;

  if (!((stat(filename.c_str(), &statbuf) >= 0) &&
        ((statbuf.st_mode & S_IFMT) == S_IFREG))) {
    return true;
  }
  // May be other reasons to return false
  return false;
}

// Write the given list of @p files to the manifest stream @p s,
// as the UCL field "files" (which is dictionary-valued, to
// associate filenames with hashes). All the files are transformed
// to paths relative to @p toplevel, with a leading / (since the paths
// in FreeBSD package files are supposed to be absolute).
void write_manifest_files(cmGeneratedFileStream& s,
                          const std::string& toplevel,
                          const std::vector<std::string>& files)
{
  const char* c_toplevel = toplevel.c_str();
  std::vector<std::string>::const_iterator it;

  s << "\"files\": {\n";
  for (it = files.begin(); it != files.end(); ++it) {
    s << "  \"/" << cmSystemTools::RelativePath(c_toplevel, it->c_str())
      << "\": \""
      << "<sha256>"
      << "\",\n";
  }
  s << "  },\n";
}

static bool has_suffix(const std::string& str, const std::string& suffix)
{
  return str.size() >= suffix.size() &&
    str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int cmCPackFreeBSDGenerator::PackageFiles()
{
  if (!this->ReadListFile("Internal/CPack/CPackFreeBSD.cmake")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error while execution CPackFreeBSD.cmake" << std::endl);
    return 0;
  }

  std::vector<std::string>::const_iterator fileIt;
  std::string dir = cmSystemTools::GetCurrentWorkingDirectory();
  cmSystemTools::ChangeDirectory(toplevel);

  files.erase(std::remove_if(files.begin(), files.end(), ignore_file),
              files.end());

  std::string manifestname = toplevel + "/+MANIFEST";
  {
    cmGeneratedFileStream manifest(manifestname.c_str());
    manifest << "{\n";
    write_manifest_fields(manifest);
    write_manifest_files(manifest, toplevel, files);
    manifest << "}\n";
  }

  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Toplevel: " << toplevel << std::endl);

  if (WantsComponentInstallation()) {
    // CASE 1 : COMPONENT ALL-IN-ONE package
    // If ALL COMPONENTS in ONE package has been requested
    // then the package file is unique and should be open here.
    if (componentPackageMethod == ONE_PACKAGE) {
      return PackageComponentsAllInOne();
    }
    // CASE 2 : COMPONENT CLASSICAL package(s) (i.e. not all-in-one)
    // There will be 1 package for each component group
    // however one may require to ignore component group and
    // in this case you'll get 1 package for each component.
    return PackageComponents(componentPackageMethod ==
                             ONE_PACKAGE_PER_COMPONENT);
  }

  std::string output_dir =
    cmSystemTools::CollapseCombinedPath(toplevel, "../");
  pkg_create_from_manifest(output_dir.c_str(), ::TXZ, toplevel.c_str(),
                           manifestname.c_str(), NULL);

  std::string broken_suffix = std::string("-") +
    var_lookup("CPACK_TOPLEVEL_TAG") + std::string(GetOutputExtension());
  for (std::vector<std::string>::iterator it = packageFileNames.begin();
       it != packageFileNames.end(); ++it) {
    cmCPackLogger(cmCPackLog::LOG_DEBUG, "Packagefile " << *it << std::endl);
    if (has_suffix(*it, broken_suffix)) {
      it->replace(it->size() - broken_suffix.size(), std::string::npos,
                  GetOutputExtension());
      break;
    }
  }

  cmSystemTools::ChangeDirectory(dir);
  return 1;
}
