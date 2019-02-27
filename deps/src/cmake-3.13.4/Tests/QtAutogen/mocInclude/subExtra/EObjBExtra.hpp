#ifndef EOBJBEXTRA_HPP
#define EOBJBEXTRA_HPP

#include <QObject>

class EObjBExtraPrivate;
class EObjBExtra : public QObject
{
  Q_OBJECT
public:
  EObjBExtra();
  ~EObjBExtra();

private:
  EObjBExtraPrivate* const d;
};

#endif
