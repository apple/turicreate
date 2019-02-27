#ifndef SOBJA_HPP
#define SOBJA_HPP

#include <QObject>

// Object source includes externally generated .moc file
class SObjA : public QObject
{
  Q_OBJECT
public:
  SObjA();
  ~SObjA();
};

#endif
