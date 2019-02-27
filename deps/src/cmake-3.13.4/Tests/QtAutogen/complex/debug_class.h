
#include <QWidget>

namespace Ui {
class DebugClass;
}

class DebugClass : public QWidget
{
  Q_OBJECT
public:
  explicit DebugClass(QWidget* parent = 0);

signals:
  void someSignal();

private:
  Ui::DebugClass* ui;
};
