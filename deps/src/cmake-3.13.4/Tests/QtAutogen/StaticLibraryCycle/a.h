#ifndef CLASSA_HPP
#define CLASSA_HPP

#include <QObject>

class A : public QObject
{
  Q_OBJECT
  static bool recursed;

public:
  A();
};

#endif
