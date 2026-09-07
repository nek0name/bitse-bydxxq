#include "AdminUi.h"
#include "Appearance.h"
#include "DataTable.h"

#include <QAbstractAxis>
#include <QApplication>
#include <QChart>
#include <QChartView>
#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLegend>
#include <QMap>
#include <QPalette>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QXYSeries>

namespace adminui {
namespace {
QString arrowSource(const QString &name, const QColor &color) {
  static QTemporaryDir directory;
  const auto path = directory.filePath(name + color.name().mid(1) + ".svg");
  QFile file(path);
  if (file.open(QIODevice::WriteOnly)) {
    const auto pathData = name == "down" ? "m3 4.5 3 3 3-3" : "m3 7.5 3-3 3 3";
    file.write(
      QString("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"12\" "
              "height=\"12\" viewBox=\"0 0 12 12\"><path d=\"%1\" "
              "fill=\"none\" stroke=\"%2\" stroke-width=\"1.5\" "
              "stroke-linecap=\"round\" stroke-linejoin=\"round\"/></svg>")
        .arg(pathData, color.name())
        .toUtf8());
  }
  return path;
}

void updateTheme() {
  const auto colors = Appearance::instance()->colors();
  const auto color = [&colors](const char *key) {
    return colors.value(key).value<QColor>();
  };
  QPalette palette;
  palette.setColor(QPalette::Window, color("paper"));
  palette.setColor(QPalette::WindowText, color("ink"));
  palette.setColor(QPalette::Base, color("card"));
  palette.setColor(QPalette::AlternateBase, color("paper"));
  palette.setColor(QPalette::Text, color("ink"));
  palette.setColor(QPalette::Button, color("card"));
  palette.setColor(QPalette::ButtonText, color("ink"));
  palette.setColor(QPalette::Highlight, color("secondary"));
  palette.setColor(QPalette::HighlightedText, color("ink"));
  palette.setColor(QPalette::Link, color("primaryText"));
  palette.setColor(QPalette::Light, color("card"));
  palette.setColor(QPalette::Midlight, color("primaryLight"));
  palette.setColor(QPalette::Mid, color("border"));
  palette.setColor(QPalette::Dark, color("muted"));
  palette.setColor(QPalette::Shadow, color("surfaceDark"));
  palette.setColor(QPalette::ToolTipBase, color("surfaceDark"));
  palette.setColor(QPalette::ToolTipText, color("onPrimary"));
  palette.setColor(QPalette::PlaceholderText, color("muted"));
  palette.setColor(QPalette::Disabled, QPalette::Text, color("disabledText"));
  palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                   color("disabledText"));
  palette.setColor(QPalette::Disabled, QPalette::Base, color("disabled"));
  palette.setColor(QPalette::Disabled, QPalette::Highlight, color("disabled"));
  QApplication::setPalette(palette);
  auto stylesheet = QStringLiteral(R"(
    QWidget { color: @ink@; }
    QMainWindow, QDialog, QStackedWidget { background: @paper@; }
    QLabel { background: transparent; }
    QWidget#loginPage { background: @primaryLight@; }
    QGroupBox {
      background: @card@; border: 0; border-radius: 10px;
      margin-top: 0; padding: 28px 12px 12px;
    }
    QGroupBox::title {
      subcontrol-origin: border; subcontrol-position: top left;
      top: 10px; left: 14px; padding: 0 6px;
      color: @muted@; background: transparent;
    }
    QGroupBox#loginCard { padding: 36px 24px 24px; }
    QWidget#sidebar { background: @card@; }
    QWidget#sidebar QLabel { color: @muted@; }
    QWidget#sidebar QLabel#brandHeading { color: @ink@; padding: 12px; }
    QWidget#sidebar QLabel#adminIdentity { padding: 0 12px 8px; }
    QWidget#sidebar QPushButton {
      background: transparent; color: @muted@; border: 0; border-radius: 6px;
      padding: 4px 12px; text-align: left;
    }
    QWidget#sidebar QPushButton:hover { background: @paper@; color: @ink@; }
    QWidget#sidebar QPushButton:checked { background: @secondaryLight@; color: @ink@; }
    QWidget#sidebar QPushButton:pressed { background: @primarySoftPressed@; color: @primaryText@; }
    QLabel#syncTime { color: @muted@; }
    QLabel#stationError, QLabel#adminLoginError { color: @danger@; }
    QChartView { background: @card@; border: 0; border-radius: 0; }
    QPushButton, QToolButton {
      background: transparent; color: @primaryText@; border: 0;
      border-radius: 4px; padding: 0 4px; min-height: 26px;
    }
    QPushButton:hover, QToolButton:hover { background: @primaryLight@; color: @primaryText@; }
    QPushButton:pressed, QToolButton:pressed { background: @primarySoftPressed@; }
    QPushButton:focus, QToolButton:focus { background: @primaryLight@; }
    QPushButton#adminLoginButton, QPushButton#addStationButton,
    QPushButton#saveStationButton, QPushButton#runForecastButton {
      background: @primary@; color: @onPrimary@; padding: 4px 14px;
    }
    QPushButton#adminLoginButton:hover, QPushButton#addStationButton:hover,
    QPushButton#saveStationButton:hover, QPushButton#runForecastButton:hover,
    QPushButton#adminLoginButton:pressed, QPushButton#addStationButton:pressed,
    QPushButton#saveStationButton:pressed, QPushButton#runForecastButton:pressed,
    QPushButton#adminLoginButton:focus, QPushButton#addStationButton:focus,
    QPushButton#saveStationButton:focus, QPushButton#runForecastButton:focus {
      background: @primaryPressed@;
    }
    QPushButton#markChargerFaultButton, QPushButton#freezeUserButton {
      color: @danger@; background: transparent;
    }
    QPushButton#markChargerFaultButton:hover, QPushButton#freezeUserButton:hover {
      background: @dangerLight@;
    }
    QPushButton:disabled, QToolButton:disabled,
    QPushButton#adminLoginButton:disabled, QPushButton#addStationButton:disabled,
    QPushButton#saveStationButton:disabled, QPushButton#runForecastButton:disabled,
    QPushButton#markChargerFaultButton:disabled, QPushButton#freezeUserButton:disabled {
      color: @disabledText@; background: transparent;
    }
    QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
      background: @paper@; color: @ink@; border: 0;
      border-radius: 5px; padding: 5px 8px; min-height: 20px;
      selection-background-color: @secondaryLight@; selection-color: @ink@;
    }
    QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
      background: @primaryLight@;
    }
    QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
      color: @disabledText@; background: @paper@;
    }
    QComboBox::drop-down { border: 0; width: 24px; }
    QComboBox::down-arrow, QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
      width: 12px; height: 12px; image: url("@downArrow@");
    }
    QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
      width: 12px; height: 12px; image: url("@upArrow@");
    }
    QSpinBox::up-button, QDoubleSpinBox::up-button {
      subcontrol-origin: border; subcontrol-position: top right;
      background: transparent; width: 22px; border: 0;
    }
    QSpinBox::down-button, QDoubleSpinBox::down-button {
      subcontrol-origin: border; subcontrol-position: bottom right;
      background: transparent; width: 22px; border: 0;
    }
    QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
    QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover { background: @primarySoftPressed@; }
    QAbstractItemView {
      background: @card@; alternate-background-color: @paper@;
      border: 0; border-radius: 0;
      selection-background-color: @secondaryLight@; selection-color: @ink@;
      gridline-color: @border@; outline: 0;
    }
    QAbstractItemView::item { padding: 4px; }
    QTableView::item { border: 0; border-bottom: 1px solid @border@; }
    QLabel#dataTableCount, QLabel#dataTableEmptyLabel { color: @muted@; }
    QWidget#dataTableEmpty { background: transparent; }
    QToolButton#dataTableMore { padding: 0; color: @muted@; }
    QToolButton#dataTableMore::menu-indicator { image: none; width: 0; }
    QAbstractItemView::item:selected { background: @secondaryLight@; color: @ink@; }
    QHeaderView { background: @paper@; }
    QHeaderView::section {
      background: @paper@; color: @muted@; padding: 7px 8px;
      border: 0; border-bottom: 1px solid @border@;
    }
    QHeaderView::section:hover { background: @primaryLight@; }
    QTableCornerButton::section { background: @paper@; border: 0; }
    QCheckBox { color: @muted@; spacing: 7px; background: transparent; }
    QCheckBox::indicator { width: 15px; height: 15px; }
    QMenu { background: @card@; color: @ink@; border: 1px solid @border@; padding: 6px; }
    QMenu::item { padding: 6px 24px; }
    QMenu::item:selected { background: @primaryLight@; color: @primaryText@; }
    QMenu::item:disabled { color: @disabledText@; }
    QMenu::separator { height: 1px; background: @border@; margin: 5px; }
    QWidget#tableFilter { background: @card@; }
    QMenu#columnFilterPopup { padding: 0; }
    QLabel#columnFilterTitle { font-weight: 500; }
    QWidget#tableFilter QLineEdit, QWidget#tableFilter QComboBox {
      padding: 4px 7px; min-height: 20px;
    }
    QWidget#tableFilter QToolButton { padding: 0; }
    QWidget#tableFilter QToolButton:checked { background: @primarySoftPressed@; }
    QPushButton#applyColumnFilter { background: @primary@; color: @onPrimary@; padding: 1px 14px; }
    QPushButton#applyColumnFilter:hover, QPushButton#applyColumnFilter:focus { background: @primaryPressed@; }
    QLabel#forecastMeta, QLabel#forecastWarnings { color: @muted@; }

    QWidget#appearanceModes QToolButton { background: @paper@; color: @muted@; padding: 5px 14px; }
    QWidget#appearanceModes QToolButton:checked { background: @secondaryLight@; color: @ink@; }
    QTabWidget::pane { border: 0; background: @card@; }
    QTabBar::tab {
      background: transparent; color: @muted@; padding: 7px 14px;
      border: 0; border-bottom: 2px solid transparent;
    }
    QTabBar::tab:selected { color: @ink@; border-bottom-color: @secondary@; }
    QTabBar::tab:hover { color: @primaryText@; }
    QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }
    QScrollBar:horizontal { background: transparent; height: 8px; margin: 0; }
    QScrollBar::handle { background: @border@; border-radius: 4px; min-width: 24px; min-height: 24px; }
    QScrollBar::handle:hover { background: @disabledText@; }
    QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
    QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
    QSplitter::handle { background: @paper@; }
    QStatusBar { background: transparent; color: @muted@; }
    QStatusBar::item { border: 0; }
    QToolTip { background: @surfaceDark@; color: @onPrimary@; border: 0; padding: 6px; }
  )");
  for (auto it = colors.cbegin(); it != colors.cend(); ++it)
    stylesheet.replace("@" + it.key() + "@", it.value().value<QColor>().name());
  stylesheet.replace("@downArrow@", arrowSource("down", color("muted")));
  stylesheet.replace("@upArrow@", arrowSource("up", color("muted")));
  qApp->setStyleSheet(stylesheet);
  for (auto *widget : QApplication::allWidgets())
    if (auto *view = qobject_cast<QChartView *>(widget))
      styleChart(view->chart());
}
} // namespace

