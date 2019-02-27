/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "sub/bar.h"

#include <stdio.h>

Bar::Bar()
  : QObject()
{
}

void Bar::doBar()
{
  printf("Hello bar !\n");
}

#include "sub/moc_bar.cpp"
