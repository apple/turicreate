#include "item.hpp"

namespace bbb {
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
  MocLocal obj;
}
}
}

#include "bbb/aaa/item.moc"
