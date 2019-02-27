#include <QObject>
#include <QString>

#include "macos_fw_lib.h"
#include "testMacosFWLib.h"

class TestMacosFWLib : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanupTestCase();
  void init() {}
  void cleanup() {}

  void testQtVersion();
};

void TestMacosFWLib::initTestCase()
{
}

void TestMacosFWLib::cleanupTestCase()
{
}

void TestMacosFWLib::testQtVersion()
{
  MacosFWLib* testLib = new MacosFWLib();
  QVERIFY(testLib->qtVersionString().contains("5."));
  testLib->deleteLater();
}

int main(int argc, char* argv[])
{
  QApplication app(argc, argv, false);
  MacosFWLib testObject;
  return QTest::qExec(&testObject, argc, argv);
}

#include "testMacosFWLib.moc"
