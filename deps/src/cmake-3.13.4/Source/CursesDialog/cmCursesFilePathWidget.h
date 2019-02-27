/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesFilePathWidget_h
#define cmCursesFilePathWidget_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCursesPathWidget.h"

class cmCursesFilePathWidget : public cmCursesPathWidget
{
  CM_DISABLE_COPY(cmCursesFilePathWidget)

public:
  cmCursesFilePathWidget(int width, int height, int left, int top);
};

#endif // cmCursesFilePathWidget_h
