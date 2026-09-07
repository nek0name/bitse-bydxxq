#include "AdminUi.h"
#include "AdminWindowState.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <memory>

using namespace adminui;

void AdminMainWindow::Impl::buildStations() {
  auto *row = new QHBoxLayout;
  auto *layout = page("充电站管理", row);
  auto *add = button("新增电站", row);
  add->setObjectName("addStationButton");
  auto *edit = button("编辑电站", row);
  auto *detail = button("站内电桩详情", row);
  stationTable = new DataTable({"电站 ID", "编号", "站名", "区域", "详细地址",
                                "纬度", "经度", "价格 (元/kWh)", "电桩总数",
                                "空闲数量", "在线率 (%)"},
                               "stationTable");
  stationTable->setColumnWidth(0, 108);
  stationTable->setColumnWidth(1, 110);
  stationTable->setColumnWidth(2, 200);
  stationTable->setColumnWidth(4, 280);
  layout->addWidget(stationTable->panel(row), 1);
  QObject::connect(add, &QPushButton::clicked, w, [this] {
    stationEditor({});
  });
  QObject::connect(edit, &QPushButton::clicked, w, [this] {
    const auto item = selected(stationTable);
    if (!item.isEmpty()) stationEditor(item);
  });
  auto showDetail = [this] {
    const auto item = selected(stationTable);
    if (!item.isEmpty()) stationDetail(item);
  };
  QObject::connect(detail, &QPushButton::clicked, w, showDetail);
  QObject::connect(stationTable, &QTableWidget::cellDoubleClicked, w,
                   showDetail);
  auto enable = [this, edit, detail] {
    const bool hasSelection = !selected(stationTable).isEmpty();
    edit->setEnabled(hasSelection);
    detail->setEnabled(hasSelection);
  };
  QObject::connect(stationTable, &QTableWidget::itemSelectionChanged, w,
                   enable);
  enable();
}

void AdminMainWindow::Impl::refreshStations() {
  read("admin.stations", {}, [this](QJsonValue data) {
    const auto stations = data.toArray();
    stationTable->fill(stations, [](const QJsonObject &o) -> QVariantList {
      return {o["id"].toInt(),
              o["code"].toString(),
              o["name"].toString(),
              o["region"].toString(),
              o["address"].toString(),
              numberCell(o["latitude"], 6),
              numberCell(o["longitude"], 6),
              moneyCell(o["priceCents"]),
              o["totalChargers"].toInt(),
              o["idleChargers"].toInt(),
              numberCell(o["onlineRate"])};
    });
    const auto selectedId = forecastStation->currentData().toInt();
    const QSignalBlocker blocker(forecastStation);
    forecastStation->clear();
    forecastStation->addItem("全部电站", 0);
    for (const auto &value : stations) {
      const auto station = value.toObject();
      forecastStation->addItem(station["name"].toString(),
                               station["id"].toInt());
    }
    forecastStation->setCurrentIndex(
      qMax(0, forecastStation->findData(selectedId)));
  });
}

