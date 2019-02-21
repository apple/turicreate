/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef BAR_H
#define BAR_H

#include <QObject>

class Bar : public QObject
{
  Q_OBJECT
public:
  Bar();
public slots:
  void doBar();
};

#endif
