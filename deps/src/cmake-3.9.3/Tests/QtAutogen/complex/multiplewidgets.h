
#ifndef MULTIPLEWIDGETS_H
#define MULTIPLEWIDGETS_H

#include <QWidget>

namespace Ui {
class Widget1;
}

class Widget1 : public QWidget
{
  Q_OBJECT
public:
  Widget1(QWidget* parent = 0);

private:
  Ui::Widget1* ui;
};

namespace Ui {
class Widget2;
}

class Widget2 : public QWidget
{
  Q_OBJECT
public:
  Widget2(QWidget* parent = 0);

private:
  Ui::Widget2* ui;
};

#endif
