#ifndef BBB_ITEM_HPP
#define BBB_ITEM_HPP

#include <QObject>
// Include ui_view.h only in source

namespace bbb {

class Item : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}

#endif
