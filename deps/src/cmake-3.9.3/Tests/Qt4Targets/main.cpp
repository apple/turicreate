
#include <QApplication>
#include <QWidget>

#include <QString>

#ifndef QT_CORE_LIB
#error Expected QT_CORE_LIB
#endif

#ifndef QT_GUI_LIB
#error Expected QT_GUI_LIB
#endif

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  QWidget w;
  w.setWindowTitle(QString::fromLatin1("SomeTitle"));
  w.show();

  return 0;
}
