#ifndef OBJA_HPP
#define OBJA_HPP

#include <QObject>

// Object source comes without any _moc/.moc includes
class ObjAPrivate;
class ObjA : public QObject
{
  Q_OBJECT
public:
  ObjA();
  ~ObjA();

private:
  ObjAPrivate* const d;
};

#endif
