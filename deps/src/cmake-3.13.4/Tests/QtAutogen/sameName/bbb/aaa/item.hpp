#ifndef BBB_AAA_ITEM_HPP
#define BBB_AAA_ITEM_HPP

#include <QObject>

namespace bbb {
namespace aaa {

class Item : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}
}

#endif
