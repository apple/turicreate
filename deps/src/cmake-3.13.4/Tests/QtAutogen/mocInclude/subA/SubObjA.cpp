#include "SubObjA.hpp"

namespace subA {

class SubObjA : public QObject
{
  Q_OBJECT

public:
  SubObjA() {}
  ~SubObjA() {}

  Q_SLOT
  void aSlot();
};

void SubObjA::aSlot()
{
}

void ObjA::go()
{
  SubObjA subObj;
}
}

#include "SubObjA.moc"
