#include "qItemC.hpp"

class QItemC_Local : public QObject
{
  Q_OBJECT
public:
  QItemC_Local(){};
  ~QItemC_Local(){};
};

void QItemC::go()
{
  QItemC_Local localObject;
}

// We need AUTOMOC processing
#include "qItemC.moc"
