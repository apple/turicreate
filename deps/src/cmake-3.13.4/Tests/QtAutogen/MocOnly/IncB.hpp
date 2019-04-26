#ifndef INCB_HPP
#define INCB_HPP

#include <QObject>

/// @brief Test moc include pattern in the source file
///
class IncB : public QObject
{
  Q_OBJECT
public:
  IncB();
};

#endif
