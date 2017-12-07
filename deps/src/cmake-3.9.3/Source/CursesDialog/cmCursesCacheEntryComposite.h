/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesCacheEntryComposite_h
#define cmCursesCacheEntryComposite_h

#include "cmConfigure.h"

#include <string>

class cmCursesLabelWidget;
class cmCursesWidget;
class cmake;

class cmCursesCacheEntryComposite
{
  CM_DISABLE_COPY(cmCursesCacheEntryComposite)

public:
  cmCursesCacheEntryComposite(const std::string& key, int labelwidth,
                              int entrywidth);
  cmCursesCacheEntryComposite(const std::string& key, cmake* cm, bool isNew,
                              int labelwidth, int entrywidth);
  ~cmCursesCacheEntryComposite();
  const char* GetValue();

  friend class cmCursesMainForm;

protected:
  cmCursesLabelWidget* Label;
  cmCursesLabelWidget* IsNewLabel;
  cmCursesWidget* Entry;
  std::string Key;
  int LabelWidth;
  int EntryWidth;
};

#endif // cmCursesCacheEntryComposite_h
