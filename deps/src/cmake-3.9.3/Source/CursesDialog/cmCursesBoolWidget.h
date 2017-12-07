/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesBoolWidget_h
#define cmCursesBoolWidget_h

#include "cmConfigure.h"

#include "cmCursesStandardIncludes.h"
#include "cmCursesWidget.h"

class cmCursesMainForm;

class cmCursesBoolWidget : public cmCursesWidget
{
  CM_DISABLE_COPY(cmCursesBoolWidget)

public:
  cmCursesBoolWidget(int width, int height, int left, int top);

  // Description:
  // Handle user input. Called by the container of this widget
  // when this widget has focus. Returns true if the input was
  // handled.
  bool HandleInput(int& key, cmCursesMainForm* fm, WINDOW* w) CM_OVERRIDE;

  // Description:
  // Set/Get the value (on/off).
  void SetValueAsBool(bool value);
  bool GetValueAsBool();
};

#endif // cmCursesBoolWidget_h
