#include "EObjA.hpp"
#include "EObjB.hpp"
#include "LObjA.hpp"
#include "LObjB.hpp"
#include "ObjA.hpp"
#include "ObjB.hpp"
#include "SObjA.hpp"
#include "SObjB.hpp"
#include "subGlobal/GObj.hpp"

int main(int argv, char** args)
{
  subGlobal::GObj gObj;
  ObjA objA;
  ObjB objB;
  LObjA lObjA;
  LObjB lObjB;
  EObjA eObjA;
  EObjB eObjB;
  SObjA sObjA;
  SObjB sObjB;
  return 0;
}

// Header in global subdirectory
#include "subGlobal/moc_GObj.cpp"
