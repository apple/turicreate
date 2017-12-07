/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef XYZ_H
#define XYZ_H

#include <QObject>

class Xyz : public QObject
{
  Q_OBJECT
public:
  Xyz();
public slots:
  void doXyz();
};

#endif
