#ifndef LOBJA_HPP
#define LOBJA_HPP

#include <QObject>

// Object source comes with a .moc include
class LObjAPrivate;
class LObjA : public QObject
{
  Q_OBJECT
public:
  LObjA();
  ~LObjA();

private:
  LObjAPrivate* const d;
};

#endif
