#include "ui_uiC.h"
#include "ui_uiD.h"
// AUTOUIC includes on the first two lines of a source file
#include "UicOnly.hpp"

UicOnly::UicOnly()
  : uiA(new Ui::UiA)
  , uiB(new Ui::UiB)
{
  Ui::UiC uiC;
  Ui::UiD uiD;
}

UicOnly::~UicOnly()
{
  delete uiB;
  delete uiA;
}
