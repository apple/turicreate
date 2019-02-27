#include "EObjA.hpp"
#include "EObjAExtra.hpp"
#include "EObjA_p.hpp"

class EObjALocal : public QObject
{
  Q_OBJECT
public:
  EObjALocal();
  ~EObjALocal();
};

EObjALocal::EObjALocal()
{
}

EObjALocal::~EObjALocal()
{
}

EObjAPrivate::EObjAPrivate()
{
  EObjALocal localObj;
  EObjAExtra extraObj;
}

EObjAPrivate::~EObjAPrivate()
{
}

EObjA::EObjA()
  : d(new EObjAPrivate)
{
}

EObjA::~EObjA()
{
  delete d;
}

// For EObjALocal
#include "EObjA.moc"
// - Not the own header
#include "moc_EObjAExtra.cpp"
