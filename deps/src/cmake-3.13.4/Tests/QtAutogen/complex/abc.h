/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef ABC_H
#define ABC_H

#include <QObject>

class Abc : public QObject
{
  Q_OBJECT
public:
  Abc();
public slots:
  void doAbc();
};

#endif
