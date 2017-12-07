/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef QCMakeWidgets_h
#define QCMakeWidgets_h

#include "cmConfigure.h"

#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>

class QToolButton;

// common widgets for Qt based CMake

/// Editor widget for editing paths or file paths
class QCMakeFileEditor : public QLineEdit
{
  Q_OBJECT
public:
  QCMakeFileEditor(QWidget* p, const QString& var);
protected slots:
  virtual void chooseFile() = 0;
signals:
  void fileDialogExists(bool);

protected:
  void resizeEvent(QResizeEvent* e);
  QToolButton* ToolButton;
  QString Variable;
};

/// editor widget for editing files
class QCMakePathEditor : public QCMakeFileEditor
{
  Q_OBJECT
public:
  QCMakePathEditor(QWidget* p = CM_NULLPTR, const QString& var = QString());
  void chooseFile();
};

/// editor widget for editing paths
class QCMakeFilePathEditor : public QCMakeFileEditor
{
  Q_OBJECT
public:
  QCMakeFilePathEditor(QWidget* p = CM_NULLPTR,
                       const QString& var = QString());
  void chooseFile();
};

/// completer class that returns native cmake paths
class QCMakeFileCompleter : public QCompleter
{
  Q_OBJECT
public:
  QCMakeFileCompleter(QObject* o, bool dirs);
  virtual QString pathFromIndex(const QModelIndex& idx) const;
};

// editor for strings
class QCMakeComboBox : public QComboBox
{
  Q_OBJECT
  Q_PROPERTY(QString value READ currentText WRITE setValue USER true);

public:
  QCMakeComboBox(QWidget* p, QStringList strings)
    : QComboBox(p)
  {
    this->addItems(strings);
  }
  void setValue(const QString& v)
  {
    int i = this->findText(v);
    if (i != -1) {
      this->setCurrentIndex(i);
    }
  }
};

#endif
