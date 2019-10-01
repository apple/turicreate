/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "blub.h"

#include <stdio.h>

class BlubBlub : public QObject
{
  Q_OBJECT
public:
  BlubBlub()
    : QObject()
  {
  }
public slots:
  int getValue() const { return 13; }
};

Blub::Blub()
{
}

void Blub::blubber()
{
  BlubBlub bb;
  printf("Blub blub %d ! \n", bb.getValue());
}

// test the case that the wrong moc-file is included, it should
// actually be "blub.moc"
#include "moc_blub.cpp"
