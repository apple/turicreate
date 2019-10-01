#include "ObjA.hpp"
#include "ObjA_p.h"

ObjAPrivate::ObjAPrivate()
{
}

ObjAPrivate::~ObjAPrivate()
{
}

ObjA::ObjA()
  : d(new ObjAPrivate)
{
}

ObjA::~ObjA()
{
  delete d;
}
