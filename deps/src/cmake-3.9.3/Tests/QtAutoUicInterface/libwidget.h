
#ifndef LIBWIDGET_H
#define LIBWIDGET_H

#include <QWidget>
#include <memory>

#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
#include <klocalizedstring.h>
#endif

#include "ui_libwidget.h"

class LibWidget : public QWidget
{
  Q_OBJECT
public:
  explicit LibWidget(QWidget* parent = 0);
  ~LibWidget();

private:
  Ui::LibWidget* ui;
};

#endif
