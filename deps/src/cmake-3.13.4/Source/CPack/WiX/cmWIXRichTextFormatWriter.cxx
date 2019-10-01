/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXRichTextFormatWriter.h"

#include "cmVersion.h"

cmWIXRichTextFormatWriter::cmWIXRichTextFormatWriter(
  std::string const& filename)
  : File(filename.c_str(), std::ios::binary)
{
  StartGroup();
  WriteHeader();
  WriteDocumentPrefix();
}

cmWIXRichTextFormatWriter::~cmWIXRichTextFormatWriter()
{
  EndGroup();

  /* I haven't seen this in the RTF spec but
   *  wordpad terminates its RTF like this */
  File << "\r\n";
  File.put(0);
}

void cmWIXRichTextFormatWriter::AddText(std::string const& text)
{
  typedef unsigned char rtf_byte_t;

  for (size_t i = 0; i < text.size(); ++i) {
    rtf_byte_t c = rtf_byte_t(text[i]);

    switch (c) {
      case '\\':
        File << "\\\\";
        break;
      case '{':
        File << "\\{";
        break;
      case '}':
        File << "\\}";
        break;
      case '\n':
        File << "\\par\r\n";
        break;
      case '\r':
        continue;
      default: {
        if (c <= 0x7F) {
          File << c;
        } else {
          if (c <= 0xC0) {
            EmitInvalidCodepoint(c);
          } else if (c < 0xE0 && i + 1 < text.size()) {
            EmitUnicodeCodepoint((text[i + 1] & 0x3F) | ((c & 0x1F) << 6));
            i += 1;
          } else if (c < 0xF0 && i + 2 < text.size()) {
            EmitUnicodeCodepoint((text[i + 2] & 0x3F) |
                                 ((text[i + 1] & 0x3F) << 6) |
                                 ((c & 0xF) << 12));
            i += 2;
          } else if (c < 0xF8 && i + 3 < text.size()) {
            EmitUnicodeCodepoint(
              (text[i + 3] & 0x3F) | ((text[i + 2] & 0x3F) << 6) |
              ((text[i + 1] & 0x3F) << 12) | ((c & 0x7) << 18));
            i += 3;
          } else {
            EmitInvalidCodepoint(c);
          }
        }
      } break;
    }
  }
}

void cmWIXRichTextFormatWriter::WriteHeader()
{
  ControlWord("rtf1");
  ControlWord("ansi");
  ControlWord("ansicpg1252");
  ControlWord("deff0");
  ControlWord("deflang1031");

  WriteFontTable();
  WriteColorTable();
  WriteGenerator();
}

void cmWIXRichTextFormatWriter::WriteFontTable()
{
  StartGroup();
  ControlWord("fonttbl");

  StartGroup();
  ControlWord("f0");
  ControlWord("fswiss");
  ControlWord("fcharset0 Arial;");
  EndGroup();

  EndGroup();
}

void cmWIXRichTextFormatWriter::WriteColorTable()
{
  StartGroup();
  ControlWord("colortbl ;");
  ControlWord("red255");
  ControlWord("green0");
  ControlWord("blue0;");
  ControlWord("red0");
  ControlWord("green255");
  ControlWord("blue0;");
  ControlWord("red0");
  ControlWord("green0");
  ControlWord("blue255;");
  EndGroup();
}

void cmWIXRichTextFormatWriter::WriteGenerator()
{
  StartGroup();
  NewControlWord("generator");
  File << " CPack WiX Generator (" << cmVersion::GetCMakeVersion() << ");";
  EndGroup();
}

void cmWIXRichTextFormatWriter::WriteDocumentPrefix()
{
  ControlWord("viewkind4");
  ControlWord("uc1");
  ControlWord("pard");
  ControlWord("f0");
  ControlWord("fs20");
}

void cmWIXRichTextFormatWriter::ControlWord(std::string const& keyword)
{
  File << "\\" << keyword;
}

void cmWIXRichTextFormatWriter::NewControlWord(std::string const& keyword)
{
  File << "\\*\\" << keyword;
}

void cmWIXRichTextFormatWriter::StartGroup()
{
  File.put('{');
}

void cmWIXRichTextFormatWriter::EndGroup()
{
  File.put('}');
}

void cmWIXRichTextFormatWriter::EmitUnicodeCodepoint(int c)
{
  // Do not emit byte order mark (BOM)
  if (c == 0xFEFF) {
    return;
  } else if (c <= 0xFFFF) {
    EmitUnicodeSurrogate(c);
  } else {
    c -= 0x10000;
    EmitUnicodeSurrogate(((c >> 10) & 0x3FF) + 0xD800);
    EmitUnicodeSurrogate((c & 0x3FF) + 0xDC00);
  }
}

void cmWIXRichTextFormatWriter::EmitUnicodeSurrogate(int c)
{
  ControlWord("u");
  if (c <= 32767) {
    File << c;
  } else {
    File << (c - 65536);
  }
  File << "?";
}

void cmWIXRichTextFormatWriter::EmitInvalidCodepoint(int c)
{
  ControlWord("cf1 ");
  File << "[INVALID-BYTE-" << int(c) << "]";
  ControlWord("cf0 ");
}
