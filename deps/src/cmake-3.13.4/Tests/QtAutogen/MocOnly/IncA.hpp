#ifndef INCA_HPP
#define INCA_HPP

#include <QObject>

/// @brief Test moc include pattern in the source file
///
class IncA : public QObject
{
  Q_OBJECT
public:
  IncA();
};

#endif
