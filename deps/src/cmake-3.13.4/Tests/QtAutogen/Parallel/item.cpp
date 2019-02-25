#include "item.hpp"
// Include ui_view.h in source and header
#include <ui_view.h>

class MocLocal : public QObject
{
  Q_OBJECT;

public:
  MocLocal() = default;
  ~MocLocal() = default;
};

void Item::go()
{
  Ui_View ui;
  MocLocal obj;
}

#include "item.moc"
