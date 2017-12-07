/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "RegexExplorer.h"

RegexExplorer::RegexExplorer(QWidget* p)
  : QDialog(p)
  , m_matched(false)
{
  this->setupUi(this);

  for (int i = 1; i < cmsys::RegularExpression::NSUBEXP; ++i) {
    matchNumber->addItem(QString("Match %1").arg(QString::number(i)),
                         QVariant(i));
  }
  matchNumber->setCurrentIndex(0);
}

void RegexExplorer::setStatusColor(QWidget* widget, bool successful)
{
  QColor color = successful ? QColor(0, 127, 0) : Qt::red;

  QPalette palette = widget->palette();
  palette.setColor(QPalette::Foreground, color);
  widget->setPalette(palette);
}

void RegexExplorer::on_regularExpression_textChanged(const QString& text)
{
#ifdef QT_NO_STL
  m_regex = text.toAscii().constData();
#else
  m_regex = text.toStdString();
#endif

  bool validExpression =
    stripEscapes(m_regex) && m_regexParser.compile(m_regex);
  if (!validExpression) {
    m_regexParser.set_invalid();
  }

  setStatusColor(labelRegexValid, validExpression);

  on_inputText_textChanged();
}

void RegexExplorer::on_inputText_textChanged()
{
  if (m_regexParser.is_valid()) {
    QString plainText = inputText->toPlainText();
#ifdef QT_NO_STL
    m_text = plainText.toAscii().constData();
#else
    m_text = plainText.toStdString();
#endif
    m_matched = m_regexParser.find(m_text);
  } else {
    m_matched = false;
  }

  setStatusColor(labelRegexMatch, m_matched);

  if (!m_matched) {
    clearMatch();
    return;
  }

  std::string matchingText;

  if (matchAll->isChecked()) {
    const char* p = m_text.c_str();
    while (m_regexParser.find(p)) {
      std::string::size_type l = m_regexParser.start();
      std::string::size_type r = m_regexParser.end();
      if (r - l == 0) {
        // matched empty string
        clearMatch();
        return;
      }
      if (!matchingText.empty()) {
        matchingText += ";";
      }
      matchingText += std::string(p + l, r - l);
      p += r;
    }
  } else {
    matchingText = m_regexParser.match(0);
  }

#ifdef QT_NO_STL
  QString matchText = matchingText.c_str();
#else
  QString matchText = QString::fromStdString(matchingText);
#endif
  match0->setPlainText(matchText);

  on_matchNumber_currentIndexChanged(matchNumber->currentIndex());
}

void RegexExplorer::on_matchNumber_currentIndexChanged(int index)
{
  if (!m_matched) {
    return;
  }

  QVariant itemData = matchNumber->itemData(index);
  int idx = itemData.toInt();

  if (idx < 1 || idx >= cmsys::RegularExpression::NSUBEXP) {
    return;
  }

#ifdef QT_NO_STL
  QString match = m_regexParser.match(idx).c_str();
#else
  QString match = QString::fromStdString(m_regexParser.match(idx));
#endif
  matchN->setPlainText(match);
}

void RegexExplorer::on_matchAll_toggled(bool checked)
{
  Q_UNUSED(checked);

  on_inputText_textChanged();
}

void RegexExplorer::clearMatch()
{
  m_matched = false;
  match0->clear();
  matchN->clear();
}

bool RegexExplorer::stripEscapes(std::string& source)
{
  const char* in = source.c_str();

  std::string result;
  result.reserve(source.size());

  for (char inc = *in; inc != '\0'; inc = *++in) {
    if (inc == '\\') {
      char nextc = in[1];
      if (nextc == 't') {
        result.append(1, '\t');
        in++;
      } else if (nextc == 'n') {
        result.append(1, '\n');
        in++;
      } else if (nextc == 't') {
        result.append(1, '\t');
        in++;
      } else if (isalnum(nextc) || nextc == '\0') {
        return false;
      } else {
        result.append(1, nextc);
        in++;
      }
    } else {
      result.append(1, inc);
    }
  }

  source = result;
  return true;
}
