#include "IncB.hpp"

/// @brief Source local QObject
///
class IncBPrivate : public QObject
{
  Q_OBJECT
public:
  IncBPrivate(){};
};

IncB::IncB()
{
  IncBPrivate priv;
}

/// AUTOMOC moc_ include on the last line of the file!
#include "IncB.moc"
#include "moc_IncB.cpp"
