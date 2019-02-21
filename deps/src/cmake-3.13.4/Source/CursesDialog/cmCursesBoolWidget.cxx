/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCursesBoolWidget.h"

#include "cmCursesWidget.h"
#include "cmStateTypes.h"

#include <string>

cmCursesBoolWidget::cmCursesBoolWidget(int width, int height, int left,
                                       int top)
  : cmCursesWidget(width, height, left, top)
{
  this->Type = cmStateEnums::BOOL;
  set_field_fore(this->Field, A_NORMAL);
  set_field_back(this->Field, A_STANDOUT);
  field_opts_off(this->Field, O_STATIC);
  this->SetValueAsBool(false);
}

bool cmCursesBoolWidget::HandleInput(int& key, cmCursesMainForm* /*fm*/,
                                     WINDOW* w)
{

  // toggle boolean values with enter or space
  // 10 == enter
  if (key == 10 || key == KEY_ENTER || key == ' ') {
    if (this->GetValueAsBool()) {
      this->SetValueAsBool(false);
    } else {
      this->SetValueAsBool(true);
    }

    touchwin(w);
    wrefresh(w);
    return true;
  }
  return false;
}

void cmCursesBoolWidget::SetValueAsBool(bool value)
{
  if (value) {
    this->SetValue("ON");
  } else {
    this->SetValue("OFF");
  }
}

bool cmCursesBoolWidget::GetValueAsBool()
{
  return this->Value == "ON";
}