void applyTheme() {
  if (!qApp->property("adminThemeInstalled").toBool()) {
    QApplication::setStyle("Fusion");
    QObject::connect(Appearance::instance(), &Appearance::changed, qApp,
                     updateTheme);
    qApp->setProperty("adminThemeInstalled", true);
  }
  updateTheme();
}

void styleChart(QChart *chart) {
  const auto colors = Appearance::instance()->colors();
  const auto color = [&colors](const char *key) {
    return colors.value(key).value<QColor>();
  };
  chart->setBackgroundBrush(color("card"));
  chart->setBackgroundPen(Qt::NoPen);
  chart->setBackgroundRoundness(10);
  chart->setDropShadowEnabled(false);
  chart->setPlotAreaBackgroundBrush(color("card"));
  chart->setPlotAreaBackgroundVisible(true);
  chart->setTitleBrush(color("ink"));
  chart->legend()->setLabelColor(color("muted"));
  for (auto *axis : chart->axes()) {
    axis->setLabelsColor(color("muted"));
    axis->setTitleBrush(color("ink"));
    axis->setLinePen(QPen(color("border")));
    axis->setGridLinePen(QPen(color("border")));
    axis->setMinorGridLinePen(QPen(color("paper")));
  }
  for (auto *series : chart->series()) {
    if (auto *line = qobject_cast<QXYSeries *>(series)) {
      line->setPen(QPen(color("secondary"), 2.5));
      line->setBrush(color("accent"));
    }
  }
}

