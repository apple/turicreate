#include "EObjBExtra.hpp"
#include "EObjBExtra_p.hpp"

EObjBExtraPrivate::EObjBExtraPrivate()
{
}

EObjBExtraPrivate::~EObjBExtraPrivate()
{
}

EObjBExtra::EObjBExtra()
  : d(new EObjBExtraPrivate)
{
}

EObjBExtra::~EObjBExtra()
{
  delete d;
}
