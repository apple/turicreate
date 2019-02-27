#ifndef AAA_ITEM_HPP
#define AAA_ITEM_HPP

#include <QObject>
// Include ui_view.h only in header
#include <aaa/ui_view.h>

namespace aaa {

class Item : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}

#endif
