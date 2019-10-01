#include "LObjA.hpp"
#include "LObjA_p.h"

class LObjALocal : public QObject
{
  Q_OBJECT
public:
  LObjALocal();
  ~LObjALocal();
};

LObjALocal::LObjALocal()
{
}

LObjALocal::~LObjALocal()
{
}

LObjAPrivate::LObjAPrivate()
{
  LObjALocal localObj;
}

LObjAPrivate::~LObjAPrivate()
{
}

LObjA::LObjA()
  : d(new LObjAPrivate)
{
}

LObjA::~LObjA()
{
  delete d;
}

#include "LObjA.moc"
