/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class QCalendarWidget;
class QCheckBox;
class QComboBox;
class QDate;
class QDateEdit;
class QGridLayout;
class QGroupBox;
class QLabel;

namespace Ui {
class Window;
}

class Window : public QWidget
{
  Q_OBJECT

public:
  Window();

private slots:
  void localeChanged(int index);
  void firstDayChanged(int index);
  void selectionModeChanged(int index);
  void horizontalHeaderChanged(int index);
  void verticalHeaderChanged(int index);
  void selectedDateChanged();
  void minimumDateChanged(const QDate& date);
  void maximumDateChanged(const QDate& date);
  void weekdayFormatChanged();
  void weekendFormatChanged();
  void reformatHeaders();
  void reformatCalendarPage();

private:
  void createPreviewGroupBox();
  void createGeneralOptionsGroupBox();
  void createDatesGroupBox();
  void createTextFormatsGroupBox();
  QComboBox* createColorComboBox();

  QGroupBox* previewGroupBox;
  QGridLayout* previewLayout;
  QCalendarWidget* calendar;

  QGroupBox* generalOptionsGroupBox;
  QLabel* localeLabel;
  QLabel* firstDayLabel;
  QLabel* selectionModeLabel;
  QLabel* horizontalHeaderLabel;
  QLabel* verticalHeaderLabel;
  QComboBox* localeCombo;
  QComboBox* firstDayCombo;
  QComboBox* selectionModeCombo;
  QCheckBox* gridCheckBox;
  QCheckBox* navigationCheckBox;
  QComboBox* horizontalHeaderCombo;
  QComboBox* verticalHeaderCombo;

  QGroupBox* datesGroupBox;
  QLabel* currentDateLabel;
  QLabel* minimumDateLabel;
  QLabel* maximumDateLabel;
  QDateEdit* currentDateEdit;
  QDateEdit* minimumDateEdit;
  QDateEdit* maximumDateEdit;

  QGroupBox* textFormatsGroupBox;
  QLabel* weekdayColorLabel;
  QLabel* weekendColorLabel;
  QLabel* headerTextFormatLabel;
  QComboBox* weekdayColorCombo;
  QComboBox* weekendColorCombo;
  QComboBox* headerTextFormatCombo;

  QCheckBox* firstFridayCheckBox;
  QCheckBox* mayFirstCheckBox;

  Ui::Window* ui;
};

#endif
