
#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>
#include <memory>

#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
#include <klocalizedstring.h>
#endif

#include "ui_mywidget.h"

class MyWidget : public QWidget
{
  Q_OBJECT
public:
  explicit MyWidget(QWidget* parent = 0);
  ~MyWidget();

private:
  Ui::MyWidget* ui;
};

#endif
