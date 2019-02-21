#ifndef OBJECT2ALIASED_HPP
#define OBJECT2ALIASED_HPP

#include "CustomMacros.hpp"

// Test Qt object macro hidden in a macro (AUTOMOC_MACRO_NAMES)
class Object2Aliased : public QObject
{
  QO2_ALIAS
public:
  Object2Aliased();

signals:
  void aSignal();

public slots:
  void aSlot();
};

#endif
