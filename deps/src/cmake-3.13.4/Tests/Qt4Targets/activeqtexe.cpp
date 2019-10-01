
#include <QApplication>

#ifndef QT_QAXSERVER_LIB
#  error Expected QT_QAXSERVER_LIB
#endif

#include <QAxFactory>

class MyObject : public QObject
{
  Q_OBJECT
public:
  MyObject(QObject* parent = 0)
    : QObject(parent)
  {
  }
};

QAXFACTORY_DEFAULT(MyObject, "{4dc3f340-a6f7-44e4-a79b-3e9217685fbd}",
                   "{9ee49617-7d5c-441a-b833-4b068d41d751}",
                   "{13eca64b-ee2a-4f3c-aa04-5d9d975779a7}",
                   "{ce947ee3-0403-4fdc-895a-4fe779344b46}",
                   "{8de435ce-8d2a-46ac-b3b3-cb800d0547c7}");

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  QAxFactory::isServer();

  return app.exec();
}

#include "activeqtexe.moc"
