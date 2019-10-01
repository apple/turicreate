#include "Obj.hpp"
#include "Obj_p.h"

ObjPrivate::ObjPrivate()
{
}

ObjPrivate::~ObjPrivate()
{
}

Obj::Obj()
  : d(new ObjPrivate)
{
}

Obj::~Obj()
{
  delete d;
}
