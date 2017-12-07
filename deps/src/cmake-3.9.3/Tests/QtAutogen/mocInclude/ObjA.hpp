#ifndef OBJA_HPP
#define OBJA_HPP

#include <QObject>

class ObjA : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};

#endif
