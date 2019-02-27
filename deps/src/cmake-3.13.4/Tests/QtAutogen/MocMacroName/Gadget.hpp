#ifndef GADGET_HPP
#define GADGET_HPP

#include <QMetaType>

class Gadget
{
  Q_GADGET
  Q_PROPERTY(int test READ getTest)
public:
  Gadget();

  int getTest() { return _test; }

private:
  int _test;
};

#endif
