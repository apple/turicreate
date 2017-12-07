#include "item.hpp"

namespace ccc {

void Item::go()
{
}

class MocTest : public QObject
{
  Q_OBJECT;
  Q_SLOT
  void go();
};

void MocTest::go()
{
}
}

// Include own moc files
#include "item.moc"
#include "moc_item.cpp"
