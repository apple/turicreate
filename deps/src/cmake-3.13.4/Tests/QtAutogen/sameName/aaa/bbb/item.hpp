#ifndef AAA_BBB_ITEM_HPP
#define AAA_BBB_ITEM_HPP

#include <QObject>

namespace aaa {
namespace bbb {

class Item : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}
}

#endif
