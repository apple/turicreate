
#include "mywidget.h"

MyWidget::MyWidget(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::MyWidget)
{
  ui->setupUi(this);
}

MyWidget::~MyWidget()
{
  delete ui;
}
