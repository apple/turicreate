
#ifndef GENERATED_H
#define GENERATED_H

#include <QObject>

#include "myinterface.h"
#include "myotherinterface.h"

class Generated : public QObject, MyInterface, MyOtherInterface
{
  Q_OBJECT
  Q_INTERFACES(MyInterface MyOtherInterface)
public:
  explicit Generated(QObject* parent = 0);
};

#endif
