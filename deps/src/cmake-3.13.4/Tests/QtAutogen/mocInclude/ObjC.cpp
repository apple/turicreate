#include "ObjC.hpp"

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

#include "ObjC.moc"
// Not the own header
#include "moc_ObjD.cpp"
