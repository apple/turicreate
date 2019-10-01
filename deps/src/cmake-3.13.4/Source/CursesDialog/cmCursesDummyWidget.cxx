/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCursesDummyWidget.h"

#include "cmCursesWidget.h"
#include "cmStateTypes.h"

cmCursesDummyWidget::cmCursesDummyWidget(int width, int height, int left,
                                         int top)
  : cmCursesWidget(width, height, left, top)
{
  this->Type = cmStateEnums::INTERNAL;
}

bool cmCursesDummyWidget::HandleInput(int& /*key*/, cmCursesMainForm* /*fm*/,
                                      WINDOW* /*w*/)
{
  return false;
}
