#ifndef ITEM_HPP
#define ITEM_HPP

#include <QObject>

class Item : public QObject
{
  Q_OBJECT

public:
  Q_SLOT
  void go();
};

#endif
