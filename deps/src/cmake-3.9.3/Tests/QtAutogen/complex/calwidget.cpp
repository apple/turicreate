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

#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTextCharFormat>

#include "calwidget.h"

#include "ui_calwidget.h"
#ifdef UI_CALWIDGET_H
#error Definition of UI_CALWIDGET_H should be disabled by file option.
#endif

Window::Window()
  : ui(new Ui::Window)
{
  createPreviewGroupBox();
  createGeneralOptionsGroupBox();
  createDatesGroupBox();
  createTextFormatsGroupBox();

  QGridLayout* layout = new QGridLayout;
  layout->addWidget(previewGroupBox, 0, 0);
  layout->addWidget(generalOptionsGroupBox, 0, 1);
  layout->addWidget(datesGroupBox, 1, 0);
  layout->addWidget(textFormatsGroupBox, 1, 1);
  layout->setSizeConstraint(QLayout::SetFixedSize);
  setLayout(layout);

  previewLayout->setRowMinimumHeight(0, calendar->sizeHint().height());
  previewLayout->setColumnMinimumWidth(0, calendar->sizeHint().width());

  setWindowTitle(tr("Calendar Widget"));
}

void Window::localeChanged(int index)
{
  calendar->setLocale(localeCombo->itemData(index).toLocale());
}

void Window::firstDayChanged(int index)
{
  calendar->setFirstDayOfWeek(
    Qt::DayOfWeek(firstDayCombo->itemData(index).toInt()));
}

void Window::selectionModeChanged(int index)
{
  calendar->setSelectionMode(QCalendarWidget::SelectionMode(
    selectionModeCombo->itemData(index).toInt()));
}

void Window::horizontalHeaderChanged(int index)
{
  calendar->setHorizontalHeaderFormat(QCalendarWidget::HorizontalHeaderFormat(
    horizontalHeaderCombo->itemData(index).toInt()));
}

void Window::verticalHeaderChanged(int index)
{
  calendar->setVerticalHeaderFormat(QCalendarWidget::VerticalHeaderFormat(
    verticalHeaderCombo->itemData(index).toInt()));
}

void Window::selectedDateChanged()
{
  currentDateEdit->setDate(calendar->selectedDate());
}

void Window::minimumDateChanged(const QDate& date)
{
  calendar->setMinimumDate(date);
  maximumDateEdit->setDate(calendar->maximumDate());
}

void Window::maximumDateChanged(const QDate& date)
{
  calendar->setMaximumDate(date);
  minimumDateEdit->setDate(calendar->minimumDate());
}

void Window::weekdayFormatChanged()
{
  QTextCharFormat format;

  format.setForeground(qvariant_cast<QColor>(
    weekdayColorCombo->itemData(weekdayColorCombo->currentIndex())));
  calendar->setWeekdayTextFormat(Qt::Monday, format);
  calendar->setWeekdayTextFormat(Qt::Tuesday, format);
  calendar->setWeekdayTextFormat(Qt::Wednesday, format);
  calendar->setWeekdayTextFormat(Qt::Thursday, format);
  calendar->setWeekdayTextFormat(Qt::Friday, format);
}

void Window::weekendFormatChanged()
{
  QTextCharFormat format;

  format.setForeground(qvariant_cast<QColor>(
    weekendColorCombo->itemData(weekendColorCombo->currentIndex())));
  calendar->setWeekdayTextFormat(Qt::Saturday, format);
  calendar->setWeekdayTextFormat(Qt::Sunday, format);
}

void Window::reformatHeaders()
{
  QString text = headerTextFormatCombo->currentText();
  QTextCharFormat format;

  if (text == tr("Bold")) {
    format.setFontWeight(QFont::Bold);
  } else if (text == tr("Italic")) {
    format.setFontItalic(true);
  } else if (text == tr("Green")) {
    format.setForeground(Qt::green);
  }
  calendar->setHeaderTextFormat(format);
}

