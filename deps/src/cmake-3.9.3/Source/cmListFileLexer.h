/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmListFileLexer_h
#define cmListFileLexer_h

typedef enum cmListFileLexer_Type_e {
  cmListFileLexer_Token_None,
  cmListFileLexer_Token_Space,
  cmListFileLexer_Token_Newline,
  cmListFileLexer_Token_Identifier,
  cmListFileLexer_Token_ParenLeft,
  cmListFileLexer_Token_ParenRight,
  cmListFileLexer_Token_ArgumentUnquoted,
  cmListFileLexer_Token_ArgumentQuoted,
  cmListFileLexer_Token_ArgumentBracket,
  cmListFileLexer_Token_CommentBracket,
  cmListFileLexer_Token_BadCharacter,
  cmListFileLexer_Token_BadBracket,
  cmListFileLexer_Token_BadString
} cmListFileLexer_Type;

typedef struct cmListFileLexer_Token_s cmListFileLexer_Token;
struct cmListFileLexer_Token_s
{
  cmListFileLexer_Type type;
  char* text;
  int length;
  int line;
  int column;
};

enum cmListFileLexer_BOM_e
{
  cmListFileLexer_BOM_None,
  cmListFileLexer_BOM_Broken,
  cmListFileLexer_BOM_UTF8,
  cmListFileLexer_BOM_UTF16BE,
  cmListFileLexer_BOM_UTF16LE,
  cmListFileLexer_BOM_UTF32BE,
  cmListFileLexer_BOM_UTF32LE
};
typedef enum cmListFileLexer_BOM_e cmListFileLexer_BOM;

typedef struct cmListFileLexer_s cmListFileLexer;

#ifdef __cplusplus
extern "C" {
#endif

cmListFileLexer* cmListFileLexer_New(void);
int cmListFileLexer_SetFileName(cmListFileLexer*, const char*,
                                cmListFileLexer_BOM* bom);
int cmListFileLexer_SetString(cmListFileLexer*, const char*);
cmListFileLexer_Token* cmListFileLexer_Scan(cmListFileLexer*);
long cmListFileLexer_GetCurrentLine(cmListFileLexer*);
long cmListFileLexer_GetCurrentColumn(cmListFileLexer*);
const char* cmListFileLexer_GetTypeAsString(cmListFileLexer*,
                                            cmListFileLexer_Type);
void cmListFileLexer_Delete(cmListFileLexer*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
