#include "SubObjB.hpp"

namespace subB {

class SubObjB : public QObject
{
  Q_OBJECT

public:
  SubObjB() {}
  ~SubObjB() {}

  Q_SLOT
  void aSlot();
};

void SubObjB::aSlot()
{
}

void ObjB::go()
{
  SubObjB subObj;
}
}

#include "SubObjB.moc"
