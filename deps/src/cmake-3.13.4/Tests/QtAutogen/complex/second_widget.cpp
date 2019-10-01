
#include "second_widget.h"
#include "ui_second_widget.h"

SecondWidget::SecondWidget(QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::SecondWidget)
{
  ui->setupUi(this);
}

SecondWidget::~SecondWidget()
{
  delete ui;
}
