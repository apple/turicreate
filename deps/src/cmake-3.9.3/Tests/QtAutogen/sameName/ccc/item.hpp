#ifndef CCC_ITEM_HPP
#define CCC_ITEM_HPP

#include <QObject>

namespace ccc {

class Item : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}

#endif
