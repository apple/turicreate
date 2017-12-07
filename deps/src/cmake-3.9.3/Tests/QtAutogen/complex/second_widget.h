
#include <QWidget>

namespace Ui {
class SecondWidget;
}

class SecondWidget : public QWidget
{
  Q_OBJECT
public:
  explicit SecondWidget(QWidget* parent = 0);

  ~SecondWidget();

private:
  Ui::SecondWidget* ui;
};