QString money(const QJsonValue &value) {
  return QStringLiteral("¥ %1").arg(value.toDouble() / 100.0, 0, 'f', 2);
}

QString number(const QJsonValue &value, int decimals) {
  return value.isDouble() ? QString::number(value.toDouble(), 'f', decimals)
                          : QStringLiteral("—");
}

QVariant numberCell(const QJsonValue &value, int decimals) {
  return cell(number(value, decimals),
              value.isDouble() ? QVariant(value.toDouble()) : QVariant());
}

QVariant moneyCell(const QJsonValue &value) {
  return cell(money(value), value.toDouble() / 100.0);
}

QString timeText(const QJsonValue &value) {
  const auto time = QDateTime::fromString(value.toString(), Qt::ISODate);
  return time.isValid() ? time.toLocalTime().toString("yyyy-MM-dd HH:mm:ss")
                        : QStringLiteral("—");
}

QString duration(const QJsonValue &value) {
  const auto seconds = value.toInteger();
  return QStringLiteral("%1小时 %2分 %3秒")
    .arg(seconds / 3600)
    .arg(seconds / 60 % 60)
    .arg(seconds % 60);
}

QString state(const QString &value) {
  static const QMap<QString, QString> labels{
    {"idle", "闲置"},   {"reserved", "已预约"},  {"charging", "充电中"},
    {"fault", "故障"},  {"offline", "离线"},     {"restarting", "重启中"},
    {"active", "正常"}, {"frozen", "冻结"},      {"pending_payment", "待结算"},
    {"paid", "已支付"}, {"cancelled", "已取消"}, {"dc", "直流快充"},
    {"ac", "交流慢充"}};
  return labels.value(value, value);
}

