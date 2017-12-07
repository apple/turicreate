#ifndef SUBOBJB_HPP
#define SUBOBJB_HPP

#include <QObject>

namespace subB {

class ObjB : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}

#endif
