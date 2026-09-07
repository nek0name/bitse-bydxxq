#include "AdminUi.h"
#include "Appearance.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

namespace adminui {
namespace {
class ColorSwatch final : public QAbstractButton {
public:
  ColorSwatch(const QColor &color, QWidget *parent)
      : QAbstractButton(parent), color(color) {
    setCheckable(true);
    setFixedSize(36, 36);
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(color.name());
    setToolTip(color.name());
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto center = QRectF(rect()).center();
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(center, 11, 11);
    if (isChecked() || hasFocus()) {
      painter.setPen(QPen(palette().color(QPalette::WindowText), 1));
      painter.setBrush(Qt::NoBrush);
      painter.drawEllipse(center, 15, 15);
    }
  }

private:
  QColor color;
};
} // namespace

void showSettings(QWidget *parent) {
  auto *appearance = Appearance::instance();
  auto *dialog = new QDialog(parent);
  dialog->setObjectName("adminSettingsDialog");
  dialog->setWindowTitle("设置");
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  auto *layout = new QVBoxLayout(dialog);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(16);
  layout->addWidget(heading("外观"));
  auto *modes = new QWidget;
  modes->setObjectName("appearanceModes");
  auto *modeLayout = new QHBoxLayout(modes);
  modeLayout->setContentsMargins(0, 0, 0, 0);
  modeLayout->setSpacing(4);
  auto *modeGroup = new QButtonGroup(dialog);
  for (const auto &entry : {qMakePair(QString("system"), QString("跟随系统")),
                            qMakePair(QString("light"), QString("浅色")),
                            qMakePair(QString("dark"), QString("深色"))}) {
    auto *button = new QToolButton;
    button->setObjectName("appearanceMode" + entry.first.left(1).toUpper()
                          + entry.first.mid(1));
    button->setText(entry.second);
    button->setCheckable(true);
    button->setProperty("mode", entry.first);
    modeGroup->addButton(button);
    modeLayout->addWidget(button, 1);
    QObject::connect(button, &QToolButton::clicked, dialog,
                     [appearance, mode = entry.first] {
                       appearance->setMode(mode);
                     });
  }
  layout->addWidget(modes);
  auto addSwatches = [appearance, dialog, layout](const QString &name,
                                                  const QString &label,
                                                  bool primary) {
    layout->addWidget(new QLabel(label));
    auto *row = new QWidget;
    row->setObjectName(name + "Swatches");
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);
    auto *group = new QButtonGroup(dialog);
    for (const auto &value : appearance->swatches()) {
      const QColor color(value);
      auto *button = new ColorSwatch(color, row);
      button->setObjectName(name + "Swatch" + color.name().mid(1));
      button->setProperty("swatch", color);
      group->addButton(button);
      rowLayout->addWidget(button);
      QObject::connect(button, &QAbstractButton::clicked, dialog,
                       [appearance, primary, color] {
                         if (primary)
                           appearance->setPrimaryColor(color);
                         else
                           appearance->setSecondaryColor(color);
                       });
    }
    rowLayout->addStretch();
    layout->addWidget(row);
    return group;
  };
  auto *primary = addSwatches("primary", "主色", true);
  auto *secondary = addSwatches("secondary", "副色", false);
  auto update = [appearance, modeGroup, primary, secondary] {
    for (auto *button : modeGroup->buttons())
      button->setChecked(button->property("mode").toString()
                         == appearance->mode());
    for (auto *button : primary->buttons())
      button->setChecked(button->property("swatch").value<QColor>()
                         == appearance->primaryColor());
    for (auto *button : secondary->buttons())
      button->setChecked(button->property("swatch").value<QColor>()
                         == appearance->secondaryColor());
  };
  QObject::connect(appearance, &Appearance::changed, dialog, update);
  update();
  dialog->open();
}
} // namespace adminui
