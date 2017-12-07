
#ifndef GADGET_H
#define GADGET_H

#include <QObject>

class Gadget
{
  Q_GADGET
  Q_ENUMS(Type)
public:
  enum Type
  {
    Type0,
    Type1
  };
};

#endif
