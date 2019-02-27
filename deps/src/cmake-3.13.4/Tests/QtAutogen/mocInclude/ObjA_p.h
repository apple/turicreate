#ifndef OBJA_P_HPP
#define OBJA_P_HPP

#include <QObject>

class ObjAPrivate : public QObject
{
  Q_OBJECT
public:
  ObjAPrivate();
  ~ObjAPrivate();
};

#endif
