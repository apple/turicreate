#ifndef SUBOBJA_HPP
#define SUBOBJA_HPP

#include <QObject>

namespace subA {

class ObjA : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}

#endif