void AdminMainWindow::Impl::stationEditor(const QJsonObject &existing) {
  const bool editing = !existing.isEmpty();
  auto *dialog = new QDialog(w);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(editing ? "编辑电站" : "新增电站");
  dialog->setMinimumWidth(500);
  auto *layout = new QVBoxLayout(dialog);
  auto *form = new QFormLayout;
  auto *name = new QLineEdit(existing["name"].toString());
  name->setObjectName("stationName");
  name->setMaxLength(60);
  auto *address = new QLineEdit(existing["address"].toString());
  address->setObjectName("stationAddress");
  address->setMaxLength(200);
  auto *region = new QLineEdit(existing["region"].toString());
  region->setObjectName("stationRegion");
  region->setMaxLength(60);
  auto *latitude = new QDoubleSpinBox;
  latitude->setObjectName("stationLatitude");
  latitude->setRange(-90, 90);
  latitude->setDecimals(6);
  latitude->setValue(existing["latitude"].toDouble(31.2304));
  auto *longitude = new QDoubleSpinBox;
  longitude->setObjectName("stationLongitude");
  longitude->setRange(-180, 180);
  longitude->setDecimals(6);
  longitude->setValue(existing["longitude"].toDouble(121.4737));
  auto *price = new QDoubleSpinBox;
  price->setObjectName("stationPrice");
  price->setRange(0.01, 100);
  price->setDecimals(2);
  price->setSingleStep(0.1);
  price->setSuffix(" 元/kWh");
  price->setValue(existing["priceCents"].toDouble(120) / 100.0);
  auto *count = new QSpinBox(dialog);
  count->setVisible(!editing);
  count->setObjectName("stationChargerCount");
  count->setRange(1, 100);
  count->setValue(8);
  auto *type = new QComboBox(dialog);
  type->setVisible(!editing);
  type->addItem("混合快慢充", "");
  type->addItem("全部直流快充", "dc");
  type->addItem("全部交流慢充", "ac");
  auto *power = new QDoubleSpinBox(dialog);
  power->setVisible(!editing);
  power->setRange(1, 500);
  power->setDecimals(1);
  power->setSuffix(" kW");
  power->setValue(60);
  power->setEnabled(false);
  QObject::connect(
    type, qOverload<int>(&QComboBox::currentIndexChanged), dialog,
    [type, power] {
      power->setEnabled(!type->currentData().toString().isEmpty());
      power->setValue(type->currentData().toString() == "ac" ? 7 : 60);
    });
  form->addRow("站名 *", name);
  form->addRow("详细地址 *", address);
  form->addRow("所属区域 *", region);
  form->addRow("纬度 (−90 ~ 90) *", latitude);
  form->addRow("经度 (−180 ~ 180) *", longitude);
  form->addRow("充电单价 *", price);
  if (!editing) {
    form->addRow("电桩数量 *", count);
    form->addRow("电桩类型", type);
    form->addRow("单桩额定功率", power);
  }
  layout->addLayout(form);
  if (editing) layout->addWidget(new QLabel("新价格从下次开始充电起生效。"));
  auto *error = new QLabel;
  error->setObjectName("stationError");
  error->setWordWrap(true);
  layout->addWidget(error);
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save
                                       | QDialogButtonBox::Cancel);
  buttons->button(QDialogButtonBox::Save)->setText("保存");
  buttons->button(QDialogButtonBox::Save)->setIcon(QIcon());
  buttons->button(QDialogButtonBox::Save)->setObjectName("saveStationButton");
  buttons->button(QDialogButtonBox::Cancel)->setText("取消");
  buttons->button(QDialogButtonBox::Cancel)->setIcon(QIcon());
  layout->addWidget(buttons);
  QObject::connect(buttons, &QDialogButtonBox::rejected, dialog,
                   &QDialog::reject);
  QObject::connect(
    buttons, &QDialogButtonBox::accepted, dialog,
    [this, dialog, existing, name, address, region, latitude, longitude, price,
     count, type, power, buttons, error, editing] {
      if (name->text().trimmed().isEmpty()
          || address->text().trimmed().isEmpty()
          || region->text().trimmed().isEmpty()) {
        error->setText("请填写站名、详细地址和所属区域。");
        return;
      }
      QJsonObject params{{"name", name->text().trimmed()},
                         {"address", address->text().trimmed()},
                         {"region", region->text().trimmed()},
                         {"latitude", latitude->value()},
                         {"longitude", longitude->value()},
                         {"priceCents", qRound(price->value() * 100)}};
      if (editing)
        params["id"] = existing["id"];
      else {
        params["chargerCount"] = count->value();
        if (!type->currentData().toString().isEmpty()) {
          params["type"] = type->currentData().toString();
          params["powerKw"] = power->value();
        }
      }
      buttons->button(QDialogButtonBox::Save)->setEnabled(false);
      buttons->button(QDialogButtonBox::Save)->setText("保存中…");
      error->clear();
      call(
        dialog, "admin.station.save", params,
        [this, dialog](QJsonValue) {
          dialog->accept();
          w->statusBar()->showMessage("电站已保存", 6000);
          refreshStations();
        },
        [buttons, error](const QString &message) {
          buttons->button(QDialogButtonBox::Save)->setEnabled(true);
          buttons->button(QDialogButtonBox::Save)->setText("保存");
          error->setText(message);
        });
    });
  dialog->open();
}

