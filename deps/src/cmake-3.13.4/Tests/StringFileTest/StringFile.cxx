#include <stdio.h>
#include <string.h>

#include "OutputFile.h"

int main(int, char* [])
{
  int res = 0;

  res += CheckMethod(rmvar, "CMake");
  res += CheckMethod(rmallvar, "CMake;cmake;CMake");
  res += CheckMethod(rrepvar, "People should use CMake and CMake");
  res += CheckMethod(repvar, "People should use CMake");
  res += CheckMethod(substringres, "Everybody should use CMake");
  res += CheckMethod(nceqvar, "0");
  res += CheckMethod(lengthres, "26");
  res += CheckMethod(ceqvar, "1");
  res += CheckMethod(cneqvar, "1");
  res += CheckMethod(ncneqvar, "0");
  res += CheckMethod(nclvar, "0");
  res += CheckMethod(clvar, "1");
  res += CheckMethod(cgvar, "1");
  res += CheckMethod(ncgvar, "0");
  res += CheckMethod(savar, "Cmake");
  res += CheckMethod(tuvar, "CMAKE");
  res += CheckMethod(tlvar, "cmake");
  res += CheckMethod(relpath, "../../X11R6/bin/xnest");
  res += CheckMethod(configvar,
                     "@$@$junk =~ s/#$xyz#/$foo_bar{$wibble}->{$xyz}/;@@");

  return res;
}
