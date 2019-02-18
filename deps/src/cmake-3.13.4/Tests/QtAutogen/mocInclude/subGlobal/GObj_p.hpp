#ifndef GOBJ_P_HPP
#define GOBJ_P_HPP

#include <QObject>

namespace subGlobal {

class GObjPrivate : public QObject
{
  Q_OBJECT
public:
  GObjPrivate();
  ~GObjPrivate();
};
}

#endif
