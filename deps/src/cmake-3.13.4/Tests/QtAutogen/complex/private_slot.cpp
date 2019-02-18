
#include "private_slot.h"

class PrivateSlotPrivate
{
public:
  void privateSlot() {}
};

PrivateSlot::PrivateSlot(QObject* parent)
  : QObject(parent)
  , d(new PrivateSlotPrivate)
{
}

#include "private_slot.moc"
