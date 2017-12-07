/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSourceFileLocation_h
#define cmSourceFileLocation_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

class cmMakefile;

/** \class cmSourceFileLocation
 * \brief cmSourceFileLocation tracks knowledge about a source file location
 *
 * Source files can be referenced by a variety of names.  The
 * directory and/or extension may be omitted leading to a certain
 * level of ambiguity about the source file location.  This class is
 * used by cmSourceFile to keep track of what is known about the
 * source file location.  Each reference may add some information
 * about the directory or extension of the file.
 */
class cmSourceFileLocation
{
public:
  /**
   * Construct for a source file created in a given cmMakefile
   * instance with an initial name.
   */
  cmSourceFileLocation(cmMakefile const* mf, const std::string& name);
  cmSourceFileLocation();
  cmSourceFileLocation(const cmSourceFileLocation& loc);
  cmSourceFileLocation& operator=(const cmSourceFileLocation& loc);

  /**
   * Return whether the given source file location could refers to the
   * same source file as this location given the level of ambiguity in
   * each location.
   */
  bool Matches(cmSourceFileLocation const& loc);

  /**
   * Explicity state that the source file is located in the source tree.
   */
  void DirectoryUseSource();

  /**
   * Explicity state that the source file is located in the build tree.
   */
  void DirectoryUseBinary();

  /**
   * Return whether the directory containing the source is ambiguous.
   */
  bool DirectoryIsAmbiguous() const { return this->AmbiguousDirectory; }

  /**
   * Return whether the extension of the source name is ambiguous.
   */
  bool ExtensionIsAmbiguous() const { return this->AmbiguousExtension; }

  /**
   * Get the directory containing the file as best is currently known.
   * If DirectoryIsAmbiguous() returns false this will be a full path.
   * Otherwise it will be a relative path (possibly empty) that is
   * either with respect to the source or build tree.
   */
  const std::string& GetDirectory() const { return this->Directory; }

  /**
   * Get the file name as best is currently known.  If
   * ExtensionIsAmbiguous() returns true this name may not be the
   * final name (but could be).  Otherwise the returned name is the
   * final name.
   */
  const std::string& GetName() const { return this->Name; }

  /**
   * Get the cmMakefile instance for which the source file was created.
   */
  cmMakefile const* GetMakefile() const { return this->Makefile; }
private:
  cmMakefile const* Makefile;
  bool AmbiguousDirectory;
  bool AmbiguousExtension;
  std::string Directory;
  std::string Name;

  bool MatchesAmbiguousExtension(cmSourceFileLocation const& loc) const;

  // Update the location with additional knowledge.
  void Update(cmSourceFileLocation const& loc);
  void UpdateExtension(const std::string& name);
};

#endif
