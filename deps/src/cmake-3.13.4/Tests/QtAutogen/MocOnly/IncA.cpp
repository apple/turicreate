#include "moc_IncA.cpp"
/// AUTOMOC moc_ include on the first line of the file!
#include "IncA.hpp"

/// @brief Source local QObject
///
class IncAPrivate : public QObject
{
  Q_OBJECT
public:
  IncAPrivate(){};
};

IncA::IncA()
{
  IncAPrivate priv;
}

#include "IncA.moc"
