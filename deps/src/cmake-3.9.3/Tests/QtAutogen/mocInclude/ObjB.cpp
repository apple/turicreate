#include "ObjB.hpp"

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

#include "ObjB.moc"
#include "moc_ObjB.cpp"
