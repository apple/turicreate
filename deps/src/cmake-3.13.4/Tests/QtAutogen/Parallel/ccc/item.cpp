#include "item.hpp"
// Include ui_view.h in source and header
#include <ccc/ui_view.h>

namespace ccc {

class MocLocal : public QObject
{
  Q_OBJECT;

public:
  MocLocal() = default;
  ~MocLocal() = default;
};

void Item::go()
{
  Ui_ViewCCC ui;
  MocLocal obj;
}
}

// Include own moc files
#include "ccc/item.moc"
#include "moc_item.cpp"
