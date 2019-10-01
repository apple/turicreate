#include "macos_fw_lib.h"

#include <QString>
#include <QtGlobal>

MacosFWLib::MacosFWLib()
{
}

MacosFWLib::~MacosFWLib()
{
}

QString MacosFWLib::qtVersionString() const
{
  return QString(qVersion());
}