void Window::reformatCalendarPage()
{
  if (firstFridayCheckBox->isChecked()) {
    QDate firstFriday(calendar->yearShown(), calendar->monthShown(), 1);
    while (firstFriday.dayOfWeek() != Qt::Friday)
      firstFriday = firstFriday.addDays(1);
    QTextCharFormat firstFridayFormat;
    firstFridayFormat.setForeground(Qt::blue);
    calendar->setDateTextFormat(firstFriday, firstFridayFormat);
  }

  // May First in Red takes precedence
  if (mayFirstCheckBox->isChecked()) {
    const QDate mayFirst(calendar->yearShown(), 5, 1);
    QTextCharFormat mayFirstFormat;
    mayFirstFormat.setForeground(Qt::red);
    calendar->setDateTextFormat(mayFirst, mayFirstFormat);
  }
}

void Window::createPreviewGroupBox()
{
  previewGroupBox = new QGroupBox(tr("Preview"));

  calendar = new QCalendarWidget;
  calendar->setMinimumDate(QDate(1900, 1, 1));
  calendar->setMaximumDate(QDate(3000, 1, 1));
  calendar->setGridVisible(true);

  connect(calendar, SIGNAL(currentPageChanged(int, int)), this,
          SLOT(reformatCalendarPage()));

  previewLayout = new QGridLayout;
  previewLayout->addWidget(calendar, 0, 0, Qt::AlignCenter);
  previewGroupBox->setLayout(previewLayout);
}

void Window::createGeneralOptionsGroupBox()
{
  generalOptionsGroupBox = new QGroupBox(tr("General Options"));

  localeCombo = new QComboBox;
  int curLocaleIndex = -1;
  int index = 0;
  for (int _lang = QLocale::C; _lang <= QLocale::LastLanguage; ++_lang) {
    QLocale::Language lang = static_cast<QLocale::Language>(_lang);
    QList<QLocale::Country> countries = QLocale::countriesForLanguage(lang);
    for (int i = 0; i < countries.count(); ++i) {
      QLocale::Country country = countries.at(i);
      QString label = QLocale::languageToString(lang);
      label += QLatin1Char('/');
      label += QLocale::countryToString(country);
      QLocale locale(lang, country);
      if (this->locale().language() == lang &&
          this->locale().country() == country)
        curLocaleIndex = index;
      localeCombo->addItem(label, locale);
      ++index;
    }
  }
  if (curLocaleIndex != -1)
    localeCombo->setCurrentIndex(curLocaleIndex);
  localeLabel = new QLabel(tr("&Locale"));
  localeLabel->setBuddy(localeCombo);

  firstDayCombo = new QComboBox;
  firstDayCombo->addItem(tr("Sunday"), Qt::Sunday);
  firstDayCombo->addItem(tr("Monday"), Qt::Monday);
  firstDayCombo->addItem(tr("Tuesday"), Qt::Tuesday);
  firstDayCombo->addItem(tr("Wednesday"), Qt::Wednesday);
  firstDayCombo->addItem(tr("Thursday"), Qt::Thursday);
  firstDayCombo->addItem(tr("Friday"), Qt::Friday);
  firstDayCombo->addItem(tr("Saturday"), Qt::Saturday);

  firstDayLabel = new QLabel(tr("Wee&k starts on:"));
  firstDayLabel->setBuddy(firstDayCombo);

  selectionModeCombo = new QComboBox;
  selectionModeCombo->addItem(tr("Single selection"),
                              QCalendarWidget::SingleSelection);
  selectionModeCombo->addItem(tr("None"), QCalendarWidget::NoSelection);

  selectionModeLabel = new QLabel(tr("&Selection mode:"));
  selectionModeLabel->setBuddy(selectionModeCombo);

  gridCheckBox = new QCheckBox(tr("&Grid"));
  gridCheckBox->setChecked(calendar->isGridVisible());

  navigationCheckBox = new QCheckBox(tr("&Navigation bar"));
  navigationCheckBox->setChecked(true);

  horizontalHeaderCombo = new QComboBox;
  horizontalHeaderCombo->addItem(tr("Single letter day names"),
                                 QCalendarWidget::SingleLetterDayNames);
  horizontalHeaderCombo->addItem(tr("Short day names"),
                                 QCalendarWidget::ShortDayNames);
  horizontalHeaderCombo->addItem(tr("None"),
                                 QCalendarWidget::NoHorizontalHeader);
  horizontalHeaderCombo->setCurrentIndex(1);

  horizontalHeaderLabel = new QLabel(tr("&Horizontal header:"));
  horizontalHeaderLabel->setBuddy(horizontalHeaderCombo);

  verticalHeaderCombo = new QComboBox;
  verticalHeaderCombo->addItem(tr("ISO week numbers"),
                               QCalendarWidget::ISOWeekNumbers);
  verticalHeaderCombo->addItem(tr("None"), QCalendarWidget::NoVerticalHeader);

  verticalHeaderLabel = new QLabel(tr("&Vertical header:"));
  verticalHeaderLabel->setBuddy(verticalHeaderCombo);

  connect(localeCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(localeChanged(int)));
  connect(firstDayCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(firstDayChanged(int)));
  connect(selectionModeCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(selectionModeChanged(int)));
  connect(gridCheckBox, SIGNAL(toggled(bool)), calendar,
          SLOT(setGridVisible(bool)));
  connect(navigationCheckBox, SIGNAL(toggled(bool)), calendar,
          SLOT(setNavigationBarVisible(bool)));
  connect(horizontalHeaderCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(horizontalHeaderChanged(int)));
  connect(verticalHeaderCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(verticalHeaderChanged(int)));

  QHBoxLayout* checkBoxLayout = new QHBoxLayout;
  checkBoxLayout->addWidget(gridCheckBox);
  checkBoxLayout->addStretch();
  checkBoxLayout->addWidget(navigationCheckBox);

  QGridLayout* outerLayout = new QGridLayout;
  outerLayout->addWidget(localeLabel, 0, 0);
  outerLayout->addWidget(localeCombo, 0, 1);
  outerLayout->addWidget(firstDayLabel, 1, 0);
  outerLayout->addWidget(firstDayCombo, 1, 1);
  outerLayout->addWidget(selectionModeLabel, 2, 0);
  outerLayout->addWidget(selectionModeCombo, 2, 1);
  outerLayout->addLayout(checkBoxLayout, 3, 0, 1, 2);
  outerLayout->addWidget(horizontalHeaderLabel, 4, 0);
  outerLayout->addWidget(horizontalHeaderCombo, 4, 1);
  outerLayout->addWidget(verticalHeaderLabel, 5, 0);
  outerLayout->addWidget(verticalHeaderCombo, 5, 1);
  generalOptionsGroupBox->setLayout(outerLayout);

  firstDayChanged(firstDayCombo->currentIndex());
  selectionModeChanged(selectionModeCombo->currentIndex());
  horizontalHeaderChanged(horizontalHeaderCombo->currentIndex());
  verticalHeaderChanged(verticalHeaderCombo->currentIndex());
}

