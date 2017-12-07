/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesOptionsWidget_h
#define cmCursesOptionsWidget_h

#include "cmConfigure.h"

#include "cmCursesStandardIncludes.h"
#include "cmCursesWidget.h"

#include <string>
#include <vector>

class cmCursesMainForm;

class cmCursesOptionsWidget : public cmCursesWidget
{
  CM_DISABLE_COPY(cmCursesOptionsWidget)

public:
  cmCursesOptionsWidget(int width, int height, int left, int top);

  // Description:
  // Handle user input. Called by the container of this widget
  // when this widget has focus. Returns true if the input was
  // handled.
  bool HandleInput(int& key, cmCursesMainForm* fm, WINDOW* w) CM_OVERRIDE;
  void SetOption(const std::string&);
  void AddOption(std::string const&);
  void NextOption();
  void PreviousOption();

protected:
  std::vector<std::string> Options;
  std::vector<std::string>::size_type CurrentOption;
};

#endif // cmCursesOptionsWidget_h
