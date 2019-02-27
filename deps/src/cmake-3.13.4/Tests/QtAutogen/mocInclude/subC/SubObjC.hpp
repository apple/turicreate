#ifndef SUBOBJC_HPP
#define SUBOBJC_HPP

#include <QObject>

namespace subC {

class ObjC : public QObject
{
  Q_OBJECT
  Q_SLOT
  void go();
};
}

#endif
