/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSourceFile_h
#define cmSourceFile_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmPropertyMap.h"
#include "cmSourceFileLocation.h"
#include "cmSourceFileLocationKind.h"

#include <string>
#include <vector>

class cmCustomCommand;
class cmMakefile;

/** \class cmSourceFile
 * \brief Represent a class loaded from a makefile.
 *
 * cmSourceFile is represents a class loaded from
 * a makefile.
 */
class cmSourceFile
{
public:
  /**
   * Construct with the makefile storing the source and the initial
   * name referencing it.
   */
  cmSourceFile(
    cmMakefile* mf, const std::string& name,
    cmSourceFileLocationKind kind = cmSourceFileLocationKind::Ambiguous);

  ~cmSourceFile();

  /**
   * Get the list of the custom commands for this source file
   */
  cmCustomCommand* GetCustomCommand();
  cmCustomCommand const* GetCustomCommand() const;
  void SetCustomCommand(cmCustomCommand* cc);

  ///! Set/Get a property of this source file
  void SetProperty(const std::string& prop, const char* value);
  void AppendProperty(const std::string& prop, const char* value,
                      bool asString = false);
  ///! Might return a nullptr if the property is not set or invalid
  const char* GetProperty(const std::string& prop) const;
  ///! Always returns a valid pointer
  const char* GetSafeProperty(const std::string& prop) const;
  bool GetPropertyAsBool(const std::string& prop) const;

  /** Implement getting a property when called from a CMake language
      command like get_property or get_source_file_property.  */
  const char* GetPropertyForUser(const std::string& prop);

  /**
   * The full path to the file.  The non-const version of this method
   * may attempt to locate the file on disk and finalize its location.
   * The const version of this method may return an empty string if
   * the non-const version has not yet been called (yes this is a
   * horrible interface, but is necessary for backwards
   * compatibility).
   */
  std::string const& GetFullPath(std::string* error = nullptr);
  std::string const& GetFullPath() const;

  /**
   * Get the information currently known about the source file
   * location without attempting to locate the file as GetFullPath
   * would.  See cmSourceFileLocation documentation.
   */
  cmSourceFileLocation const& GetLocation() const;

  /**
   * Get the file extension of this source file.
   */
  std::string const& GetExtension() const;

  /**
   * Get the language of the compiler to use for this source file.
   */
  std::string GetLanguage();
  std::string GetLanguage() const;

  /**
   * Return the vector that holds the list of dependencies
   */
  const std::vector<std::string>& GetDepends() const { return this->Depends; }
  void AddDepend(const std::string& d) { this->Depends.push_back(d); }

  // Get the properties
  cmPropertyMap& GetProperties() { return this->Properties; }
  const cmPropertyMap& GetProperties() const { return this->Properties; }

  /**
   * Check whether the given source file location could refer to this
   * source.
   */
  bool Matches(cmSourceFileLocation const&);

  void SetObjectLibrary(std::string const& objlib);
  std::string GetObjectLibrary() const;

private:
  cmSourceFileLocation Location;
  cmPropertyMap Properties;
  cmCustomCommand* CustomCommand;
  std::string Extension;
  std::string Language;
  std::string FullPath;
  std::string ObjectLibrary;
  std::vector<std::string> Depends;
  bool FindFullPathFailed;

  bool FindFullPath(std::string* error);
  bool TryFullPath(const std::string& path, const std::string& ext);
  void CheckExtension();
  void CheckLanguage(std::string const& ext);

  static const std::string propLANGUAGE;
};

// TODO: Factor out into platform information modules.
#define CM_HEADER_REGEX "\\.(h|hh|h\\+\\+|hm|hpp|hxx|in|txx|inl)$"

#define CM_SOURCE_REGEX                                                       \
  "\\.(C|M|c|c\\+\\+|cc|cpp|cxx|cu|f|f90|for|fpp|ftn|m|mm|rc|def|r|odl|idl|"  \
  "hpj"                                                                       \
  "|bat)$"

#define CM_RESOURCE_REGEX "\\.(pdf|plist|png|jpeg|jpg|storyboard|xcassets)$"

#endif
