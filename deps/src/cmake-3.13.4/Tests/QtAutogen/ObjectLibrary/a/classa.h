#ifndef CLASSA_H
#define CLASSA_H

#include <QObject>
#include <QString>

class ClassA : public QObject
{
  Q_OBJECT
public:
  ClassA()
    : m_member("Hello A")
  {
  }

public slots:
  void slotDoSomething();

private:
  QString m_member;
};

#endif
