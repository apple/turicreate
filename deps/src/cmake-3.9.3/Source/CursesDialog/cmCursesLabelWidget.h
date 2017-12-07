/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesLabelWidget_h
#define cmCursesLabelWidget_h

#include "cmConfigure.h"

#include "cmCursesStandardIncludes.h"
#include "cmCursesWidget.h"

#include <string>

class cmCursesMainForm;

class cmCursesLabelWidget : public cmCursesWidget
{
  CM_DISABLE_COPY(cmCursesLabelWidget)

public:
  cmCursesLabelWidget(int width, int height, int left, int top,
                      const std::string& name);
  ~cmCursesLabelWidget() CM_OVERRIDE;

  // Description:
  // Handle user input. Called by the container of this widget
  // when this widget has focus. Returns true if the input was
  // handled
  bool HandleInput(int& key, cmCursesMainForm* fm, WINDOW* w) CM_OVERRIDE;
};

#endif // cmCursesLabelWidget_h
