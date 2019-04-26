#ifndef OBJ_P_HPP
#define OBJ_P_HPP

#include <QObject>

class ObjPrivate : public QObject
{
  Q_OBJECT
public:
  ObjPrivate();
  ~ObjPrivate();
};

#endif
