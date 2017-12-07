#ifndef OBJB_HPP
#define OBJB_HPP

#include <QObject>

class ObjB : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};

#endif
