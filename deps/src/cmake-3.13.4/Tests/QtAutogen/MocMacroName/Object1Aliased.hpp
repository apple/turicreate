#ifndef OBJECTALIASED_HPP
#define OBJECTALIASED_HPP

#include "CustomMacros.hpp"

// Test Qt object macro hidden in a macro (AUTOMOC_MACRO_NAMES)
class Object1Aliased : public QObject
{
  QO1_ALIAS
public:
  Object1Aliased();

signals:
  void aSignal();

public slots:
  void aSlot();
};

#endif
