#ifndef SOBJCEXTRA_HPP
#define SOBJCEXTRA_HPP

#include <QObject>

// Object source includes externally generated .moc file
class SObjCExtra : public QObject
{
  Q_OBJECT
public:
  SObjCExtra();
  ~SObjCExtra();
};

#endif
