#ifndef STYLEA_HPP
#define STYLEA_HPP

#include "StyleCommon.hpp"
#include <QStylePlugin>

class StyleA : public QStylePlugin
{
  Q_OBJECT
  // Json file in local directory
  Q_PLUGIN_METADATA(IID "org.styles.A" FILE "StyleA.json")
  A_CUSTOM_MACRO(SomeArg, "StyleA_Custom.json", AnotherArg)
public:
  QStyle* create(const QString& key);
};

#endif