void AdminMainWindow::Impl::chargerActions(
  DataTable *target, QHBoxLayout *toolbar, QObject *owner,
  const std::function<void()> &refresh) {
  auto *restart = button("远程重启", toolbar);
  restart->setObjectName("restartChargerButton");
  auto *fault = button("标记故障", toolbar);
  fault->setObjectName("markChargerFaultButton");
  auto *restore = button("恢复可用", toolbar);
  restore->setObjectName("restoreChargerButton");
  const auto pendingSession = std::make_shared<int>(-1);
  auto update = [this, target, restart, fault, restore, pendingSession] {
    const auto item = selected(target);
    const auto status = item["status"].toString();
    const bool available = *pendingSession != session && !item.isEmpty()
                        && status != "charging" && status != "reserved"
                        && status != "restarting";
    restart->setEnabled(available);
    fault->setEnabled(available && status != "fault");
    restore->setEnabled(available && status != "idle");
  };
  QObject::connect(target, &QTableWidget::itemSelectionChanged, owner, update);
  update();
  QObject::connect(
    restart, &QPushButton::clicked, owner,
    [this, target, owner, refresh, pendingSession, update] {
      const auto item = selected(target);
      if (item.isEmpty()) return;
      *pendingSession = session;
      update();
      call(
        owner, "admin.charger.restart", {{"chargerId", item["id"]}},
        [this, owner, refresh, pendingSession, update](QJsonValue data) {
          *pendingSession = -1;
          update();
          w->statusBar()->showMessage(data.toObject()["message"].toString(),
                                      10000);
          refresh();
          const int epoch = session;
          QTimer::singleShot(3500, owner, [this, epoch, refresh] {
            if (loggedIn && session == epoch) refresh();
          });
        },
        [this, pendingSession, update](const QString &message) {
          *pendingSession = -1;
          update();
          w->statusBar()->showMessage(message, 12000);
        });
    });
  for (auto entry : {qMakePair(fault, QString("fault")),
                     qMakePair(restore, QString("idle"))}) {
    QObject::connect(
      entry.first, &QPushButton::clicked, owner,
      [this, target, owner, refresh, entry, pendingSession, update] {
        const auto item = selected(target);
        if (item.isEmpty()) return;
        *pendingSession = session;
        update();
        call(
          owner, "admin.charger.status",
          {{"chargerId", item["id"]}, {"status", entry.second}},
          [this, refresh, pendingSession, update](QJsonValue) {
            *pendingSession = -1;
            update();
            w->statusBar()->showMessage("电桩状态已更新", 6000);
            refresh();
          },
          [this, pendingSession, update](const QString &message) {
            *pendingSession = -1;
            update();
            w->statusBar()->showMessage(message, 12000);
          });
      });
  }
}

void AdminMainWindow::Impl::stationDetail(const QJsonObject &station) {
  auto *dialog = new QDialog(w);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(station["name"].toString() + " · 站内电桩");
  dialog->resize(1080, 560);
  auto *layout = new QVBoxLayout(dialog);
  auto *address = new QLabel(station["address"].toString());
  address->setWordWrap(true);
  layout->addWidget(address);
  auto *toolbar = new QHBoxLayout;
  auto *refreshButton = button("刷新设备", toolbar);
  refreshButton->setObjectName("refreshStationChargersButton");
  auto *details = new DataTable(chargerHeaders(), "stationChargerTable");
  layout->addWidget(details->panel(toolbar), 1);
  const auto revision = std::make_shared<int>(0);
  auto refresh = [this, dialog, details, station, revision] {
    const int request = ++*revision;
    call(
      dialog, "admin.chargers", {{"stationId", station["id"]}},
      [details, revision, request](QJsonValue data) {
        if (request != *revision) return;
        details->fill(data.toArray(), chargerColumns);
      },
      [this, revision, request](const QString &message) {
        if (request == *revision) w->statusBar()->showMessage(message, 12000);
      });
  };
  chargerActions(details, toolbar, dialog, refresh);
  QObject::connect(refreshButton, &QPushButton::clicked, dialog, refresh);
  auto *timer = new QTimer(dialog);
  timer->setInterval(5000);
  QObject::connect(timer, &QTimer::timeout, dialog, refresh);
  timer->start();
  refresh();
  dialog->open();
}

void AdminMainWindow::Impl::buildChargers() {
  auto *toolbar = new QHBoxLayout;
  auto *layout = page("充电桩管理", toolbar);
  chargerTable = new DataTable(chargerHeaders(), "chargerTable");
  const QList<int> widths{108, 104, 160, 100, 120, 88, 140, 180, 140};
  for (int column = 0; column < widths.size(); ++column)
    chargerTable->setColumnWidth(column, widths[column]);
  layout->addWidget(chargerTable->panel(toolbar), 1);
  chargerActions(chargerTable, toolbar, w, [this] {
    refreshChargers();
  });
}

void AdminMainWindow::Impl::refreshChargers() {
  read("admin.chargers", {}, [this](QJsonValue data) {
    chargerTable->fill(data.toArray(), chargerColumns);
  });
}
