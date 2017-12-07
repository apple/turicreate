
#include "multiplewidgets.h"

#include "ui_widget1.h"
#include "ui_widget2.h"

Widget1::Widget1(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::Widget1)
{
  ui->setupUi(this);
}

Widget2::Widget2(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::Widget2)
{
  ui->setupUi(this);
}
