#include "SubObjC.hpp"

namespace subC {

class SubObjC : public QObject
{
  Q_OBJECT

public:
  SubObjC() {}
  ~SubObjC() {}

  Q_SLOT
  void aSlot();
};

void SubObjC::aSlot()
{
}

void ObjC::go()
{
  SubObjC subObj;
}
}

#include "SubObjC.moc"
