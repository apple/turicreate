#ifndef MACOSFWLIB_H
#define MACOSFWLIB_H

#include <QObject>
#include <QString>

class __attribute__((visibility("default"))) MacosFWLib : public QObject
{
  Q_OBJECT

public:
  explicit MacosFWLib();
  ~MacosFWLib();

  QString qtVersionString() const;
};

#endif // MACOSFWLIB_H
