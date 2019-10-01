/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef ABC_P_H
#define ABC_P_H

#include <QObject>

#include <stdio.h>

class AbcP : public QObject
{
  Q_OBJECT
public:
  AbcP() {}
public slots:
  void doAbcP() { printf("I am private abc !\n"); }
};

#endif
