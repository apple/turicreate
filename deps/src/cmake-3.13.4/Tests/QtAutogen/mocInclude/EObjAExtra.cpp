#include "EObjAExtra.hpp"
#include "EObjAExtra_p.hpp"

EObjAExtraPrivate::EObjAExtraPrivate()
{
}

EObjAExtraPrivate::~EObjAExtraPrivate()
{
}

EObjAExtra::EObjAExtra()
  : d(new EObjAExtraPrivate)
{
}

EObjAExtra::~EObjAExtra()
{
  delete d;
}
