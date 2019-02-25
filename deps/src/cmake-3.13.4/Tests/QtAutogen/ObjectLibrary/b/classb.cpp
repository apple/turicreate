#include "classb.h"
#include <QDebug>

void ClassB::slotDoSomething()
{
  qDebug() << m_member;
}
