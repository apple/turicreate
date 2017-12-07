/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef WarningMessagesDialog_h
#define WarningMessagesDialog_h

#include <QDialog>
#include <QWidget>

#include "QCMake.h"
#include "ui_WarningMessagesDialog.h"

/**
 * Dialog window for setting the warning message related options.
 */
class WarningMessagesDialog : public QDialog, public Ui_MessagesDialog
{
  Q_OBJECT

public:
  WarningMessagesDialog(QWidget* prnt, QCMake* instance);

private slots:
  /**
   * Handler for the accept event of the ok/cancel button box.
   */
  void doAccept();

  /**
   * Handler for checked state changed event of the suppress developer warnings
   * checkbox.
   */
  void doSuppressDeveloperWarningsChanged(int state);
  /**
   * Handler for checked state changed event of the suppress deprecated
   * warnings checkbox.
   */
  void doSuppressDeprecatedWarningsChanged(int state);

  /**
   * Handler for checked state changed event of the developer warnings as
   * errors checkbox.
   */
  void doDeveloperWarningsAsErrorsChanged(int state);
  /**
   * Handler for checked state changed event of the deprecated warnings as
   * errors checkbox.
   */
  void doDeprecatedWarningsAsErrorsChanged(int state);

private:
  QCMake* cmakeInstance;

  /**
   * Set the initial values of the widgets on this dialog window, using the
   * current state of the cache.
   */
  void setInitialValues();

  /**
   * Setup the signals for the widgets on this dialog window.
   */
  void setupSignals();
};

#endif /* MessageDialog_h */
