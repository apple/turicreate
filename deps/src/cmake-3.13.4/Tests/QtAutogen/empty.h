
#include <QObject>

class Empty : public QObject
{
  Q_OBJECT
public:
  explicit Empty(QObject* parent = 0) {}
};
