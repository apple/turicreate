/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesWidget_h
#define cmCursesWidget_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCursesStandardIncludes.h"
#include "cmStateTypes.h"

#include <string>

class cmCursesMainForm;

class cmCursesWidget
{
  CM_DISABLE_COPY(cmCursesWidget)

public:
  cmCursesWidget(int width, int height, int left, int top);
  virtual ~cmCursesWidget();

  /**
   * Handle user input. Called by the container of this widget
   * when this widget has focus. Returns true if the input was
   * handled
   */
  virtual bool HandleInput(int& key, cmCursesMainForm* fm, WINDOW* w) = 0;

  /**
   * Change the position of the widget. Set isNewPage to true
   * if this widget marks the beginning of a new page.
   */
  virtual void Move(int x, int y, bool isNewPage);

  /**
   * Set/Get the value (setting the value also changes the contents
   * of the field buffer).
   */
  virtual void SetValue(const std::string& value);
  virtual const char* GetValue();

  /**
   * Get the type of the widget (STRING, PATH etc...)
   */
  cmStateEnums::CacheEntryType GetType() { return this->Type; }

  /**
   * If there are any, print the widget specific commands
   * in the toolbar and return true. Otherwise, return false
   * and the parent widget will print.
   */
  virtual bool PrintKeys() { return false; }

  /**
   * Set/Get the page this widget is in.
   */
  void SetPage(int page) { this->Page = page; }
  int GetPage() { return this->Page; }

  friend class cmCursesMainForm;

protected:
  cmStateEnums::CacheEntryType Type;
  std::string Value;
  FIELD* Field;
  // The page in the main form this widget is in
  int Page;
};

#endif // cmCursesWidget_h
