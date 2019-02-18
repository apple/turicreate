#ifndef SOBJC_HPP
#define SOBJC_HPP

#include <QObject>

// Object source includes externally generated .moc file
class SObjC : public QObject
{
  Q_OBJECT
public:
  SObjC();
  ~SObjC();
};

#endif
