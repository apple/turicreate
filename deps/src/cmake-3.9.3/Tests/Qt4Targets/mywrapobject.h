
#ifndef MYWRAPOBJECT_H
#define MYWRAPOBJECT_H

#include <QObject>

#include "myinterface.h"

class MyWrapObject : public QObject, MyInterface
{
  Q_OBJECT
  Q_INTERFACES(MyInterface)
public:
  explicit MyWrapObject(QObject* parent = 0)
    : QObject(parent)
  {
  }
};

#endif
