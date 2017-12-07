/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmELF_h
#define cmELF_h

#include "cmConfigure.h"

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#if !defined(CMAKE_USE_ELF_PARSER)
#error "This file may be included only if CMAKE_USE_ELF_PARSER is enabled."
#endif

class cmELFInternal;

/** \class cmELF
 * \brief Executable and Link Format (ELF) parser.
 */
class cmELF
{
public:
  /** Construct with the name of the ELF input file to parse.  */
  cmELF(const char* fname);

  /** Destruct.   */
  ~cmELF();

  /** Get the error message if any.  */
  std::string const& GetErrorMessage() const { return this->ErrorMessage; }

  /** Boolean conversion.  True if the ELF file is valid.  */
  operator bool() const { return this->Valid(); }

  /** Enumeration of ELF file types.  */
  enum FileType
  {
    FileTypeInvalid,
    FileTypeRelocatableObject,
    FileTypeExecutable,
    FileTypeSharedLibrary,
    FileTypeCore,
    FileTypeSpecificOS,
    FileTypeSpecificProc
  };

  /** Represent string table entries.  */
  struct StringEntry
  {
    // The string value itself.
    std::string Value;

    // The position in the file at which the string appears.
    unsigned long Position;

    // The size of the string table entry.  This includes the space
    // allocated for one or more null terminators.
    unsigned long Size;

    // The index of the section entry referencing the string.
    int IndexInSection;
  };

  /** Represent entire dynamic section header */
  typedef std::vector<std::pair<long, unsigned long> > DynamicEntryList;

  /** Get the type of the file opened.  */
  FileType GetFileType() const;

  /** Get the number of ELF sections present.  */
  unsigned int GetNumberOfSections() const;

  /** Get the position of a DYNAMIC section header entry.  Returns
      zero on error.  */
  unsigned long GetDynamicEntryPosition(int index) const;

  /** Get a copy of all the DYNAMIC section header entries.
      Returns an empty vector on error */
  DynamicEntryList GetDynamicEntries() const;

  /** Encodes a DYNAMIC section header entry list into a char vector according
      to the type of ELF file this is */
  std::vector<char> EncodeDynamicEntries(
    const DynamicEntryList& entries) const;

  /** Get the SONAME field if any.  */
  bool GetSOName(std::string& soname);
  StringEntry const* GetSOName();

  /** Get the RPATH field if any.  */
  StringEntry const* GetRPath();

  /** Get the RUNPATH field if any.  */
  StringEntry const* GetRunPath();

  /** Print human-readable information about the ELF file.  */
  void PrintInfo(std::ostream& os) const;

  /** Interesting dynamic tags.
      If the tag is 0, it does not exist in the host ELF implementation */
  static const long TagRPath, TagRunPath, TagMipsRldMapRel;

private:
  friend class cmELFInternal;
  bool Valid() const;
  cmELFInternal* Internal;
  std::string ErrorMessage;
};

#endif
