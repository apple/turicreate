#ifndef CLASSB_H
#define CLASSB_H

#include <QObject>
#include <QString>

class ClassB : public QObject
{
  Q_OBJECT
public:
  ClassB()
    : m_member("Hello B")
  {
  }

public slots:
  void slotDoSomething();

private:
  QString m_member;
};

#endif
