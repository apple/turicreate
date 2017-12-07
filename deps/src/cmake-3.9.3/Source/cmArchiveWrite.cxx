/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmArchiveWrite.h"

#include "cmLocale.h"
#include "cmSystemTools.h"
#include "cm_get_date.h"
#include "cm_libarchive.h"
#include "cmsys/Directory.hxx"
#include "cmsys/Encoding.hxx"
#include "cmsys/FStream.hxx"
#include <iostream>
#include <string.h>
#include <time.h>

#ifndef __LA_SSIZE_T
#define __LA_SSIZE_T la_ssize_t
#endif

static std::string cm_archive_error_string(struct archive* a)
{
  const char* e = archive_error_string(a);
  return e ? e : "unknown error";
}

static void cm_archive_entry_copy_pathname(struct archive_entry* e,
                                           const std::string& dest)
{
#if cmsys_STL_HAS_WSTRING
  archive_entry_copy_pathname_w(e, cmsys::Encoding::ToWide(dest).c_str());
#else
  archive_entry_copy_pathname(e, dest.c_str());
#endif
}

static void cm_archive_entry_copy_sourcepath(struct archive_entry* e,
                                             const std::string& file)
{
#if cmsys_STL_HAS_WSTRING
  archive_entry_copy_sourcepath_w(e, cmsys::Encoding::ToWide(file).c_str());
#else
  archive_entry_copy_sourcepath(e, file.c_str());
#endif
}

class cmArchiveWrite::Entry
{
  struct archive_entry* Object;

public:
  Entry()
    : Object(archive_entry_new())
  {
  }
  ~Entry() { archive_entry_free(this->Object); }
  operator struct archive_entry*() { return this->Object; }
};

struct cmArchiveWrite::Callback
{
  // archive_write_callback
  static __LA_SSIZE_T Write(struct archive* /*unused*/, void* cd,
                            const void* b, size_t n)
  {
    cmArchiveWrite* self = static_cast<cmArchiveWrite*>(cd);
    if (self->Stream.write(static_cast<const char*>(b),
                           static_cast<std::streamsize>(n))) {
      return static_cast<__LA_SSIZE_T>(n);
    }
    return static_cast<__LA_SSIZE_T>(-1);
  }
};

cmArchiveWrite::cmArchiveWrite(std::ostream& os, Compress c,
                               std::string const& format)
  : Stream(os)
  , Archive(archive_write_new())
  , Disk(archive_read_disk_new())
  , Verbose(false)
  , Format(format)
{
  switch (c) {
    case CompressNone:
      if (archive_write_add_filter_none(this->Archive) != ARCHIVE_OK) {
        this->Error = "archive_write_add_filter_none: ";
        this->Error += cm_archive_error_string(this->Archive);
        return;
      }
      break;
    case CompressCompress:
      if (archive_write_add_filter_compress(this->Archive) != ARCHIVE_OK) {
        this->Error = "archive_write_add_filter_compress: ";
        this->Error += cm_archive_error_string(this->Archive);
        return;
      }
      break;
    case CompressGZip:
      if (archive_write_add_filter_gzip(this->Archive) != ARCHIVE_OK) {
        this->Error = "archive_write_add_filter_gzip: ";
        this->Error += cm_archive_error_string(this->Archive);
        return;
      }
      break;
    case CompressBZip2:
      if (archive_write_add_filter_bzip2(this->Archive) != ARCHIVE_OK) {
        this->Error = "archive_write_add_filter_bzip2: ";
        this->Error += cm_archive_error_string(this->Archive);
        return;
      }
      break;
    case CompressLZMA:
      if (archive_write_add_filter_lzma(this->Archive) != ARCHIVE_OK) {
        this->Error = "archive_write_add_filter_lzma: ";
        this->Error += cm_archive_error_string(this->Archive);
        return;
      }
      break;
    case CompressXZ:
      if (archive_write_add_filter_xz(this->Archive) != ARCHIVE_OK) {
        this->Error = "archive_write_add_filter_xz: ";
        this->Error += cm_archive_error_string(this->Archive);
        return;
      }
      break;
  };
#if !defined(_WIN32) || defined(__CYGWIN__)
  if (archive_read_disk_set_standard_lookup(this->Disk) != ARCHIVE_OK) {
    this->Error = "archive_read_disk_set_standard_lookup: ";
    this->Error += cm_archive_error_string(this->Archive);
    return;
  }
#endif

  if (archive_write_set_format_by_name(this->Archive, format.c_str()) !=
      ARCHIVE_OK) {
    this->Error = "archive_write_set_format_by_name: ";
    this->Error += cm_archive_error_string(this->Archive);
    return;
  }

  // do not pad the last block!!
  if (archive_write_set_bytes_in_last_block(this->Archive, 1)) {
    this->Error = "archive_write_set_bytes_in_last_block: ";
    this->Error += cm_archive_error_string(this->Archive);
    return;
  }

  if (archive_write_open(
        this->Archive, this, CM_NULLPTR,
        reinterpret_cast<archive_write_callback*>(&Callback::Write),
        CM_NULLPTR) != ARCHIVE_OK) {
    this->Error = "archive_write_open: ";
    this->Error += cm_archive_error_string(this->Archive);
    return;
  }
}

cmArchiveWrite::~cmArchiveWrite()
{
  archive_read_free(this->Disk);
  archive_write_free(this->Archive);
}

