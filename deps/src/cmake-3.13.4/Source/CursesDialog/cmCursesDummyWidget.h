/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesDummyWidget_h
#define cmCursesDummyWidget_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCursesStandardIncludes.h"
#include "cmCursesWidget.h"

class cmCursesMainForm;

class cmCursesDummyWidget : public cmCursesWidget
{
  CM_DISABLE_COPY(cmCursesDummyWidget)

public:
  cmCursesDummyWidget(int width, int height, int left, int top);

  // Description:
  // Handle user input. Called by the container of this widget
  // when this widget has focus. Returns true if the input was
  // handled.
  bool HandleInput(int& key, cmCursesMainForm* fm, WINDOW* w) override;
};

#endif // cmCursesDummyWidget_h
