#ifndef GOBJ_HPP
#define GOBJ_HPP

#include <QObject>

namespace subGlobal {

class GObj : public QObject
{
  Q_OBJECT
public:
  GObj();
  ~GObj();
};
}

#endif
