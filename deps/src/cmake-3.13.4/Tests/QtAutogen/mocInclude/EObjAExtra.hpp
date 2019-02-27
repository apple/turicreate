#ifndef EOBJAEXTRA_HPP
#define EOBJAEXTRA_HPP

#include <QObject>

class EObjAExtraPrivate;
class EObjAExtra : public QObject
{
  Q_OBJECT
public:
  EObjAExtra();
  ~EObjAExtra();

private:
  EObjAExtraPrivate* const d;
};

#endif
