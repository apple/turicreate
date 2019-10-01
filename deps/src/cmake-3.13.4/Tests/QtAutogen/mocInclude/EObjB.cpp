#include "EObjB.hpp"
#include "EObjB_p.hpp"
#include "subExtra/EObjBExtra.hpp"

class EObjBLocal : public QObject
{
  Q_OBJECT
public:
  EObjBLocal();
  ~EObjBLocal();
};

EObjBLocal::EObjBLocal()
{
}

EObjBLocal::~EObjBLocal()
{
}

EObjBPrivate::EObjBPrivate()
{
  EObjBLocal localObj;
  EObjBExtra extraObj;
}

EObjBPrivate::~EObjBPrivate()
{
}

EObjB::EObjB()
  : d(new EObjBPrivate)
{
}

EObjB::~EObjB()
{
  delete d;
}

// For EObjBLocal
#include "EObjB.moc"
// - Not the own header
// - in a subdirectory
#include "subExtra/moc_EObjBExtra.cpp"
