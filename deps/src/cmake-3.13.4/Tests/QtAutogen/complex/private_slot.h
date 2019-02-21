
#ifndef PRIVATE_SLOT_H
#define PRIVATE_SLOT_H

#include <QObject>

class PrivateSlotPrivate;

class PrivateSlot : public QObject
{
  Q_OBJECT
public:
  PrivateSlot(QObject* parent = 0);

private:
  PrivateSlotPrivate* const d;
  Q_PRIVATE_SLOT(d, void privateSlot())
};

#endif
