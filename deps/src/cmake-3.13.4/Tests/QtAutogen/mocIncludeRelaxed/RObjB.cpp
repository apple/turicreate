#include "RObjB.hpp"
#include "RObjBExtra.hpp"

RObjBExtra::RObjBExtra()
{
}

RObjBExtra::~RObjBExtra()
{
}

RObjB::RObjB()
{
  RObjBExtra extraObject;
}

RObjB::~RObjB()
{
}

// Relaxed mode should run moc on RObjBExtra.hpp instead
#include "RObjBExtra.moc"
