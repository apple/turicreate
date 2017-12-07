/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesForm_h
#define cmCursesForm_h

#include "cmConfigure.h"

#include "cmCursesStandardIncludes.h"

#include "cmsys/FStream.hxx"

class cmCursesForm
{
  CM_DISABLE_COPY(cmCursesForm)

public:
  cmCursesForm();
  virtual ~cmCursesForm();

  // Description:
  // Handle user input.
  virtual void HandleInput() = 0;

  // Description:
  // Display form.
  virtual void Render(int left, int top, int width, int height) = 0;

  // Description:
  // This method should normally  called only by the form.
  // The only exception is during a resize.
  virtual void UpdateStatusBar() = 0;

  // Description:
  // During a CMake run, an error handle should add errors
  // to be displayed afterwards.
  virtual void AddError(const char*, const char*) {}

  // Description:
  // Turn debugging on. This will create ccmakelog.txt.
  static void DebugStart();

  // Description:
  // Turn debugging off. This will close ccmakelog.txt.
  static void DebugEnd();

  // Description:
  // Write a debugging message.
  static void LogMessage(const char* msg);

  // Description:
  // Return the FORM. Should be only used by low-level methods.
  FORM* GetForm() { return this->Form; }

  static cmCursesForm* CurrentForm;

protected:
  static cmsys::ofstream DebugFile;
  static bool Debug;

  FORM* Form;
};

#endif // cmCursesForm_h
