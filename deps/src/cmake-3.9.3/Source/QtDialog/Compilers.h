

#ifndef COMPILERS_HPP
#define COMPILERS_HPP

#include "cmConfigure.h"

#include <QWidget>

#include <ui_Compilers.h>

class Compilers : public QWidget, public Ui::Compilers
{
  Q_OBJECT
public:
  Compilers(QWidget* p = CM_NULLPTR)
    : QWidget(p)
  {
    this->setupUi(this);
  }
};

#endif
