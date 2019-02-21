#include "GObj.hpp"
#include "GObj_p.hpp"

namespace subGlobal {

class GObjLocal : public QObject
{
  Q_OBJECT
public:
  GObjLocal();
  ~GObjLocal();
};

GObjLocal::GObjLocal()
{
}

GObjLocal::~GObjLocal()
{
}

GObjPrivate::GObjPrivate()
{
}

GObjPrivate::~GObjPrivate()
{
}

GObj::GObj()
{
  GObjLocal localObj;
}

GObj::~GObj()
{
}
}

// For the local QObject
#include "GObj.moc"
