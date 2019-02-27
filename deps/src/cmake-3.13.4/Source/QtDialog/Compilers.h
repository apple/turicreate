

#ifndef COMPILERS_HPP
#define COMPILERS_HPP

#include "cmConfigure.h" // IWYU pragma: keep

#include <QWidget>

#include <ui_Compilers.h>

class Compilers
  : public QWidget
  , public Ui::Compilers
{
  Q_OBJECT
public:
  Compilers(QWidget* p = nullptr)
    : QWidget(p)
  {
    this->setupUi(this);
  }
};

#endif
