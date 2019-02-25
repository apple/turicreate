/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWIXRichTextFormatWriter_h
#define cmWIXRichTextFormatWriter_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmsys/FStream.hxx"
#include <string>

/** \class cmWIXRichtTextFormatWriter
 * \brief Helper class to generate Rich Text Format (RTF) documents
 * from plain text (e.g. for license and welcome text)
 */
class cmWIXRichTextFormatWriter
{
public:
  cmWIXRichTextFormatWriter(std::string const& filename);
  ~cmWIXRichTextFormatWriter();

  void AddText(std::string const& text);

private:
  void WriteHeader();
  void WriteFontTable();
  void WriteColorTable();
  void WriteGenerator();

  void WriteDocumentPrefix();

  void ControlWord(std::string const& keyword);
  void NewControlWord(std::string const& keyword);

  void StartGroup();
  void EndGroup();

  void EmitUnicodeCodepoint(int c);
  void EmitUnicodeSurrogate(int c);

  void EmitInvalidCodepoint(int c);

  cmsys::ofstream File;
};

#endif
