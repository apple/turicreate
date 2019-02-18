#include "item.hpp"
// Include ui_view.h only in header

namespace aaa {

class MocLocal : public QObject
{
  Q_OBJECT;

public:
  MocLocal() = default;
  ~MocLocal() = default;
};

void Item::go()
{
  Ui_ViewAAA ui;
  MocLocal obj;
}
}

#include "aaa/item.moc"
