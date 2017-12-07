/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCursesOptionsWidget.h"

#include "cmCursesWidget.h"
#include "cmStateTypes.h"

#define ctrl(z) ((z)&037)

cmCursesOptionsWidget::cmCursesOptionsWidget(int width, int height, int left,
                                             int top)
  : cmCursesWidget(width, height, left, top)
{
  this->Type = cmStateEnums::BOOL; // this is a bit of a hack
  // there is no option type, and string type causes ccmake to cast
  // the widget into a string widget at some point.  BOOL is safe for
  // now.
  set_field_fore(this->Field, A_NORMAL);
  set_field_back(this->Field, A_STANDOUT);
  field_opts_off(this->Field, O_STATIC);
}

bool cmCursesOptionsWidget::HandleInput(int& key, cmCursesMainForm* /*fm*/,
                                        WINDOW* w)
{
  switch (key) {
    case 10: // 10 == enter
    case KEY_ENTER:
      this->NextOption();
      touchwin(w);
      wrefresh(w);
      return true;
    case KEY_LEFT:
    case ctrl('b'):
      touchwin(w);
      wrefresh(w);
      this->PreviousOption();
      return true;
    case KEY_RIGHT:
    case ctrl('f'):
      this->NextOption();
      touchwin(w);
      wrefresh(w);
      return true;
    default:
      return false;
  }
}

void cmCursesOptionsWidget::AddOption(std::string const& option)
{
  this->Options.push_back(option);
}

void cmCursesOptionsWidget::NextOption()
{
  this->CurrentOption++;
  if (this->CurrentOption > this->Options.size() - 1) {
    this->CurrentOption = 0;
  }
  this->SetValue(this->Options[this->CurrentOption]);
}
void cmCursesOptionsWidget::PreviousOption()
{
  if (this->CurrentOption == 0) {
    this->CurrentOption = this->Options.size() - 1;
  } else {
    this->CurrentOption--;
  }
  this->SetValue(this->Options[this->CurrentOption]);
}

void cmCursesOptionsWidget::SetOption(const std::string& value)
{
  this->CurrentOption = 0; // default to 0 index
  this->SetValue(value);
  int index = 0;
  for (std::vector<std::string>::iterator i = this->Options.begin();
       i != this->Options.end(); ++i) {
    if (*i == value) {
      this->CurrentOption = index;
    }
    index++;
  }
}
