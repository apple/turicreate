#ifndef EOBJB_HPP
#define EOBJB_HPP

#include <QObject>

// Sources includes a moc_ includes of an extra object in a subdirectory
class EObjBPrivate;
class EObjB : public QObject
{
  Q_OBJECT
public:
  EObjB();
  ~EObjB();

private:
  EObjBPrivate* const d;
};

#endif
