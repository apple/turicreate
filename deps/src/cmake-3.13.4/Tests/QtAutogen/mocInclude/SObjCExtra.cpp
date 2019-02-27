#include "SObjCExtra.hpp"

class SObjCLocalExtra : public QObject
{
  Q_OBJECT

public:
  SObjCLocalExtra();
  ~SObjCLocalExtra();
};

SObjCLocalExtra::SObjCLocalExtra()
{
}

SObjCLocalExtra::~SObjCLocalExtra()
{
}

SObjCExtra::SObjCExtra()
{
}

SObjCExtra::~SObjCExtra()
{
}

// Externally generated header moc
#include "SObjCExtra_extMoc.cpp"
// AUTOMOC generated source moc
#include "SObjCExtra.moc"
