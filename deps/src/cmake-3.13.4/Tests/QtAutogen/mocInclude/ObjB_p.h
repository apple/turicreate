#ifndef OBJB_P_HPP
#define OBJB_P_HPP

#include <QObject>

class ObjBPrivate : public QObject
{
  Q_OBJECT
public:
  ObjBPrivate();
  ~ObjBPrivate();
};

#endif
