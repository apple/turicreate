
#include "uiconly.h"

UicOnly::UicOnly(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::UicOnly)
{
}

UicOnly::~UicOnly()
{
  delete ui;
}

int main()
{
  return 0;
}
