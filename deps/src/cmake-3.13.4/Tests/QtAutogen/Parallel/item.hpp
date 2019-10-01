#ifndef ITEM_HPP
#define ITEM_HPP

#include <QObject>
// Include ui_view.h in source and header
#include <ui_view.h>

class Item : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};

#endif
