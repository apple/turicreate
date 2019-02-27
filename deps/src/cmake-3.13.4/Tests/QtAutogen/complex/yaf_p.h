/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef YAF_P_H
#define YAF_P_H

#include <QObject>

#include <stdio.h>

class YafP : public QObject
{
  Q_OBJECT
public:
  YafP() {}
public slots:
  void doYafP() { printf("I am yet another file !\n"); }
};

#endif
