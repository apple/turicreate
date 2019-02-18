/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "foo.h"

#include <stdio.h>

class FooFoo : public QObject
{
  Q_OBJECT
public:
  FooFoo()
    : QObject()
  {
  }
public slots:
  int getValue() const { return 12; }
};

Foo::Foo()
  : QObject()
{
}

void Foo::doFoo()
{
  FooFoo ff;
  printf("Hello automoc: %d\n", ff.getValue());
}

#include "foo.moc"
