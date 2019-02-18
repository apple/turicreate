#include "SObjC.hpp"

void SObjCLocalFunction();

class SObjCLocal : public QObject
{
  Q_OBJECT

public:
  SObjCLocal();
  ~SObjCLocal();
};

SObjCLocal::SObjCLocal()
{
}

SObjCLocal::~SObjCLocal()
{
}

SObjC::SObjC()
{
  SObjCLocal localObject;
  SObjCLocalFunction();
}

SObjC::~SObjC()
{
}

#include "SObjC.moc"
#include "moc_SObjC.cpp"
// Include moc_ file for which the header is SKIP_AUTOMOC enabled
#include "moc_SObjCExtra.cpp"
