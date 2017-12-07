/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef AddCacheEntry_h
#define AddCacheEntry_h

#include "QCMake.h"

#include <QCheckBox>
#include <QStringList>
#include <QWidget>

#include "ui_AddCacheEntry.h"

class AddCacheEntry : public QWidget, public Ui::AddCacheEntry
{
  Q_OBJECT
public:
  AddCacheEntry(QWidget* p, const QStringList& varNames,
                const QStringList& varTypes);

  QString name() const;
  QVariant value() const;
  QString description() const;
  QCMakeProperty::PropertyType type() const;
  QString typeString() const;

private slots:
  void onCompletionActivated(const QString& text);

private:
  const QStringList& VarNames;
  const QStringList& VarTypes;
};

#endif
