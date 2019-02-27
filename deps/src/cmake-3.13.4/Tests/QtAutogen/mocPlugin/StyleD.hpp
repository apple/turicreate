#ifndef STYLED_HPP
#define STYLED_HPP

#include "StyleCommon.hpp"
#include <QStylePlugin>

class StyleD : public QStylePlugin
{
  Q_OBJECT
  // Json file in global sub director
  Q_PLUGIN_METADATA(IID "org.styles.D" FILE "sub/StyleD.json")
  A_CUSTOM_MACRO(SomeArg, "sub/StyleD_Custom.json", AnotherArg)
public:
  QStyle* create(const QString& key);
};

#endif
