#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <QObject>

class Object : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int test READ getTest)
public:
  Object();

  int getTest() { return _test; }

  Q_SLOT
  void aSlot();

private:
  int _test;
};

#endif
