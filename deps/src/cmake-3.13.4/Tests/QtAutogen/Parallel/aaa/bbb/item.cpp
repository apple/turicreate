#include "item.hpp"

namespace aaa {
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
  MocLocal obj;
}
}
}

#include "aaa/bbb/item.moc"
