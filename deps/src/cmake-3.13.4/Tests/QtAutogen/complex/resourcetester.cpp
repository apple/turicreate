
#include "resourcetester.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTimer>

ResourceTester::ResourceTester(QObject* parent)
  : QObject(parent)
{
}

void ResourceTester::doTest()
{
  if (!QFile::exists(":/CMakeLists.txt"))
    qApp->exit(EXIT_FAILURE);
  if (!QFile::exists(":/main.cpp"))
    qApp->exit(EXIT_FAILURE);
#ifdef TEST_DEBUG_CLASS
  if (!QFile::exists(":/debug_class.ui"))
    qApp->exit(EXIT_FAILURE);
#endif

  QTimer::singleShot(0, qApp, SLOT(quit()));
}
