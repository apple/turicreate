
#ifndef UIC_ONLY_H
#define UIC_ONLY_H

#include <QWidget>

#include "ui_uiconly.h"

class UicOnly : public QWidget
{
  Q_OBJECT
public:
  explicit UicOnly(QWidget* parent = 0);
  ~UicOnly();

private:
  Ui::UicOnly* ui;
};

#endif
