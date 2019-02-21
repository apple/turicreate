#include "classa.h"
#include <QDebug>

void ClassA::slotDoSomething()
{
  qDebug() << m_member;
}
