/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmArchiveWrite_h
#define cmArchiveWrite_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <stddef.h>
#include <string>

#if !defined(CMAKE_BUILD_WITH_CMAKE)
#error "cmArchiveWrite not allowed during bootstrap build!"
#endif

template <typename T>
class cmArchiveWriteOptional
{
public:
  cmArchiveWriteOptional() { this->Clear(); }
  explicit cmArchiveWriteOptional(T val) { this->Set(val); }

  void Set(T val)
  {
    this->IsValueSet = true;
    this->Value = val;
  }
  void Clear() { this->IsValueSet = false; }
  bool IsSet() const { return this->IsValueSet; }
  T Get() const { return Value; }
private:
  T Value;
  bool IsValueSet;
};

/** \class cmArchiveWrite
 * \brief Wrapper around libarchive for writing.
 *
 */
class cmArchiveWrite
{
  typedef void (cmArchiveWrite::*safe_bool)();
  void safe_bool_true() {}
public:
  /** Compression type.  */
  enum Compress
  {
    CompressNone,
    CompressCompress,
    CompressGZip,
    CompressBZip2,
    CompressLZMA,
    CompressXZ
  };

  /** Construct with output stream to which to write archive.  */
  cmArchiveWrite(std::ostream& os, Compress c = CompressNone,
                 std::string const& format = "paxr");

  ~cmArchiveWrite();

  /**
   * Add a path (file or directory) to the archive.  Directories are
   * added recursively.  The "path" must be readable on disk, either
   * full path or relative to current working directory.  The "skip"
   * value indicates how many leading bytes from the input path to
   * skip.  The remaining part of the input path is appended to the
   * "prefix" value to construct the final name in the archive.
   */
  bool Add(std::string path, size_t skip = 0, const char* prefix = CM_NULLPTR,
           bool recursive = true);

  /** Returns true if there has been no error.  */
  operator safe_bool() const
  {
    return this->Okay() ? &cmArchiveWrite::safe_bool_true : CM_NULLPTR;
  }

  /** Returns true if there has been an error.  */
  bool operator!() const { return !this->Okay(); }

  /** Return the error string; empty if none.  */
  std::string GetError() const { return this->Error; }

  // TODO: More general callback instead of hard-coding calls to
  // std::cout.
  void SetVerbose(bool v) { this->Verbose = v; }

  void SetMTime(std::string const& t) { this->MTime = t; }

  //! Sets the permissions of the added files/folders
  void SetPermissions(int permissions_)
  {
    this->Permissions.Set(permissions_);
  }

  //! Clears permissions - default is used instead
  void ClearPermissions() { this->Permissions.Clear(); }

  //! Sets the permissions mask of files/folders
  //!
  //! The permissions will be copied from the existing file
  //! or folder. The mask will then be applied to unset
  //! some of them
  void SetPermissionsMask(int permissionsMask_)
  {
    this->PermissionsMask.Set(permissionsMask_);
  }

  //! Clears permissions mask - default is used instead
  void ClearPermissionsMask() { this->PermissionsMask.Clear(); }

  //! Sets UID and GID to be used in the tar file
  void SetUIDAndGID(int uid_, int gid_)
  {
    this->Uid.Set(uid_);
    this->Gid.Set(gid_);
  }

  //! Clears UID and GID to be used in the tar file - default is used instead
  void ClearUIDAndGID()
  {
    this->Uid.Clear();
    this->Gid.Clear();
  }

  //! Sets UNAME and GNAME to be used in the tar file
  void SetUNAMEAndGNAME(const std::string& uname_, const std::string& gname_)
  {
    this->Uname = uname_;
    this->Gname = gname_;
  }

  //! Clears UNAME and GNAME to be used in the tar file
  //! default is used instead
  void ClearUNAMEAndGNAME()
  {
    this->Uname = "";
    this->Gname = "";
  }

private:
  bool Okay() const { return this->Error.empty(); }
  bool AddPath(const char* path, size_t skip, const char* prefix,
               bool recursive = true);
  bool AddFile(const char* file, size_t skip, const char* prefix);
  bool AddData(const char* file, size_t size);

  struct Callback;
  friend struct Callback;

  class Entry;

  std::ostream& Stream;
  struct archive* Archive;
  struct archive* Disk;
  bool Verbose;
  std::string Format;
  std::string Error;
  std::string MTime;

  //! UID of the user in the tar file
  cmArchiveWriteOptional<int> Uid;

  //! GUID of the user in the tar file
  cmArchiveWriteOptional<int> Gid;

  //! UNAME/GNAME of the user (does not override UID/GID)
  //!@{
  std::string Uname;
  std::string Gname;
  //!@}

  //! Permissions on files/folders
  cmArchiveWriteOptional<int> Permissions;
  cmArchiveWriteOptional<int> PermissionsMask;
};

#endif
