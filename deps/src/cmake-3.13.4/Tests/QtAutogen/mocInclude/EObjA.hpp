#ifndef EOBJA_HPP
#define EOBJA_HPP

#include <QObject>

// Sources includes a moc_ includes of an extra object
class EObjAPrivate;
class EObjA : public QObject
{
  Q_OBJECT
public:
  EObjA();
  ~EObjA();

private:
  EObjAPrivate* const d;
};

#endif
