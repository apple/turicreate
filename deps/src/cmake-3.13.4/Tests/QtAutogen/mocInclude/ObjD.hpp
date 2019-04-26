#ifndef OBJD_HPP
#define OBJD_HPP

#include <QObject>

class ObjD : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};

#endif
