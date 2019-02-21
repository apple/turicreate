/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCursesLabelWidget.h"

#include "cmCursesWidget.h"

cmCursesLabelWidget::cmCursesLabelWidget(int width, int height, int left,
                                         int top, const std::string& name)
  : cmCursesWidget(width, height, left, top)
{
  field_opts_off(this->Field, O_EDIT);
  field_opts_off(this->Field, O_ACTIVE);
  field_opts_off(this->Field, O_STATIC);
  this->SetValue(name);
}

cmCursesLabelWidget::~cmCursesLabelWidget()
{
}

bool cmCursesLabelWidget::HandleInput(int& /*key*/, cmCursesMainForm* /*fm*/,
                                      WINDOW* /*w*/)
{
  // Static text. No input is handled here.
  return false;
}