bool cmArchiveWrite::Add(std::string path, size_t skip, const char* prefix,
                         bool recursive)
{
  if (this->Okay()) {
    if (!path.empty() && path[path.size() - 1] == '/') {
      path.erase(path.size() - 1);
    }
    this->AddPath(path.c_str(), skip, prefix, recursive);
  }
  return this->Okay();
}

bool cmArchiveWrite::AddPath(const char* path, size_t skip, const char* prefix,
                             bool recursive)
{
  if (!this->AddFile(path, skip, prefix)) {
    return false;
  }
  if ((!cmSystemTools::FileIsDirectory(path) || !recursive) ||
      cmSystemTools::FileIsSymlink(path)) {
    return true;
  }
  cmsys::Directory d;
  if (d.Load(path)) {
    std::string next = path;
    next += "/";
    std::string::size_type end = next.size();
    unsigned long n = d.GetNumberOfFiles();
    for (unsigned long i = 0; i < n; ++i) {
      const char* file = d.GetFile(i);
      if (strcmp(file, ".") != 0 && strcmp(file, "..") != 0) {
        next.erase(end);
        next += file;
        if (!this->AddPath(next.c_str(), skip, prefix)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool cmArchiveWrite::AddFile(const char* file, size_t skip, const char* prefix)
{
  // Skip the file if we have no name for it.  This may happen on a
  // top-level directory, which does not need to be included anyway.
  if (skip >= strlen(file)) {
    return true;
  }
  const char* out = file + skip;

  cmLocaleRAII localeRAII;
  static_cast<void>(localeRAII);

  // Meta-data.
  std::string dest = prefix ? prefix : "";
  dest += out;
  if (this->Verbose) {
    std::cout << dest << "\n";
  }
  Entry e;
  cm_archive_entry_copy_sourcepath(e, file);
  cm_archive_entry_copy_pathname(e, dest);
  if (archive_read_disk_entry_from_file(this->Disk, e, -1, CM_NULLPTR) !=
      ARCHIVE_OK) {
    this->Error = "archive_read_disk_entry_from_file '";
    this->Error += file;
    this->Error += "': ";
    this->Error += cm_archive_error_string(this->Disk);
    return false;
  }
  if (!this->MTime.empty()) {
    time_t now;
    time(&now);
    time_t t = cm_get_date(now, this->MTime.c_str());
    if (t == -1) {
      this->Error = "unable to parse mtime '";
      this->Error += this->MTime;
      this->Error += "'";
      return false;
    }
    archive_entry_set_mtime(e, t, 0);
  }

  // manages the uid/guid of the entry (if any)
  if (this->Uid.IsSet() && this->Gid.IsSet()) {
    archive_entry_set_uid(e, this->Uid.Get());
    archive_entry_set_gid(e, this->Gid.Get());
  }

  if (!this->Uname.empty() && !this->Gname.empty()) {
    archive_entry_set_uname(e, this->Uname.c_str());
    archive_entry_set_gname(e, this->Gname.c_str());
  }

  // manages the permissions
  if (this->Permissions.IsSet()) {
    archive_entry_set_perm(e, this->Permissions.Get());
  }

  if (this->PermissionsMask.IsSet()) {
    int perm = archive_entry_perm(e);
    archive_entry_set_perm(e, perm & this->PermissionsMask.Get());
  }

  // Clear acl and xattr fields not useful for distribution.
  archive_entry_acl_clear(e);
  archive_entry_xattr_clear(e);
  archive_entry_set_fflags(e, 0, 0);

  if (this->Format == "pax" || this->Format == "paxr") {
    // Sparse files are a GNU tar extension.
    // Do not use them in standard tar files.
    archive_entry_sparse_clear(e);
  }

  if (archive_write_header(this->Archive, e) != ARCHIVE_OK) {
    this->Error = "archive_write_header: ";
    this->Error += cm_archive_error_string(this->Archive);
    return false;
  }

  // do not copy content of symlink
  if (!archive_entry_symlink(e)) {
    // Content.
    if (size_t size = static_cast<size_t>(archive_entry_size(e))) {
      return this->AddData(file, size);
    }
  }
  return true;
}

bool cmArchiveWrite::AddData(const char* file, size_t size)
{
  cmsys::ifstream fin(file, std::ios::in | std::ios::binary);
  if (!fin) {
    this->Error = "Error opening \"";
    this->Error += file;
    this->Error += "\": ";
    this->Error += cmSystemTools::GetLastSystemError();
    return false;
  }

  char buffer[16384];
  size_t nleft = size;
  while (nleft > 0) {
    typedef std::streamsize ssize_type;
    size_t const nnext = nleft > sizeof(buffer) ? sizeof(buffer) : nleft;
    ssize_type const nnext_s = static_cast<ssize_type>(nnext);
    fin.read(buffer, nnext_s);
    // Some stream libraries (older HPUX) return failure at end of
    // file on the last read even if some data were read.  Check
    // gcount instead of trusting the stream error status.
    if (static_cast<size_t>(fin.gcount()) != nnext) {
      break;
    }
    if (archive_write_data(this->Archive, buffer, nnext) != nnext_s) {
      this->Error = "archive_write_data: ";
      this->Error += cm_archive_error_string(this->Archive);
      return false;
    }
    nleft -= nnext;
  }
  if (nleft > 0) {
    this->Error = "Error reading \"";
    this->Error += file;
    this->Error += "\": ";
    this->Error += cmSystemTools::GetLastSystemError();
    return false;
  }
  return true;
}
