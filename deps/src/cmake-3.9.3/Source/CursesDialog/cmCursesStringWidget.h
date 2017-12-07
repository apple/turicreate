/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesStringWidget_h
#define cmCursesStringWidget_h

#include "cmConfigure.h"

#include "cmCursesStandardIncludes.h"
#include "cmCursesWidget.h"

#include <string>

class cmCursesMainForm;

/** \class cmCursesStringWidget
 * \brief A simple entry widget.
 *
 * cmCursesStringWdiget is a simple text entry widget.
 */

class cmCursesStringWidget : public cmCursesWidget
{
  CM_DISABLE_COPY(cmCursesStringWidget)

public:
  cmCursesStringWidget(int width, int height, int left, int top);

  /**
   * Handle user input. Called by the container of this widget
   * when this widget has focus. Returns true if the input was
   * handled.
   */
  bool HandleInput(int& key, cmCursesMainForm* fm, WINDOW* w) CM_OVERRIDE;

  /**
   * Set/Get the string.
   */
  void SetString(const std::string& value);
  const char* GetString();
  const char* GetValue() CM_OVERRIDE;

  /**
   * Set/Get InEdit flag. Can be used to tell the widget to leave
   * edit mode (in case of a resize for example).
   */
  void SetInEdit(bool inedit) { this->InEdit = inedit; }
  bool GetInEdit() { return this->InEdit; }

  /**
   * This method is called when different keys are pressed. The
   * subclass can have a special implementation handler for this.
   */
  virtual void OnTab(cmCursesMainForm* fm, WINDOW* w);
  virtual void OnReturn(cmCursesMainForm* fm, WINDOW* w);
  virtual void OnType(int& key, cmCursesMainForm* fm, WINDOW* w);

  /**
   * If there are any, print the widget specific commands
   * in the toolbar and return true. Otherwise, return false
   * and the parent widget will print.
   */
  bool PrintKeys() CM_OVERRIDE;

protected:
  // true if the widget is in edit mode
  bool InEdit;
  char* OriginalString;
  bool Done;
};

#endif // cmCursesStringWidget_h
