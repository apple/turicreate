/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "abc.h"
#include "abc_p.h"

#include <stdio.h>

class PrintAbc : public QObject
{
  Q_OBJECT
public:
  PrintAbc()
    : QObject()
  {
  }
public slots:
  void print() const { printf("abc\n"); }
};

Abc::Abc()
  : QObject()
{
}

void Abc::doAbc()
{
  PrintAbc pa;
  pa.print();
  AbcP abcP;
  abcP.doAbcP();
}

// check that including the moc file for the cpp file and the header works:
#include "abc.moc"
#include "moc_abc.cpp"
#include "moc_abc_p.cpp"

// check that including a moc file from another header works:
#include "moc_xyz.cpp"
