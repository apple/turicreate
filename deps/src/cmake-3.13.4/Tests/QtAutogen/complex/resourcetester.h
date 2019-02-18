
#ifndef RESOURCE_TESTER_H
#define RESOURCE_TESTER_H

#include <QObject>

class ResourceTester : public QObject
{
  Q_OBJECT
public:
  explicit ResourceTester(QObject* parent = 0);

private slots:
  void doTest();
};

#endif
