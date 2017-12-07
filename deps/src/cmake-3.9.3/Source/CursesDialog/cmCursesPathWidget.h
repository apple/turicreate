/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesPathWidget_h
#define cmCursesPathWidget_h

#include "cmConfigure.h"

#include "cmCursesStandardIncludes.h"
#include "cmCursesStringWidget.h"

#include <string>

class cmCursesMainForm;

class cmCursesPathWidget : public cmCursesStringWidget
{
  CM_DISABLE_COPY(cmCursesPathWidget)

public:
  cmCursesPathWidget(int width, int height, int left, int top);

  /**
   * This method is called when different keys are pressed. The
   * subclass can have a special implementation handler for this.
   */
  void OnTab(cmCursesMainForm* fm, WINDOW* w) CM_OVERRIDE;
  void OnReturn(cmCursesMainForm* fm, WINDOW* w) CM_OVERRIDE;
  void OnType(int& key, cmCursesMainForm* fm, WINDOW* w) CM_OVERRIDE;

protected:
  std::string LastString;
  std::string LastGlob;
  bool Cycle;
  std::string::size_type CurrentIndex;
};

#endif // cmCursesPathWidget_h
