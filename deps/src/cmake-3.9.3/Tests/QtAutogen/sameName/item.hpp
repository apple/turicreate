#ifndef ITEM_HPP
#define ITEM_HPP

#include <QObject>

class Item : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};

#endif
