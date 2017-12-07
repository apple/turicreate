
#include "debug_class.h"
#include "ui_debug_class.h"

DebugClass::DebugClass(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::DebugClass)
{
  ui->setupUi(this);
}
