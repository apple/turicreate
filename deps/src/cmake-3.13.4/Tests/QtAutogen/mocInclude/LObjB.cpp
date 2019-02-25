#include "LObjB.hpp"
#include "LObjB_p.h"

class LObjBLocal : public QObject
{
  Q_OBJECT
public:
  LObjBLocal();
  ~LObjBLocal();
};

LObjBLocal::LObjBLocal()
{
}

LObjBLocal::~LObjBLocal()
{
}

LObjBPrivate::LObjBPrivate()
{
  LObjBLocal localObj;
}

LObjBPrivate::~LObjBPrivate()
{
}

LObjB::LObjB()
  : d(new LObjBPrivate)
{
}

LObjB::~LObjB()
{
  delete d;
}

#include "LObjB.moc"
#include "moc_LObjB.cpp"
