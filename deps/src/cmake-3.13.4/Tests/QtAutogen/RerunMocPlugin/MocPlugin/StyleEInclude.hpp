#ifndef STYLEE_INCLUDE_HPP
#define STYLEE_INCLUDE_HPP

#include "UtilityMacros.hpp"
#include <QStylePlugin>

class StyleE : public QStylePlugin
{
  Q_OBJECT
  // Json files in global root directory
  Q_PLUGIN_METADATA(IID "org.styles.E" FILE "StyleE.json")
  A_CUSTOM_MACRO(SomeArg, "StyleE_Custom.json", AnotherArg)
public:
  QStyle* create(const QString& key);
};

#endif
