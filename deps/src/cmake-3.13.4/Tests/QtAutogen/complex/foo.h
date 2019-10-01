/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef FOO_H
#define FOO_H

#include <QObject>

class Foo
#ifdef FOO
  : public QObject
#endif
{
  Q_OBJECT
public:
  Foo();
public slots:
  void doFoo();
};

#endif
