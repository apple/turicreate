#ifndef ObjB_HPP
#define ObjB_HPP

#include <QObject>

// Object source comes with a _moc include
class ObjBPrivate;
class ObjB : public QObject
{
  Q_OBJECT
public:
  ObjB();
  ~ObjB();

private:
  ObjBPrivate* const d;
};

#endif
