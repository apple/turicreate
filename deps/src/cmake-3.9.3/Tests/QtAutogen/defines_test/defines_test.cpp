
#include <QObject>

#ifdef QT_GUI_LIB
#include <QTextDocument>

class SomeDocument : public QTextDocument
{
  Q_OBJECT

Q_SIGNALS:
  void someSig();
};
#endif

#ifdef QT_CORE_LIB
class SomeObject : public QObject
{
  Q_OBJECT

Q_SIGNALS:
  void someSig();
};
#endif

int main(int argc, char** argv)
{
#ifdef QT_CORE_LIB
  QMetaObject sosmo = SomeObject::staticMetaObject;
#endif
#ifdef QT_GUI_LIB
  QMetaObject sdsmo = SomeDocument::staticMetaObject;
#endif

  return 0;
}

#include "defines_test.moc"