void Window::createDatesGroupBox()
{
  datesGroupBox = new QGroupBox(tr("Dates"));

  minimumDateEdit = new QDateEdit;
  minimumDateEdit->setDisplayFormat("MMM d yyyy");
  minimumDateEdit->setDateRange(calendar->minimumDate(),
                                calendar->maximumDate());
  minimumDateEdit->setDate(calendar->minimumDate());

  minimumDateLabel = new QLabel(tr("&Minimum Date:"));
  minimumDateLabel->setBuddy(minimumDateEdit);

  currentDateEdit = new QDateEdit;
  currentDateEdit->setDisplayFormat("MMM d yyyy");
  currentDateEdit->setDate(calendar->selectedDate());
  currentDateEdit->setDateRange(calendar->minimumDate(),
                                calendar->maximumDate());

  currentDateLabel = new QLabel(tr("&Current Date:"));
  currentDateLabel->setBuddy(currentDateEdit);

  maximumDateEdit = new QDateEdit;
  maximumDateEdit->setDisplayFormat("MMM d yyyy");
  maximumDateEdit->setDateRange(calendar->minimumDate(),
                                calendar->maximumDate());
  maximumDateEdit->setDate(calendar->maximumDate());

  maximumDateLabel = new QLabel(tr("Ma&ximum Date:"));
  maximumDateLabel->setBuddy(maximumDateEdit);

  connect(currentDateEdit, SIGNAL(dateChanged(QDate)), calendar,
          SLOT(setSelectedDate(QDate)));
  connect(calendar, SIGNAL(selectionChanged()), this,
          SLOT(selectedDateChanged()));
  connect(minimumDateEdit, SIGNAL(dateChanged(QDate)), this,
          SLOT(minimumDateChanged(QDate)));
  connect(maximumDateEdit, SIGNAL(dateChanged(QDate)), this,
          SLOT(maximumDateChanged(QDate)));

  QGridLayout* dateBoxLayout = new QGridLayout;
  dateBoxLayout->addWidget(currentDateLabel, 1, 0);
  dateBoxLayout->addWidget(currentDateEdit, 1, 1);
  dateBoxLayout->addWidget(minimumDateLabel, 0, 0);
  dateBoxLayout->addWidget(minimumDateEdit, 0, 1);
  dateBoxLayout->addWidget(maximumDateLabel, 2, 0);
  dateBoxLayout->addWidget(maximumDateEdit, 2, 1);
  dateBoxLayout->setRowStretch(3, 1);

  datesGroupBox->setLayout(dateBoxLayout);
}

