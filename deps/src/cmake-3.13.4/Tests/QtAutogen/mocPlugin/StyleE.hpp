#ifndef STYLEE_HPP
#define STYLEE_HPP

#include "StyleCommon.hpp"
#include <QStylePlugin>

class StyleE : public QStylePlugin
{
  Q_OBJECT
  // No Json file
  Q_PLUGIN_METADATA(IID "org.styles.E")
  A_CUSTOM_MACRO(SomeArg, InvalidFileArg, AnotherArg)
public:
  QStyle* create(const QString& key);
};

#endif
