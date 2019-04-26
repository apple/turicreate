#include "item.hpp"
// Include ui_view.h only in source
#include <bbb/ui_view.h>

namespace bbb {

class MocLocal : public QObject
{
  Q_OBJECT;

public:
  MocLocal() = default;
  ~MocLocal() = default;
};

void Item::go()
{
  Ui_ViewBBB ui;
  MocLocal obj;
}
}

#include "bbb/item.moc"
