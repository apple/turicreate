
#include <QObject>

#include "myinterface.h"

class MyObject
  : public QObject
  , MyInterface
{
  Q_OBJECT
  Q_INTERFACES(MyInterface)
public:
  explicit MyObject(QObject* parent = 0)
    : QObject(parent)
  {
  }
};

int main(int argc, char** argv)
{
  MyObject mo;
  mo.objectName();
  return 0;
}

#include "main_gen_test.moc"
