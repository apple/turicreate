#ifndef OBJ_HPP
#define OBJ_HPP

#include <QObject>

// Object source comes without any _moc/.moc includes
class ObjPrivate;
class Obj : public QObject
{
  Q_OBJECT
public:
  Obj();
  ~Obj();

private:
  ObjPrivate* const d;
};

#endif
