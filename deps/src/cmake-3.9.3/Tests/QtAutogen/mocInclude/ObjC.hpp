#ifndef OBJC_HPP
#define OBJC_HPP

#include <QObject>

class ObjC : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};

#endif
