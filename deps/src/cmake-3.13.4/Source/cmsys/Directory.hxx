/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_Directory_hxx
#define cmsys_Directory_hxx

#include <cmsys/Configure.h>

#include <string>

namespace cmsys {

class DirectoryInternals;

/** \class Directory
 * \brief Portable directory/filename traversal.
 *
 * Directory provides a portable way of finding the names of the files
 * in a system directory.
 *
 * Directory currently works with Windows and Unix operating systems.
 */
class cmsys_EXPORT Directory
{
public:
  Directory();
  ~Directory();

  /**
   * Load the specified directory and load the names of the files
   * in that directory. 0 is returned if the directory can not be
   * opened, 1 if it is opened.
   */
  bool Load(const std::string&);

  /**
   * Return the number of files in the current directory.
   */
  unsigned long GetNumberOfFiles() const;

  /**
   * Return the number of files in the specified directory.
   * A higher performance static method.
   */
  static unsigned long GetNumberOfFilesInDirectory(const std::string&);

  /**
   * Return the file at the given index, the indexing is 0 based
   */
  const char* GetFile(unsigned long) const;

  /**
   * Return the path to Open'ed directory
   */
  const char* GetPath() const;

  /**
   * Clear the internal structure. Used internally at beginning of Load(...)
   * to clear the cache.
   */
  void Clear();

private:
  // Private implementation details.
  DirectoryInternals* Internal;

  Directory(const Directory&);      // Not implemented.
  void operator=(const Directory&); // Not implemented.
};                                  // End Class: Directory

} // namespace cmsys

#endif
