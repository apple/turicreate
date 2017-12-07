
#include "klocalizedstring.h"

QString tr2xi18n(const char* text, const char*)
{
  return QLatin1String("TranslatedX") + QString::fromLatin1(text);
}

QString tr2i18n(const char* text, const char*)
{
  return QLatin1String("Translated") + QString::fromLatin1(text);
}
