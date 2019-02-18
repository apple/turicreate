/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCursesStringWidget.h"

#include "cmCursesForm.h"
#include "cmCursesMainForm.h"
#include "cmCursesStandardIncludes.h"
#include "cmCursesWidget.h"
#include "cmStateTypes.h"

#include <stdio.h>
#include <string.h>

inline int ctrl(int z)
{
  return (z & 037);
}

cmCursesStringWidget::cmCursesStringWidget(int width, int height, int left,
                                           int top)
  : cmCursesWidget(width, height, left, top)
{
  this->InEdit = false;
  this->Type = cmStateEnums::STRING;
  set_field_fore(this->Field, A_NORMAL);
  set_field_back(this->Field, A_STANDOUT);
  field_opts_off(this->Field, O_STATIC);
}

void cmCursesStringWidget::OnTab(cmCursesMainForm* /*unused*/,
                                 WINDOW* /*unused*/)
{
  // FORM* form = fm->GetForm();
}

void cmCursesStringWidget::OnReturn(cmCursesMainForm* fm, WINDOW* /*unused*/)
{
  FORM* form = fm->GetForm();
  if (this->InEdit) {
    cmCursesForm::LogMessage("String widget leaving edit.");
    this->InEdit = false;
    fm->PrintKeys();
    delete[] this->OriginalString;
    // trick to force forms to update the field buffer
    form_driver(form, REQ_NEXT_FIELD);
    form_driver(form, REQ_PREV_FIELD);
    this->Done = true;
  } else {
    cmCursesForm::LogMessage("String widget entering edit.");
    this->InEdit = true;
    fm->PrintKeys();
    char* buf = field_buffer(this->Field, 0);
    this->OriginalString = new char[strlen(buf) + 1];
    strcpy(this->OriginalString, buf);
  }
}

void cmCursesStringWidget::OnType(int& key, cmCursesMainForm* fm,
                                  WINDOW* /*unused*/)
{
  form_driver(fm->GetForm(), key);
}

bool cmCursesStringWidget::HandleInput(int& key, cmCursesMainForm* fm,
                                       WINDOW* w)
{
  int x, y;

  FORM* form = fm->GetForm();
  // when not in edit mode, edit mode is entered by pressing enter or i (vim
  // binding)
  // 10 == enter
  if (!this->InEdit && (key != 10 && key != KEY_ENTER && key != 'i')) {
    return false;
  }

  this->OriginalString = nullptr;
  this->Done = false;

  char debugMessage[128];

  // <Enter> is used to change edit mode (like <Esc> in vi).
  while (!this->Done) {
    sprintf(debugMessage, "String widget handling input, key: %d", key);
    cmCursesForm::LogMessage(debugMessage);

    fm->PrintKeys();

    getmaxyx(stdscr, y, x);
    // If window too small, handle 'q' only
    if (x < cmCursesMainForm::MIN_WIDTH || y < cmCursesMainForm::MIN_HEIGHT) {
      // quit
      if (key == 'q') {
        return false;
      }
      key = getch();
      continue;
    }

    // If resize occurred during edit, move out of edit mode
    if (!this->InEdit && (key != 10 && key != KEY_ENTER && key != 'i')) {
      return false;
    }
    // enter edit with return and i (vim binding)
    if (!this->InEdit && (key == 10 || key == KEY_ENTER || key == 'i')) {
      this->OnReturn(fm, w);
    }
    // leave edit with return (but not i -- not a toggle)
    else if (this->InEdit && (key == 10 || key == KEY_ENTER)) {
      this->OnReturn(fm, w);
    } else if (key == KEY_DOWN || key == ctrl('n') || key == KEY_UP ||
               key == ctrl('p') || key == KEY_NPAGE || key == ctrl('d') ||
               key == KEY_PPAGE || key == ctrl('u')) {
      this->InEdit = false;
      delete[] this->OriginalString;
      // trick to force forms to update the field buffer
      form_driver(form, REQ_NEXT_FIELD);
      form_driver(form, REQ_PREV_FIELD);
      return false;
    }
    // esc
    else if (key == 27) {
      if (this->InEdit) {
        this->InEdit = false;
        fm->PrintKeys();
        this->SetString(this->OriginalString);
        delete[] this->OriginalString;
        touchwin(w);
        wrefresh(w);
        return true;
      }
    } else if (key == 9) {
      this->OnTab(fm, w);
    } else if (key == KEY_LEFT || key == ctrl('b')) {
      form_driver(form, REQ_PREV_CHAR);
    } else if (key == KEY_RIGHT || key == ctrl('f')) {
      form_driver(form, REQ_NEXT_CHAR);
    } else if (key == ctrl('k')) {
      form_driver(form, REQ_CLR_EOL);
    } else if (key == ctrl('a') || key == KEY_HOME) {
      form_driver(form, REQ_BEG_FIELD);
    } else if (key == ctrl('e') || key == KEY_END) {
      form_driver(form, REQ_END_FIELD);
    } else if (key == 127 || key == KEY_BACKSPACE) {
      FIELD* cur = current_field(form);
      form_driver(form, REQ_DEL_PREV);
      if (current_field(form) != cur) {
        set_current_field(form, cur);
      }
    } else if (key == ctrl('d') || key == KEY_DC) {
      form_driver(form, REQ_DEL_CHAR);
    } else {
      this->OnType(key, fm, w);
    }
    if (!this->Done) {
      touchwin(w);
      wrefresh(w);

      key = getch();
    }
  }
  return true;
}

void cmCursesStringWidget::SetString(const std::string& value)
{
  this->SetValue(value);
}

const char* cmCursesStringWidget::GetString()
{
  return this->GetValue();
}

const char* cmCursesStringWidget::GetValue()
{
  return field_buffer(this->Field, 0);
}

bool cmCursesStringWidget::PrintKeys()
{
  int x, y;
  getmaxyx(stdscr, y, x);
  if (x < cmCursesMainForm::MIN_WIDTH || y < cmCursesMainForm::MIN_HEIGHT) {
    return false;
  }
  if (this->InEdit) {
    char fmt_s[] = "%s";
    char firstLine[512];
    // Clean the toolbar
    memset(firstLine, ' ', sizeof(firstLine));
    firstLine[511] = '\0';
    curses_move(y - 4, 0);
    printw(fmt_s, firstLine);
    curses_move(y - 3, 0);
    printw(fmt_s, firstLine);
    curses_move(y - 2, 0);
    printw(fmt_s, firstLine);
    curses_move(y - 1, 0);
    printw(fmt_s, firstLine);

    curses_move(y - 3, 0);
    printw(fmt_s, "Editing option, press [enter] to confirm");
    curses_move(y - 2, 0);
    printw(fmt_s, "                press [esc] to cancel");
    return true;
  }
  return false;
}
