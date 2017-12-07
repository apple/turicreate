/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGeneratedFileStream_h
#define cmGeneratedFileStream_h

#include "cmConfigure.h"

#include "cm_codecvt.hxx"
#include "cmsys/FStream.hxx"
#include <string>

// This is the first base class of cmGeneratedFileStream.  It will be
// created before and destroyed after the ofstream portion and can
// therefore be used to manage the temporary file.
class cmGeneratedFileStreamBase
{
protected:
  // This constructor does not prepare the temporary file.  The open
  // method must be used.
  cmGeneratedFileStreamBase();

  // This constructor prepares the temporary output file.
  cmGeneratedFileStreamBase(const char* name);

  // The destructor renames the temporary output file to the real name.
  ~cmGeneratedFileStreamBase();

  // Internal methods to handle the temporary file.  Open is always
  // called before the real stream is opened.  Close is always called
  // after the real stream is closed and Okay is set to whether the
  // real stream was still valid for writing when it was closed.
  void Open(const char* name);
  bool Close();

  // Internal file replacement implementation.
  int RenameFile(const char* oldname, const char* newname);

  // Internal file compression implementation.
  int CompressFile(const char* oldname, const char* newname);

  // The name of the final destination file for the output.
  std::string Name;

  // The name of the temporary file.
  std::string TempName;

  // Whether to do a copy-if-different.
  bool CopyIfDifferent;

  // Whether the real file stream was valid when it was closed.
  bool Okay;

  // Whether the destination file is compressed
  bool Compress;

  // Whether the destination file is compressed
  bool CompressExtraExtension;
};

/** \class cmGeneratedFileStream
 * \brief Output stream for generated files.
 *
 * File generation should be atomic so that if CMake is killed then a
 * generated file is either the original version or the complete new
 * version.  This stream is used to make sure file generation is
 * atomic.  Optionally the output file is only replaced if its
 * contents have changed to prevent the file modification time from
 * being updated.
 */
class cmGeneratedFileStream : private cmGeneratedFileStreamBase,
                              public cmsys::ofstream
{
public:
  typedef cmsys::ofstream Stream;
  typedef codecvt::Encoding Encoding;

  /**
   * This constructor prepares a default stream.  The open method must
   * be used before writing to the stream.
   */
  cmGeneratedFileStream(Encoding encoding = codecvt::None);

  /**
   * This constructor takes the name of the file to be generated.  It
   * automatically generates a name for the temporary file.  If the
   * file cannot be opened an error message is produced unless the
   * second argument is set to true.
   */
  cmGeneratedFileStream(const char* name, bool quiet = false,
                        Encoding encoding = codecvt::None);

  /**
   * The destructor checks the stream status to be sure the temporary
   * file was successfully written before allowing the original to be
   * replaced.
   */
  ~cmGeneratedFileStream() CM_OVERRIDE;

  /**
   * Open an output file by name.  This should be used only with a
   * non-open stream.  It automatically generates a name for the
   * temporary file.  If the file cannot be opened an error message is
   * produced unless the second argument is set to true.
   */
  cmGeneratedFileStream& Open(const char* name, bool quiet = false,
                              bool binaryFlag = false);

  /**
   * Close the output file.  This should be used only with an open
   * stream.  The temporary file is atomically renamed to the
   * destionation file if the stream is still valid when this method
   * is called.
   */
  bool Close();

  /**
   * Set whether copy-if-different is done.
   */
  void SetCopyIfDifferent(bool copy_if_different);

  /**
   * Set whether compression is done.
   */
  void SetCompression(bool compression);

  /**
   * Set whether compression has extra extension
   */
  void SetCompressionExtraExtension(bool ext);

  /**
   * Set name of the file that will hold the actual output. This method allows
   * the output file to be changed during the use of cmGeneratedFileStream.
   */
  void SetName(const std::string& fname);

private:
  cmGeneratedFileStream(cmGeneratedFileStream const&); // not implemented
};

#endif
