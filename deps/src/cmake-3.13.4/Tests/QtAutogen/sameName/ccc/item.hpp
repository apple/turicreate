#ifndef CCC_ITEM_HPP
#define CCC_ITEM_HPP

#include <QObject>
// Include ui_view.h in source and header
#include <ccc/ui_view.h>

namespace ccc {

class Item : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}

#endif
