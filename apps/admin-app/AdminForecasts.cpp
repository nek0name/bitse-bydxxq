#include "AdminUi.h"
#include "AdminWindowState.h"

#include <QComboBox>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

using namespace adminui;

namespace {
constexpr auto runningMessage = "正在生成预测…";

QJsonObject horizon(const QJsonArray &hours, int hour) {
  for (const auto &value : hours) {
    const auto item = value.toObject();
    if (item["hour"].toInt() == hour) return item;
  }
  return {};
}

QString peaks(const QJsonArray &hours) {
  QStringList periods;
  for (const auto &value : hours) {
    const auto item = value.toObject();
    if (!item["isPeak"].toBool()) continue;
    const auto dateTime = QDateTime::fromString(item["time"].toString(),
                                                Qt::ISODate);
    periods << (dateTime.isValid()
                  ? dateTime.toLocalTime().toString("MM-dd HH:mm")
                  : QString("+%1h").arg(item["hour"].toInt()));
  }
  return periods.isEmpty() ? "无高峰预警" : periods.join("、");
}
} // namespace

void AdminMainWindow::Impl::buildForecasts() {
  auto *row = new QHBoxLayout;
  auto *layout = page("站点与电桩负荷预测", row);
  row->addStretch();
  forecastStation = new QComboBox;
  forecastStation->setObjectName("forecastStationFilter");
  forecastStation->addItem("全部电站", 0);
  row->addWidget(new QLabel("查看电站"));
  forecastStation->setMinimumWidth(180);
  row->addWidget(forecastStation);
  runForecast = button("更新未来 24 小时预测", row);
  runForecast->setObjectName("runForecastButton");
  layout->addLayout(row);
  auto *metadata = new QHBoxLayout;
  metadata->setSpacing(16);
  forecastMeta = new QLabel("尚未加载预测数据");
  forecastMeta->setObjectName("forecastMeta");
  forecastMeta->setWordWrap(true);
  forecastMeta->setTextInteractionFlags(Qt::TextSelectableByMouse);
  metadata->addWidget(forecastMeta, 1);
  forecastState = new QLabel;
  forecastState->setObjectName("forecastState");
  forecastState->setWordWrap(true);
  metadata->addWidget(forecastState);
  forecastWarnings = new QLabel;
  forecastWarnings->setObjectName("forecastWarnings");
  forecastWarnings->setWordWrap(true);
  metadata->addWidget(forecastWarnings);
  layout->addLayout(metadata);
  auto *tabs = new QTabWidget;
  stationForecasts = new DataTable(
    {"电站", "+1h 负荷 (kW)", "+1h 空闲桩", "+6h 负荷 (kW)", "+6h 空闲桩",
     "+24h 负荷 (kW)", "+24h 空闲桩", "高峰预警时段"},
    "stationForecastTable");
  chargerForecasts = new DataTable({"电站", "电桩编号", "+1h 负荷 (kW)",
                                    "+6h 负荷 (kW)", "+24h 负荷 (kW)",
                                    "高峰预警时段"},
                                   "chargerForecastTable");
  tabs->addTab(stationForecasts->panel(), "站级负荷与空闲电桩");
  tabs->addTab(chargerForecasts->panel(), "桩级负荷");
  layout->addWidget(tabs, 1);
  QObject::connect(forecastStation,
                   qOverload<int>(&QComboBox::currentIndexChanged), w, [this] {
                     if (loggedIn) refreshForecasts();
                   });
  QObject::connect(runForecast, &QPushButton::clicked, w, [this] {
    if (forecastRunning || forecastRequestPending) return;
    ++revisions["forecasts.status"];
    forecastRequestPending = true;
    runForecast->setEnabled(false);
    forecastState->setText("正在提交预测任务…");
    call(
      w, "forecasts.run", {},
      [this](QJsonValue) {
        forecastRequestPending = false;
        forecastRunning = true;
        forecastState->setText(runningMessage);
        forecastPoll->start();
      },
      [this](const QString &message) {
        forecastRequestPending = false;
        runForecast->setEnabled(!forecastRunning);
        forecastState->setText(message);
      });
  });
}

void AdminMainWindow::Impl::refreshForecasts() {
  QJsonObject params;
  if (forecastStation->currentData().toInt() > 0)
    params["stationId"] = forecastStation->currentData().toInt();
  read("forecasts.list", params, [this](QJsonValue data) {
    const auto output = data.toObject();
    const auto stations = output["stations"].toArray();
    if (stations.isEmpty()) {
      forecastMeta->setText("暂无预测结果");
    } else {
      forecastMeta->setText(QString("生成时间：%1 · 模型：%2")
                              .arg(timeText(output["generatedAt"]),
                                   output["modelVersion"].toString()));
    }
    QJsonArray stationRows, chargerRows;
    int warningStations = 0;
    for (const auto &value : stations) {
      auto item = value.toObject();
      item["id"] = item["stationId"];
      stationRows.append(item);
      const auto hours = item["hours"].toArray();
      if (std::any_of(hours.begin(), hours.end(), [](const QJsonValue &hour) {
            return hour.toObject()["isPeak"].toBool();
          }))
        ++warningStations;
      for (const auto &charger : item["chargers"].toArray()) {
        auto row = charger.toObject();
        row["id"] = row["chargerId"];
        row["stationName"] = item["stationName"];
        chargerRows.append(row);
      }
    }
    stationForecasts->fill(
      stationRows, [](const QJsonObject &item) -> QVariantList {
        const auto hours = item["hours"].toArray();
        QVariantList row{item["stationName"].toString()};
        for (const auto h : {1, 6, 24}) {
          const auto forecast = horizon(hours, h);
          row << numberCell(forecast["loadKw"], 2)
              << numberCell(forecast["availableChargers"], 0);
        }
        row << peaks(hours);
        return row;
      });
    chargerForecasts->fill(
      chargerRows, [](const QJsonObject &item) -> QVariantList {
        const auto hours = item["hours"].toArray();
        return {item["stationName"].toString(),
                item["code"].toString(),
                numberCell(horizon(hours, 1)["loadKw"], 2),
                numberCell(horizon(hours, 6)["loadKw"], 2),
                numberCell(horizon(hours, 24)["loadKw"], 2),
                peaks(hours)};
      });
    forecastWarnings->setText(
      QString("未来 24 小时高峰预警：%1 站").arg(warningStations));
  });
}

void AdminMainWindow::Impl::refreshForecastStatus() {
  if (forecastRequestPending) return;
  read("forecasts.status", {}, [this](QJsonValue data) {
    const auto status = data.toObject();
    const bool wasRunning = forecastRunning;
    forecastRunning = status["running"].toBool();
    runForecast->setEnabled(!forecastRunning && !forecastRequestPending);
    if (forecastRunning) {
      forecastState->setText(runningMessage);
      if (!forecastPoll->isActive()) forecastPoll->start();
      return;
    }
    forecastPoll->stop();
    const auto lastError = status["lastError"].toString();
    if (!lastError.isEmpty()) {
      forecastState->setText("预测失败：" + lastError);
    } else {
      forecastState->clear();
    }
    if (wasRunning) refreshForecasts();
  });
}