void Window::createTextFormatsGroupBox()
{
  textFormatsGroupBox = new QGroupBox(tr("Text Formats"));

  weekdayColorCombo = createColorComboBox();
  weekdayColorCombo->setCurrentIndex(weekdayColorCombo->findText(tr("Black")));

  weekdayColorLabel = new QLabel(tr("&Weekday color:"));
  weekdayColorLabel->setBuddy(weekdayColorCombo);

  weekendColorCombo = createColorComboBox();
  weekendColorCombo->setCurrentIndex(weekendColorCombo->findText(tr("Red")));

  weekendColorLabel = new QLabel(tr("Week&end color:"));
  weekendColorLabel->setBuddy(weekendColorCombo);

  headerTextFormatCombo = new QComboBox;
  headerTextFormatCombo->addItem(tr("Bold"));
  headerTextFormatCombo->addItem(tr("Italic"));
  headerTextFormatCombo->addItem(tr("Plain"));

  headerTextFormatLabel = new QLabel(tr("&Header text:"));
  headerTextFormatLabel->setBuddy(headerTextFormatCombo);

  firstFridayCheckBox = new QCheckBox(tr("&First Friday in blue"));

  mayFirstCheckBox = new QCheckBox(tr("May &1 in red"));

  connect(weekdayColorCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(weekdayFormatChanged()));
  connect(weekendColorCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(weekendFormatChanged()));
  connect(headerTextFormatCombo, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(reformatHeaders()));
  connect(firstFridayCheckBox, SIGNAL(toggled(bool)), this,
          SLOT(reformatCalendarPage()));
  connect(mayFirstCheckBox, SIGNAL(toggled(bool)), this,
          SLOT(reformatCalendarPage()));

  QHBoxLayout* checkBoxLayout = new QHBoxLayout;
  checkBoxLayout->addWidget(firstFridayCheckBox);
  checkBoxLayout->addStretch();
  checkBoxLayout->addWidget(mayFirstCheckBox);

  QGridLayout* outerLayout = new QGridLayout;
  outerLayout->addWidget(weekdayColorLabel, 0, 0);
  outerLayout->addWidget(weekdayColorCombo, 0, 1);
  outerLayout->addWidget(weekendColorLabel, 1, 0);
  outerLayout->addWidget(weekendColorCombo, 1, 1);
  outerLayout->addWidget(headerTextFormatLabel, 2, 0);
  outerLayout->addWidget(headerTextFormatCombo, 2, 1);
  outerLayout->addLayout(checkBoxLayout, 3, 0, 1, 2);
  textFormatsGroupBox->setLayout(outerLayout);

  weekdayFormatChanged();
  weekendFormatChanged();
  reformatHeaders();
  reformatCalendarPage();
}

QComboBox* Window::createColorComboBox()
{
  QComboBox* comboBox = new QComboBox;
  comboBox->addItem(tr("Red"), QColor(Qt::red));
  comboBox->addItem(tr("Blue"), QColor(Qt::blue));
  comboBox->addItem(tr("Black"), QColor(Qt::black));
  comboBox->addItem(tr("Magenta"), QColor(Qt::magenta));
  return comboBox;
}

//#include "moc_calwidget.cpp"
