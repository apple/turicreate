#include "ObjA.hpp"
#include "ObjB.hpp"
#include "ObjC.hpp"

int main(int argv, char** args)
{
  ObjA objA;
  ObjB objB;
  ObjC objC;
  return 0;
}

// Header in global subdirectory
#include "subB/moc_SubObjB.cpp"
