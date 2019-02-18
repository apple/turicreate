#include "ui_uiA.h"
#include "ui_uiB.h"
// AUTOUIC includes on the first two lines of a header file
#include <QObject>

class UicOnly : public QObject
{
public:
  UicOnly();
  ~UicOnly();

private:
  Ui::UiA* uiA;
  Ui::UiB* uiB;
};