QLabel *heading(const QString &text) {
  auto *label = new QLabel(text);
  auto font = label->font();
  font.setPointSize(font.pointSize() + 3);
  font.setBold(true);
  label->setFont(font);
  return label;
}

QJsonObject selected(QTableWidget *tableWidget) {
  const auto row = tableWidget->currentRow();
  if (row < 0 || tableWidget->isRowHidden(row) || !tableWidget->item(row, 0))
    return {};
  const auto selection = tableWidget->selectedItems();
  if (selection.isEmpty()) return {};
  for (const auto *item : selection)
    if (item->row() != row) return {};
  return tableWidget->item(row, 0)->data(Qt::UserRole).value<QJsonObject>();
}

QStringList chargerHeaders() {
  return {"电桩 ID",      "编号",         "所属电站",
          "类型",         "功率 (kW)",    "状态",
          "累计充电次数", "累计充电时长", "累计电量 (kWh)"};
}

QVariantList chargerColumns(const QJsonObject &o) {
  return {o["id"].toInt(),
          o["code"].toString(),
          o["stationName"].toString(),
          state(o["type"].toString()),
          o["powerKw"].toDouble(),
          state(o["status"].toString()),
          o["chargingCount"].toInt(),
          cell(duration(o["chargingSeconds"]), o["chargingSeconds"].toDouble()),
          numberCell(o["energyKwh"], 2)};
}

QStringList orderHeaders() {
  return {"订单 ID",  "订单号", "用户 ID",    "电站",
          "电桩",     "状态",   "创建时间",   "开始时间",
          "结束时间", "时长",   "电量 (kWh)", "金额 (元)"};
}

QVariantList orderColumns(const QJsonObject &o) {
  return {o["id"].toInt(),
          o["orderNo"].toString(),
          o["userId"].toInt(),
          o["stationName"].toString(),
          o["chargerCode"].toString(),
          state(o["status"].toString()),
          timeText(o["createdAt"]),
          timeText(o["startedAt"]),
          timeText(o["endedAt"]),
          cell(duration(o["durationSeconds"]), o["durationSeconds"].toDouble()),
          numberCell(o["energyKwh"], 3),
          moneyCell(o["amountCents"])};
}

QPushButton *button(const QString &text, QHBoxLayout *row) {
  auto *result = new QPushButton(text);
  row->addWidget(result);
  return result;
}

} // namespace adminui
