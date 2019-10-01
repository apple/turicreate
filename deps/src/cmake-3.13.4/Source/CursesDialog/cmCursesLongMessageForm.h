/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesLongMessageForm_h
#define cmCursesLongMessageForm_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCursesForm.h"
#include "cmCursesStandardIncludes.h"

#include <string>
#include <vector>

class cmCursesLongMessageForm : public cmCursesForm
{
  CM_DISABLE_COPY(cmCursesLongMessageForm)

public:
  cmCursesLongMessageForm(std::vector<std::string> const& messages,
                          const char* title);
  ~cmCursesLongMessageForm() override;

  // Description:
  // Handle user input.
  void HandleInput() override;

  // Description:
  // Display form. Use a window of size width x height, starting
  // at top, left.
  void Render(int left, int top, int width, int height) override;

  // Description:
  // This method should normally  called only by the form.
  // The only exception is during a resize.
  void PrintKeys();

  // Description:
  // This method should normally  called only by the form.
  // The only exception is during a resize.
  void UpdateStatusBar() override;

protected:
  std::string Messages;
  std::string Title;

  FIELD* Fields[2];
};

#endif // cmCursesLongMessageForm_h
