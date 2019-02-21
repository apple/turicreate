#include "RObjC.hpp"
#include <QObject>

class RObjCPrivate : public QObject
{
  Q_OBJECT
public:
  RObjCPrivate();
  ~RObjCPrivate();
};

RObjCPrivate::RObjCPrivate()
{
}

RObjCPrivate::~RObjCPrivate()
{
}

RObjC::RObjC()
{
  RObjCPrivate privateObject;
}

RObjC::~RObjC()
{
}

// Relaxed include should moc this source instead of the header
#include "moc_RObjC.cpp"
