#include "qItemD.hpp"

class QItemD_Local : public QObject
{
  Q_OBJECT
public:
  QItemD_Local(){};
  ~QItemD_Local(){};
};

void QItemD::go()
{
  QItemD_Local localObject;
}

// We need AUTOMOC processing
#include "qItemD.moc"
