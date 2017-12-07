/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "WarningMessagesDialog.h"

WarningMessagesDialog::WarningMessagesDialog(QWidget* prnt, QCMake* instance)
  : QDialog(prnt)
  , cmakeInstance(instance)
{
  this->setupUi(this);
  this->setInitialValues();
  this->setupSignals();
}

void WarningMessagesDialog::setInitialValues()
{
  this->suppressDeveloperWarnings->setChecked(
    this->cmakeInstance->getSuppressDevWarnings());
  this->suppressDeprecatedWarnings->setChecked(
    this->cmakeInstance->getSuppressDeprecatedWarnings());

  this->developerWarningsAsErrors->setChecked(
    this->cmakeInstance->getDevWarningsAsErrors());
  this->deprecatedWarningsAsErrors->setChecked(
    this->cmakeInstance->getDeprecatedWarningsAsErrors());
}

void WarningMessagesDialog::setupSignals()
{
  QObject::connect(this->buttonBox, SIGNAL(accepted()), this,
                   SLOT(doAccept()));

  QObject::connect(this->suppressDeveloperWarnings, SIGNAL(stateChanged(int)),
                   this, SLOT(doSuppressDeveloperWarningsChanged(int)));
  QObject::connect(this->suppressDeprecatedWarnings, SIGNAL(stateChanged(int)),
                   this, SLOT(doSuppressDeprecatedWarningsChanged(int)));

  QObject::connect(this->developerWarningsAsErrors, SIGNAL(stateChanged(int)),
                   this, SLOT(doDeveloperWarningsAsErrorsChanged(int)));
  QObject::connect(this->deprecatedWarningsAsErrors, SIGNAL(stateChanged(int)),
                   this, SLOT(doDeprecatedWarningsAsErrorsChanged(int)));
}

void WarningMessagesDialog::doAccept()
{
  this->cmakeInstance->setSuppressDevWarnings(
    this->suppressDeveloperWarnings->isChecked());
  this->cmakeInstance->setSuppressDeprecatedWarnings(
    this->suppressDeprecatedWarnings->isChecked());

  this->cmakeInstance->setDevWarningsAsErrors(
    this->developerWarningsAsErrors->isChecked());
  this->cmakeInstance->setDeprecatedWarningsAsErrors(
    this->deprecatedWarningsAsErrors->isChecked());
}

void WarningMessagesDialog::doSuppressDeveloperWarningsChanged(int state)
{
  // no warnings implies no errors either
  if (state) {
    this->developerWarningsAsErrors->setChecked(false);
  }
}

void WarningMessagesDialog::doSuppressDeprecatedWarningsChanged(int state)
{
  // no warnings implies no errors either
  if (state) {
    this->deprecatedWarningsAsErrors->setChecked(false);
  }
}

void WarningMessagesDialog::doDeveloperWarningsAsErrorsChanged(int state)
{
  // warnings as errors implies warnings are not suppressed
  if (state) {
    this->suppressDeveloperWarnings->setChecked(false);
  }
}

void WarningMessagesDialog::doDeprecatedWarningsAsErrorsChanged(int state)
{
  // warnings as errors implies warnings are not suppressed
  if (state) {
    this->suppressDeprecatedWarnings->setChecked(false);
  }
}
