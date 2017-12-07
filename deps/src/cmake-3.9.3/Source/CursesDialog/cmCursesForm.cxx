/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCursesForm.h"

cmsys::ofstream cmCursesForm::DebugFile;
bool cmCursesForm::Debug = false;

cmCursesForm::cmCursesForm()
{
  this->Form = CM_NULLPTR;
}

cmCursesForm::~cmCursesForm()
{
  if (this->Form) {
    unpost_form(this->Form);
    free_form(this->Form);
    this->Form = CM_NULLPTR;
  }
}

void cmCursesForm::DebugStart()
{
  cmCursesForm::Debug = true;
  cmCursesForm::DebugFile.open("ccmakelog.txt");
}

void cmCursesForm::DebugEnd()
{
  if (!cmCursesForm::Debug) {
    return;
  }

  cmCursesForm::Debug = false;
  cmCursesForm::DebugFile.close();
}

void cmCursesForm::LogMessage(const char* msg)
{
  if (!cmCursesForm::Debug) {
    return;
  }

  cmCursesForm::DebugFile << msg << std::endl;
}
