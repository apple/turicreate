/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCursesFilePathWidget.h"

#include "cmCursesPathWidget.h"
#include "cmStateTypes.h"

cmCursesFilePathWidget::cmCursesFilePathWidget(int width, int height, int left,
                                               int top)
  : cmCursesPathWidget(width, height, left, top)
{
  this->Type = cmStateEnums::FILEPATH;
}
