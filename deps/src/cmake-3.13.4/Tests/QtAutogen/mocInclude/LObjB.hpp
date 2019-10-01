#ifndef LLObjB_HPP
#define LLObjB_HPP

#include <QObject>

// Object source comes with a .moc and a _moc include
class LObjBPrivate;
class LObjB : public QObject
{
  Q_OBJECT
public:
  LObjB();
  ~LObjB();

private:
  LObjBPrivate* const d;
};

#endif
