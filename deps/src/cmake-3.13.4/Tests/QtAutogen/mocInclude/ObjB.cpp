#include "ObjB.hpp"
#include "ObjB_p.h"

ObjBPrivate::ObjBPrivate()
{
}

ObjBPrivate::~ObjBPrivate()
{
}

ObjB::ObjB()
  : d(new ObjBPrivate)
{
}

ObjB::~ObjB()
{
  delete d;
}

#include "moc_ObjB.cpp"
