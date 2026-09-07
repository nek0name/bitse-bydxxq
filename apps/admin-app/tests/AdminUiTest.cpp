#include "AdminUi.h"
#include "AdminMainWindow.h"
#include "Appearance.h"
#include "Fonts.h"

#include <QApplication>
#include <QCheckBox>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusVariant>
#include <QFontInfo>
#include <QListWidget>
#include <QSettings>
#include <QTemporaryDir>
#include <QToolButton>

#include <QChartView>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLineSeries>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>
#include <memory>

// Hold HTTP responses to exercise callbacks after logout or dialog closure.
class RpcFixture : public QObject {
public:
  QTcpServer server;
  QJsonArray stations{{QJsonObject{{"id", 1},
                                   {"code", "ST001"},
                                   {"name", "人民广场站"},
                                   {"region", "黄浦区"},
                                   {"address", "人民大道 100 号"},
                                   {"latitude", 31.23},
                                   {"longitude", 121.47},
                                   {"priceCents", 123},
                                   {"totalChargers", 2},
                                   {"idleChargers", 1},
                                   {"onlineRate", 100}}}};
  QJsonArray chargers{QJsonObject{{"id", 1},
                                  {"stationId", 1},
                                  {"stationName", "人民广场站"},
                                  {"code", "DC001"},
                                  {"type", "dc"},
                                  {"powerKw", 60},
                                  {"status", "idle"},
                                  {"chargingCount", 7},
                                  {"chargingSeconds", 3661},
                                  {"energyKwh", 125.5}},
                      QJsonObject{{"id", 2},
                                  {"stationId", 1},
                                  {"stationName", "人民广场站"},
                                  {"code", "AC002"},
                                  {"type", "ac"},
                                  {"powerKw", 7},
                                  {"status", "charging"},
                                  {"chargingCount", 2},
                                  {"chargingSeconds", 120},
                                  {"energyKwh", 4}}};
  QJsonArray users{{QJsonObject{{"id", 1},
                                {"phone", "13800001234"},
                                {"nickname", "小明"},
                                {"balanceCents", 36109},
                                {"status", "active"},
                                {"createdAt", "2026-09-05T00:00:00Z"}}}};
  QMap<QString, QString> errors;
  QMap<QString, int> requests;
  QMap<QString, QJsonObject> lastParams;
  bool running = false;
  bool holdUsers = false;
  QList<QPointer<QTcpSocket>> heldUsers;
  QString holdNextAction;
  QPointer<QTcpSocket> heldSocket;
  QJsonObject heldResponse;
  int unauthorized = 0;

  RpcFixture() {
    server.listen(QHostAddress::LocalHost, 0);
    connect(&server, &QTcpServer::newConnection, this, [this] {
      while (auto *socket = server.nextPendingConnection()) {
        connect(socket, &QTcpSocket::disconnected, socket,
                &QObject::deleteLater);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
          auto bytes = socket->property("bytes").toByteArray()
                     + socket->readAll();
          socket->setProperty("bytes", bytes);
          const auto headerEnd = bytes.indexOf("\r\n\r\n");
          if (headerEnd < 0 || socket->property("handled").toBool()) return;
          int length = 0;
          for (const auto &header : bytes.left(headerEnd).split('\n')) {
            if (header.toLower().startsWith("content-length:"))
              length = header.mid(15).trimmed().toInt();
          }
          if (bytes.size() < headerEnd + 4 + length) return;
          socket->setProperty("handled", true);
          const auto rpc = QJsonDocument::fromJson(
                             bytes.mid(headerEnd + 4, length))
                             .object();
          const auto action = rpc["action"].toString();
          const auto params = rpc["params"].toObject();
          ++requests[action];
          lastParams[action] = params;
          if (action != "admin.login"
              && !bytes.left(headerEnd).contains("Bearer admin-token"))
            ++unauthorized;
          if (holdUsers && action == "admin.users") {
            heldUsers.append(socket);
            return;
          }
          if (action == "admin.login"
              && params["password"].toString() != "123456") {
            respond(socket,
                    {{"ok", false},
                     {"error", QJsonObject{{"code", "BAD_LOGIN"},
                                           {"message", "账号或密码错误"}}}});
            return;
          }
          const QJsonObject
            response = errors.contains(action)
                       ? QJsonObject{{"ok", false},
                                     {"error",
                                      QJsonObject{{"code", "FAILED"},
                                                  {"message", errors[action]}}}}
                       : QJsonObject{{"ok", true},
                                     {"data", dispatch(action, params)}};
          if (action == holdNextAction) {
            holdNextAction.clear();
            heldSocket = socket;
            heldResponse = response;
            return;
          }
          respond(socket, response);
        });
      }
    });
  }

  QString url() const {
    return QString("http://127.0.0.1:%1").arg(server.serverPort());
  }

  static void respond(QTcpSocket *socket, const QJsonObject &object) {
    const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
    socket->write(
      "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
      + QByteArray::number(bytes.size()) + "\r\nConnection: close\r\n\r\n"
      + bytes);
    socket->disconnectFromHost();
  }

  void releaseUsers() {
    holdUsers = false;
    for (auto socket : heldUsers) {
      if (socket) respond(socket, {{"ok", true}, {"data", users}});
    }
    heldUsers.clear();
  }

  void releaseResponse() {
    if (heldSocket) respond(heldSocket, heldResponse);
    heldSocket.clear();
  }

  QJsonValue dispatch(const QString &action, const QJsonObject &params) {
    if (action == "admin.login")
      return QJsonObject{{"token", "admin-token"}, {"username", "admin"}};
    if (action == "auth.logout") return QJsonObject{};
    if (action == "admin.overview") {
      QJsonArray trend;
      const int days = params["days"].toInt(7);
      for (int index = 0; index < days; ++index) {
        trend.append(QJsonObject{
          {"date",
           QDate(2026, 9, 5).addDays(index - days + 1).toString("yyyy-MM-dd")},
          {"revenueCents", (index + 1) * 123},
          {"orderCount", index + 1}});
      }
      return QJsonObject{
        {"todayRevenueCents", 123},
        {"monthRevenueCents", 45678},
        {"totalRevenueCents", 98765},
        {"todayOrders", 3},
        {"statusCounts", QJsonArray{QJsonObject{{"status", "idle"},
                                                {"label", "闲置"},
                                                {"count", 1},
                                                {"percent", 50}},
                                    QJsonObject{{"status", "charging"},
                                                {"label", "在用"},
                                                {"count", 1},
                                                {"percent", 50}}}},
        {"revenueTrend", trend}};
    }
    if (action == "admin.stations") return stations;
    if (action == "admin.station.save") {
      auto station = params;
      station["id"] = params["id"].toInt(2);
      station["code"] = "ST002";
      station["totalChargers"] = params["chargerCount"].toInt(2);
      station["idleChargers"] = station["totalChargers"];
      station["onlineRate"] = 100;
      if (params.contains("id"))
        stations[0] = station;
      else
        stations.append(station);
      return station;
    }
    if (action == "admin.chargers") {
      QJsonArray result;
      for (const auto &value : chargers) {
        auto row = value.toObject();
        if (params.contains("status") && row["status"] != params["status"])
          continue;
        result.append(row);
      }
      return result;
    }
    if (action == "admin.charger.restart" || action == "admin.charger.status") {
      for (int i = 0; i < chargers.size(); ++i) {
        auto item = chargers[i].toObject();
        if (item["id"] != params["chargerId"]) continue;
        item["status"] = action.endsWith("restart") ? QJsonValue("restarting")
                                                    : params["status"];
        chargers[i] = item;
      }
      return QJsonObject{{"message", "重启指令已发送"}};
    }
    if (action == "admin.users") return users;
    if (action == "admin.user.status") {
      auto user = users[0].toObject();
      user["status"] = params["status"];
      users[0] = user;
      return user;
    }
    if (action == "admin.orders")
      return QJsonArray{QJsonObject{{"id", 1},
                                    {"userId", 1},
                                    {"orderNo", "ORDER001"},
                                    {"stationName", "人民广场站"},
                                    {"chargerCode", "DC001"},
                                    {"status", "paid"},
                                    {"createdAt", "2026-09-05T00:00:00Z"},
                                    {"durationSeconds", 60},
                                    {"energyKwh", 1},
                                    {"amountCents", 123}}};
    if (action == "admin.logs")
      return QJsonArray{QJsonObject{{"id", 1},
                                    {"action", "admin.charger.restart"},
                                    {"target", "DC001"},
                                    {"detail", "管理员重启设备"},
                                    {"createdAt", "2026-09-05T00:00:00Z"}}};
    if (action == "forecasts.run") {
      running = true;
      return QJsonObject{{"running", true}};
    }
    if (action == "forecasts.status")
      return QJsonObject{{"running", running},
                         {"lastError", ""},
                         {"lastRunAt", "2026-09-05T00:00:00Z"}};
    if (action == "forecasts.list") {
      QJsonArray hours;
      for (int h = 1; h <= 24; ++h)
        hours.append(QJsonObject{{"hour", h},
                                 {"time", "2026-09-05T01:00:00Z"},
                                 {"loadKw", h * 1.5},
                                 {"availableChargers", 2},
                                 {"isPeak", h == 6}});
      return QJsonObject{
        {"generatedAt", "2026-09-05T00:00:00Z"},
        {"modelVersion", "baseline-v1"},
        {"source", "课程演示业务数据；样本不足，使用基线估计"},
        {"stations",
         QJsonArray{QJsonObject{
           {"stationId", 1},
           {"stationName", "人民广场站"},
           {"hours", hours},
           {"chargers",
            QJsonArray{QJsonObject{
              {"chargerId", 1}, {"code", "DC001"}, {"hours", hours}}}}}}}};
    }
    return QJsonObject{};
  }
};

using PortalSettings = QMap<QString, QVariantMap>;
class PortalFixture : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.portal.Settings")
public:
  uint preference = 1;
  int reads = 0;
  void setPreference(uint value) {
    preference = value;
    emit SettingChanged("org.freedesktop.appearance", "color-scheme",
                        QDBusVariant(value));
  }
public slots:
  PortalSettings ReadAll(const QStringList &) {
    ++reads;
    const auto initial = preference;
    setPreference(2);
    return {{"org.freedesktop.appearance", {{"color-scheme", initial}}}};
  }
signals:
  void SettingChanged(const QString &group, const QString &key,
                      const QDBusVariant &value);
};

class AdminUiTest : public QObject {
  Q_OBJECT
private:
  QTemporaryDir appearanceDirectory;
  PortalFixture portal;
  std::unique_ptr<RpcFixture> fixture;
  std::unique_ptr<AdminMainWindow> window;
  template <class T> T *widget(const char *name) {
    return window->findChild<T *>(name);
  }
  QPushButton *nav(int index) {
    return widget<QPushButton>(
      qPrintable("navigation" + QString::number(index)));
  }
  bool login() {
    widget<QLineEdit>("adminPassword")->setText("123456");
    widget<QPushButton>("adminLoginButton")->click();
    return QTest::qWaitFor([this] {
      return widget<QTableWidget>("trendTable")->rowCount() == 7;
    });
  }
private slots:
  void initTestCase() {
    QVERIFY(appearanceDirectory.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       appearanceDirectory.path());
    qRegisterMetaType<PortalSettings>("PortalSettings");
    qDBusRegisterMetaType<PortalSettings>();
    auto bus = QDBusConnection::sessionBus();
    QVERIFY(bus.registerService("org.freedesktop.portal.Desktop"));
    QVERIFY(bus.registerObject("/org/freedesktop/portal/desktop", &portal,
                               QDBusConnection::ExportAllSlots
                                 | QDBusConnection::ExportAllSignals));
    loadFonts();
    adminui::applyTheme();
    QTRY_COMPARE(portal.reads, 1);
    QTRY_VERIFY(
      !Appearance::instance()->findChild<QDBusPendingCallWatcher *>());
    QVERIFY(!Appearance::instance()->dark());
    QCOMPARE(QFontInfo(QApplication::font()).family(),
             QString("HarmonyOS Sans SC"));
  }

  void init() {
    auto *appearance = Appearance::instance();
    appearance->setMode("light");
    appearance->setPrimaryColor(QColor("#6259CA"));
    appearance->setSecondaryColor(QColor("#8E86B8"));
    fixture = std::make_unique<RpcFixture>();
    qputenv("CHARGING_SERVER_URL", fixture->url().toUtf8());
    window = std::make_unique<AdminMainWindow>();
    window->show();
  }

  void cleanup() {
    window.reset();
    fixture.reset();
  }

  void appearanceSettingsPersistAndFollowSystem() {
    QVERIFY(login());
    auto *appearance = Appearance::instance();
    auto *chart = widget<QChartView>("revenueChart")->chart();
    widget<QAbstractButton>("adminSettingsButton")->click();
    auto *dialog = widget<QDialog>("adminSettingsDialog");
    QVERIFY(dialog);
    dialog->findChild<QToolButton *>("appearanceModeDark")->click();
    QVERIFY(appearance->dark());
    QCOMPARE(chart->backgroundBrush().color(),
             appearance->colors()["card"].value<QColor>());
    dialog->findChild<QAbstractButton *>("primarySwatch3867a6")->click();
    dialog->findChild<QAbstractButton *>("secondarySwatch287b73")->click();
    QCOMPARE(appearance->primaryColor(), QColor("#3867A6"));
    QCOMPARE(appearance->secondaryColor(), QColor("#287B73"));
    QCOMPARE(chart->series().first()->property("visible").toBool(), true);
    QCOMPARE(
      qobject_cast<QLineSeries *>(chart->series().first())->pen().color(),
      appearance->colors()["secondary"].value<QColor>());
    QSettings saved(QSettings::IniFormat, QSettings::UserScope,
                    "ChargingPlatform", "Appearance");
    QCOMPARE(saved.value("mode").toString(), QString("dark"));
    QCOMPARE(QColor(saved.value("primaryColor").toString()), QColor("#3867A6"));
    QCOMPARE(QColor(saved.value("secondaryColor").toString()),
             QColor("#287B73"));
    dialog->findChild<QToolButton *>("appearanceModeSystem")->click();
    portal.setPreference(1);
    QTRY_VERIFY(appearance->dark());
    portal.setPreference(2);
    QTRY_VERIFY(!appearance->dark());
    QCOMPARE(chart->backgroundBrush().color(),
             appearance->colors()["card"].value<QColor>());
    dialog->findChild<QToolButton *>("appearanceModeLight")->click();
    portal.setPreference(1);
    QTest::qWait(30);
    QVERIFY(!appearance->dark());
    dialog->close();
  }

  void serverTimestampFormats() {
    const auto expected = QDateTime(QDate(2026, 9, 5), QTime(0, 0), Qt::UTC)
                            .toLocalTime()
                            .toString("yyyy-MM-dd HH:mm:ss");
    for (const auto *timestamp :
         {"2026-09-05T00:00:00Z", "2026-09-05T00:00:00.123Z",
          "2026-09-05T08:00:00+08:00", "2026-09-05T08:00:00.123+08:00"})
      QCOMPARE(adminui::timeText(timestamp), expected);
    QCOMPARE(adminui::timeText(QJsonValue::Null), QString("—"));
    QCOMPARE(adminui::timeText("invalid"), QString("—"));
    QCOMPARE(adminui::duration(3661), QString("1小时 1分 1秒"));
  }

  void tableRefreshPreservesSelectionAfterSorting() {
    std::unique_ptr<adminui::DataTable> rows(
      new adminui::DataTable({"ID", "金额"}, "records"));
    const auto columns = [](const QJsonObject &record) -> QVariantList {
      return {record["id"].toInt(), adminui::moneyCell(record["balanceCents"])};
    };
    auto first = fixture->users.first().toObject();
    first["balanceCents"] = 900;
    const QJsonObject second{{"id", 2}, {"balanceCents", 1000}};
    rows->fill({first, second}, columns);
    rows->sortItems(1, Qt::AscendingOrder);
    rows->selectRow(0);
    QCOMPARE(adminui::selected(rows.get()), first);
    first["balanceCents"] = 1200;
    rows->fill({second, first}, columns);
    QCOMPARE(rows->currentRow(), 1);
    QCOMPARE(adminui::selected(rows.get()), first);
  }

  void tableFiltersCopyExportAndRefresh() {
    auto *table = new adminui::DataTable({"ID", "名称", "金额"}, "sheet");
    std::unique_ptr<QWidget> panel(table->panel());
    const auto columns = [](const QJsonObject &record) -> QVariantList {
      return {record["id"].toInt(), record["name"].toString(),
              adminui::moneyCell(record["cents"])};
    };
    QJsonArray records{
      QJsonObject{{"id", 1}, {"name", "North, \"A\""}, {"cents", 900}},
      QJsonObject{{"id", 2}, {"name", "North B"}, {"cents", 1200}},
      QJsonObject{{"id", 3}, {"name", "South"}, {"cents", 10000}}};
    table->fill(records, columns);
    table->sortItems(2, Qt::DescendingOrder);
    QCOMPARE(table->item(0, 0)->text(), QString("3"));
    table->setSearch("north");
    table->setColumnFilter(2, "10", ">");
    QVERIFY(table->isRowHidden(0));
    QVERIFY(!table->isRowHidden(1));
    QVERIFY(table->isRowHidden(2));
    table->selectRow(1);
    QCOMPARE(table->copyText(true),
             QString("ID\t名称\t金额\n2\tNorth B\t¥ 12.00"));
    auto changed = records[1].toObject();
    changed["cents"] = 15000;
    records[1] = changed;
    table->fill(records, columns);
    QCOMPARE(table->currentRow(), 0);
    QCOMPARE(adminui::selected(table)["id"].toInt(), 2);
    QVERIFY(!table->isRowHidden(0));
    QVERIFY(table->isRowHidden(1));
    table->setColumnFilter(1, "", "包含", {"North B"});
    QVERIFY(adminui::selected(table).isEmpty());
    QVERIFY(table->copyText().isEmpty());
    table->clearFilters();
    table->horizontalHeader()->moveSection(2, 0);
    QVERIFY(table->csvText().startsWith("\"金额\",\"ID\",\"名称\"\r\n"));
    QVERIFY(table->csvText().contains("\"North, \"\"A\"\"\""));
    table->item(0, 1)->setSelected(true);
    table->item(1, 1)->setSelected(true);
    QVERIFY(adminui::selected(table).isEmpty());
    table->fill(records, columns);
    QCOMPARE(table->selectedItems().size(), 2);
    QCOMPARE(table->selectedItems()[0]->column(), 1);
    QCOMPARE(table->selectedItems()[1]->column(), 1);
    QVERIFY(adminui::selected(table).isEmpty());
  }

  void headerTextSortsAndIconFilters() {
    auto *table = new adminui::DataTable({"ID", "名称"}, "headerActions");
    std::unique_ptr<QWidget> panel(table->panel());
    panel->resize(500, 300);
    panel->show();
    table->fill({QJsonObject{{"id", 1}, {"name", "alpha"}},
                 QJsonObject{{"id", 2}, {"name", "beta"}}},
                [](const QJsonObject &row) -> QVariantList {
                  return {row["id"].toInt(), row["name"].toString()};
                });
    auto *header = table->horizontalHeader();
    QVERIFY(header->sectionsClickable());
    QTest::mouseClick(
      header->viewport(), Qt::LeftButton, Qt::NoModifier,
      QPoint(header->sectionViewportPosition(1) + 20, header->height() / 2));
    QCOMPARE(header->sortIndicatorSection(), 1);
    table->sortItems(0, Qt::DescendingOrder);
    header->moveSection(1, 0);
    bool opened = false;
    QTimer::singleShot(100, table, [&] {
      auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
      if (!menu) return;
      opened = true;
      menu->findChild<QLineEdit *>("columnFilterQuery")->setText("beta");
      menu->findChild<QPushButton *>("applyColumnFilter")->click();
    });
    QTest::mouseClick(
      header->viewport(), Qt::LeftButton, Qt::NoModifier,
      QPoint(header->sectionViewportPosition(1) + header->sectionSize(1) - 16,
             header->height() / 2));
    QTRY_VERIFY(opened);
    QCOMPARE(header->sortIndicatorSection(), 0);
    QCOMPARE(header->sortIndicatorOrder(), Qt::DescendingOrder);
    QVERIFY(!table->isRowHidden(0));
    QVERIFY(table->isRowHidden(1));
    QCOMPARE(table->item(0, 1)->text(), QString("beta"));
  }

  void tableSearchKeyboardAndEmptyState() {
    auto *table = new adminui::DataTable({"ID", "名称"}, "keyboardSheet");
    std::unique_ptr<QWidget> panel(table->panel());
    panel->resize(500, 300);
    panel->show();
    panel->activateWindow();
    table->fill({QJsonObject{{"id", 1}, {"name", "alpha"}},
                 QJsonObject{{"id", 2}, {"name", "beta"}}},
                [](const QJsonObject &row) -> QVariantList {
                  return {row["id"].toInt(), row["name"].toString()};
                });
    auto *search = panel->findChild<QLineEdit *>("keyboardSheetSearch");
    auto *count = panel->findChild<QLabel *>("dataTableCount");
    auto *empty = table->findChild<QWidget *>("dataTableEmpty");
    QCOMPARE(count->text(), QString("2 行"));
    QVERIFY(!empty->isVisible());
    table->setFocus();
    QTRY_VERIFY(table->hasFocus());
    QTest::keyClick(table, Qt::Key_F, Qt::ControlModifier);
    QTRY_VERIFY(search->hasFocus());
    QTest::keyClicks(search, "absent");
    QVERIFY(empty->isVisible());
    QCOMPARE(count->text(), QString("0 / 2 行"));
    QTest::keyClick(search, Qt::Key_Escape);
    QTRY_VERIFY(table->hasFocus());
    QVERIFY(search->text().isEmpty());
    QVERIFY(!empty->isVisible());
    auto *more = panel->findChild<QToolButton *>("dataTableMore");
    more->setFocus();
    QTest::keyClick(more, Qt::Key_F, Qt::ControlModifier);
    QTRY_VERIFY(search->hasFocus());
    search->setText("beta");
    QTest::keyClick(search, Qt::Key_F, Qt::ControlModifier);
    QCOMPARE(search->selectedText(), QString("beta"));
    table->setColumnFilter(1, "missing");
    QVERIFY(empty->isVisible());
    empty->findChild<QPushButton *>("dataTableEmptyClear")->click();
    QVERIFY(search->text().isEmpty());
    QCOMPARE(count->text(), QString("2 行"));
    QVERIFY(!empty->isVisible());
    table->fill({}, [](const QJsonObject &) {
      return QVariantList{};
    });
    QCOMPARE(count->text(), QString("0 行"));
    QVERIFY(empty->isVisible());
    QVERIFY(
      !empty->findChild<QPushButton *>("dataTableEmptyClear")->isVisible());
  }

  void keyboardFilterSelectAllTracksVisibleOptions() {
    auto *table = new adminui::DataTable({"ID", "名称"}, "filterKeyboard");
    std::unique_ptr<QWidget> panel(table->panel());
    panel->resize(500, 300);
    panel->show();
    panel->activateWindow();
    table->fill({QJsonObject{{"id", 1}, {"name", "alpha"}},
                 QJsonObject{{"id", 2}, {"name", "beta"}}},
                [](const QJsonObject &row) -> QVariantList {
                  return {row["id"].toInt(), row["name"].toString()};
                });
    table->setCurrentCell(0, 1);
    table->setFocus();
    QTRY_VERIFY(table->hasFocus());
    bool opened = false;
    bool statesCorrect = true;
    QTimer::singleShot(100, table, [&] {
      auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
      if (!menu) return;
      opened = true;
      auto *ascending = menu->findChild<QToolButton *>("columnSortAscending");
      auto *descending = menu->findChild<QToolButton *>("columnSortDescending");
      auto *query = menu->findChild<QLineEdit *>("columnFilterQuery");
      statesCorrect &= !ascending->icon().isNull()
                    && !descending->icon().isNull();
      query->setText("a");
      descending->click();
      statesCorrect &= table->item(0, 1)->text() == "beta";
      statesCorrect &= descending->isChecked() && !ascending->isChecked();
      statesCorrect &= query->text() == "a";
      ascending->click();
      statesCorrect &= table->item(0, 1)->text() == "alpha";
      statesCorrect &= ascending->isChecked() && !descending->isChecked();
      query->clear();
      auto *all = menu->findChild<QCheckBox *>("columnSelectAll");
      auto *values = menu->findChild<QListWidget *>("columnValues");
      auto *search = menu->findChild<QLineEdit *>("columnValueSearch");
      statesCorrect &= all->checkState() == Qt::Checked;
      statesCorrect &= values->item(0)->text() == "alpha";
      values->item(0)->setCheckState(Qt::Unchecked);
      statesCorrect &= all->checkState() == Qt::PartiallyChecked;
      search->setText("beta");
      statesCorrect &= all->checkState() == Qt::Checked;
      all->click();
      statesCorrect &= values->item(1)->checkState() == Qt::Unchecked;
      search->clear();
      statesCorrect &= all->checkState() == Qt::Unchecked;
      all->click();
      statesCorrect &= all->checkState() == Qt::Checked;
      search->setText("beta");
      all->click();
      statesCorrect &= values->item(0)->checkState() == Qt::Checked;
      menu->findChild<QPushButton *>("applyColumnFilter")->click();
    });
    QTest::keyClick(table, Qt::Key_Down, Qt::AltModifier);
    QTRY_VERIFY(opened);
    QVERIFY(statesCorrect);
    QVERIFY(!table->isRowHidden(0));
    QVERIFY(table->isRowHidden(1));
  }

  void tableSortsNumbersDurationsAndKeepsTextIdentifiers() {
    adminui::DataTable table({"ID", "编号", "时长", "电量"}, "typed");
    const auto columns = [](const QJsonObject &record) -> QVariantList {
      return {record["id"].toInt(), record["code"].toString(),
              adminui::cell(adminui::duration(record["seconds"]),
                            record["seconds"].toDouble()),
              adminui::numberCell(record["energy"], 2)};
    };
    table.fill(
      {QJsonObject{
         {"id", 1}, {"code", "002"}, {"seconds", 36000}, {"energy", 2}},
       QJsonObject{
         {"id", 2}, {"code", "01"}, {"seconds", 7200}, {"energy", 10}},
       QJsonObject{{"id", 3}, {"code", "003"}, {"seconds", 60}}},
      columns);
    table.sortItems(1, Qt::AscendingOrder);
    QCOMPARE(table.item(1, 0)->text(), QString("3"));
    table.sortItems(2, Qt::AscendingOrder);
    QCOMPARE(table.item(0, 0)->text(), QString("3"));
    QCOMPARE(table.item(1, 0)->text(), QString("2"));
    table.sortItems(3, Qt::AscendingOrder);
    QCOMPARE(table.item(0, 3)->text(), QString("—"));
    QCOMPARE(table.item(2, 0)->text(), QString("2"));
    table.setColumnFilter(3, "5", "≥");
    QVERIFY(table.isRowHidden(0));
    QVERIFY(table.isRowHidden(1));
    QVERIFY(!table.isRowHidden(2));
  }

  void loginRevenueAndTrend() {
    widget<QLineEdit>("adminPassword")->setText("bad-password");
    widget<QPushButton>("adminLoginButton")->click();
    QTRY_COMPARE(widget<QLabel>("adminLoginError")->text(),
                 QString("账号或密码错误"));
    QVERIFY(login());
    QCOMPARE(widget<QTableWidget>("statusTable")->rowCount(), 2);
    QCOMPARE(widget<QTableWidget>("trendTable")->item(0, 1)->text(),
             QString("¥ 1.23"));
    auto *chart = widget<QChartView>("revenueChart")->chart();
    QCOMPARE(qobject_cast<QLineSeries *>(chart->series().first())->count(), 7);
    QPointer<QChart> previousChart(chart);
    QSignalSpy refreshed(widget<QTableWidget>("trendTable")->model(),
                         &QAbstractItemModel::modelReset);
    widget<QPushButton>("refreshCurrentPage")->click();
    QTRY_VERIFY(!refreshed.isEmpty());
    QVERIFY(previousChart);
    QCOMPARE(widget<QChartView>("revenueChart")->chart(), previousChart.data());
    widget<QComboBox>("revenueTrendDays")->setCurrentIndex(1);
    QTRY_COMPARE(widget<QTableWidget>("trendTable")->rowCount(), 30);
    QCOMPARE(fixture->lastParams["admin.overview"]["days"].toInt(), 30);
    QCOMPARE(fixture->unauthorized, 0);
  }

  void requestErrorsHaveOneSurface() {
    widget<QLineEdit>("adminPassword")->setText("bad-password");
    widget<QPushButton>("adminLoginButton")->click();
    QTRY_COMPARE(widget<QLabel>("adminLoginError")->text(),
                 QString("账号或密码错误"));
    QVERIFY(window->statusBar()->currentMessage().isEmpty());
    QVERIFY(window->findChildren<QMessageBox *>().isEmpty());
    QVERIFY(login());
    nav(1)->click();
    QTRY_COMPARE(widget<QTableWidget>("stationTable")->rowCount(), 1);
    widget<QPushButton>("addStationButton")->click();
    widget<QLineEdit>("stationName")->setText("新站");
    widget<QLineEdit>("stationAddress")->setText("校园路");
    widget<QLineEdit>("stationRegion")->setText("校园区");
    fixture->errors["admin.station.save"] = "电站名称已存在";
    widget<QPushButton>("saveStationButton")->click();
    QTRY_COMPARE(widget<QLabel>("stationError")->text(),
                 QString("电站名称已存在"));
    QVERIFY(widget<QPushButton>("saveStationButton")->isEnabled());
    QVERIFY(window->statusBar()->currentMessage().isEmpty());
    QVERIFY(window->findChildren<QMessageBox *>().isEmpty());
    window->findChild<QDialog *>()->reject();
    nav(5)->click();
    QTRY_COMPARE(widget<QTableWidget>("stationForecastTable")->rowCount(), 1);
    QTRY_COMPARE(fixture->requests["forecasts.status"], 1);
    fixture->errors["forecasts.run"] = "预测服务未就绪";
    widget<QPushButton>("runForecastButton")->click();
    QTRY_COMPARE(widget<QLabel>("forecastState")->text(),
                 QString("预测服务未就绪"));
    QVERIFY(widget<QPushButton>("runForecastButton")->isEnabled());
    QVERIFY(window->statusBar()->currentMessage().isEmpty());
    QVERIFY(window->findChildren<QMessageBox *>().isEmpty());
    fixture->errors["admin.users"] = "无法加载用户";
    nav(3)->click();
    QTRY_COMPARE(window->statusBar()->currentMessage(),
                 QString("无法加载用户"));
    QVERIFY(window->findChildren<QMessageBox *>().isEmpty());
    QVERIFY(window->statusBar()->findChildren<QLabel *>().isEmpty());
    window->statusBar()->clearMessage();
    fixture->server.close();
    widget<QPushButton>("refreshCurrentPage")->click();
    QTRY_VERIFY(!window->statusBar()->currentMessage().isEmpty());
    QVERIFY(window->findChildren<QMessageBox *>().isEmpty());
    QVERIFY(window->statusBar()->findChildren<QLabel *>().isEmpty());
  }

  void stationCreationAndSafeDialogClosure() {
    QVERIFY(login());
    nav(1)->click();
    QTRY_COMPARE(widget<QTableWidget>("stationTable")->rowCount(), 1);
    widget<QPushButton>("addStationButton")->click();
    QTRY_VERIFY(widget<QLineEdit>("stationName"));
    widget<QLineEdit>("stationName")->setText("新校园站");
    widget<QLineEdit>("stationAddress")->setText("校园路 1 号");
    widget<QLineEdit>("stationRegion")->setText("校园区");
    widget<QDoubleSpinBox>("stationPrice")->setValue(1.29);
    widget<QSpinBox>("stationChargerCount")->setValue(3);
    widget<QPushButton>("saveStationButton")->click();
    QTRY_COMPARE(widget<QTableWidget>("stationTable")->rowCount(), 2);
    QCOMPARE(fixture->lastParams["admin.station.save"]["priceCents"].toInt(),
             129);
    QCOMPARE(fixture->lastParams["admin.station.save"]["chargerCount"].toInt(),
             3);
    QVERIFY(!fixture->lastParams["admin.station.save"].contains("type"));
    QTRY_VERIFY(!window->findChild<QDialog *>());
    auto *stations = widget<QTableWidget>("stationTable");
    stations->selectRow(0);
    stations->cellDoubleClicked(0, 0);
    QPointer<QDialog> detail = window->findChild<QDialog *>();
    QVERIFY(detail);
    detail->reject();
    QTRY_VERIFY(detail.isNull());
    QTRY_COMPARE(fixture->requests["admin.chargers"], 1);
    QCOMPARE(fixture->unauthorized, 0);
  }

  void forecastIgnoresStatusFromBeforeRun() {
    QVERIFY(login());
    window->findChild<QCheckBox *>()->setChecked(false);
    fixture->holdNextAction = "forecasts.status";
    nav(5)->click();
    QTRY_VERIFY(fixture->heldSocket);
    auto *run = widget<QPushButton>("runForecastButton");
    run->click();
    QTRY_COMPARE(widget<QLabel>("forecastState")->text(),
                 QString("正在生成预测…"));
    fixture->releaseResponse();
    QTest::qWait(100);
    QVERIFY(!run->isEnabled());
    QCOMPARE(widget<QLabel>("forecastState")->text(), QString("正在生成预测…"));
    fixture->running = false;
    QTRY_VERIFY_WITH_TIMEOUT(fixture->requests["forecasts.status"] >= 2, 4500);
    QTRY_VERIFY(run->isEnabled());
    QVERIFY(widget<QLabel>("forecastState")->text().isEmpty());
  }

  void failedActionsRespectCurrentSelection() {
    QVERIFY(login());
    nav(2)->click();
    auto *chargers = widget<QTableWidget>("chargerTable");
    QTRY_COMPARE(chargers->rowCount(), 2);
    chargers->selectRow(0);
    fixture->errors["admin.charger.restart"] = "重启失败";
    fixture->holdNextAction = "admin.charger.restart";
    auto *restart = widget<QPushButton>("restartChargerButton");
    restart->click();
    QTRY_VERIFY(fixture->heldSocket);
    chargers->selectRow(1);
    chargers->selectRow(0);
    QVERIFY(!restart->isEnabled());
    chargers->selectRow(1);
    fixture->releaseResponse();
    QTRY_COMPARE(window->statusBar()->currentMessage(), QString("重启失败"));
    QVERIFY(!restart->isEnabled());

    nav(3)->click();
    auto *users = widget<QTableWidget>("userTable");
    QTRY_COMPARE(users->rowCount(), 1);
    users->selectRow(0);
    fixture->errors["admin.user.status"] = "冻结失败";
    fixture->holdNextAction = "admin.user.status";
    auto *freeze = widget<QPushButton>("freezeUserButton");
    freeze->click();
    QTRY_VERIFY(fixture->heldSocket);
    users->clearSelection();
    users->selectRow(0);
    QVERIFY(!freeze->isEnabled());
    users->clearSelection();
    fixture->releaseResponse();
    QTRY_COMPARE(window->statusBar()->currentMessage(), QString("冻结失败"));
    QVERIFY(!freeze->isEnabled());
    QVERIFY(!widget<QPushButton>("unfreezeUserButton")->isEnabled());
  }

  void stationDetailsIgnoreOlderRefresh() {
    QVERIFY(login());
    nav(1)->click();
    auto *stations = widget<QTableWidget>("stationTable");
    QTRY_COMPARE(stations->rowCount(), 1);
    stations->selectRow(0);
    stations->cellDoubleClicked(0, 0);
    auto *details = widget<QTableWidget>("stationChargerTable");
    QTRY_COMPARE(details->rowCount(), 2);
    fixture->holdNextAction = "admin.chargers";
    auto *refresh = widget<QPushButton>("refreshStationChargersButton");
    refresh->click();
    QTRY_VERIFY(fixture->heldSocket);
    auto charger = fixture->chargers[0].toObject();
    charger["status"] = "restarting";
    fixture->chargers[0] = charger;
    refresh->click();
    QTRY_COMPARE(details->item(0, 5)->text(), QString("重启中"));
    fixture->releaseResponse();
    QTest::qWait(100);
    QCOMPARE(details->item(0, 5)->text(), QString("重启中"));
    fixture->errors["admin.chargers"] = "较早的刷新失败";
    fixture->holdNextAction = "admin.chargers";
    refresh->click();
    QTRY_VERIFY(fixture->heldSocket);
    fixture->errors.remove("admin.chargers");
    refresh->click();
    QTRY_COMPARE(fixture->requests["admin.chargers"], 5);
    fixture->releaseResponse();
    QTest::qWait(100);
    QVERIFY(window->statusBar()->currentMessage().isEmpty());
  }

  void devicesAndAccounts() {
    QVERIFY(login());
    nav(2)->click();
    auto *chargers = widget<QTableWidget>("chargerTable");
    QTRY_COMPARE(chargers->rowCount(), 2);
    chargers->selectRow(1);
    QVERIFY(!widget<QPushButton>("restartChargerButton")->isEnabled());
    chargers->selectRow(0);
    QVERIFY(widget<QPushButton>("restartChargerButton")->isEnabled());
    widget<QPushButton>("restartChargerButton")->click();
    QTRY_COMPARE(chargers->item(0, 5)->text(), QString("重启中"));
    QCOMPARE(fixture->lastParams["admin.charger.restart"]["chargerId"].toInt(),
             1);
    static_cast<adminui::DataTable *>(chargers)->setColumnFilter(5, "充电中",
                                                                 "=");
    QVERIFY(chargers->isRowHidden(0));
    QVERIFY(!chargers->isRowHidden(1));
    QCOMPARE(chargers->item(1, 1)->text(), QString("AC002"));
    QVERIFY(!fixture->lastParams["admin.chargers"].contains("status"));
    nav(3)->click();
    auto *users = widget<QTableWidget>("userTable");
    QTRY_COMPARE(users->rowCount(), 1);
    QCOMPARE(users->item(0, 3)->text(), QString("¥ 361.09"));
    users->selectRow(0);
    widget<QPushButton>("freezeUserButton")->click();
    QTRY_COMPARE(users->item(0, 5)->text(), QString("冻结"));
    QVERIFY(widget<QPushButton>("unfreezeUserButton")->isEnabled());
    widget<QPushButton>("unfreezeUserButton")->click();
    QTRY_COMPARE(users->item(0, 5)->text(), QString("正常"));
    widget<QLineEdit>("userTableSearch")->setText("1234");
    QVERIFY(!users->isRowHidden(0));
    widget<QLineEdit>("userTableSearch")->setText("9999");
    QVERIFY(users->isRowHidden(0));
    QVERIFY(!fixture->lastParams["admin.users"].contains("query"));
    QCOMPARE(fixture->unauthorized, 0);
  }

  void forecastAndLogs() {
    QVERIFY(login());
    nav(5)->click();
    auto *stations = widget<QTableWidget>("stationForecastTable");
    auto *chargers = widget<QTableWidget>("chargerForecastTable");
    QTRY_COMPARE(stations->rowCount(), 1);
    QCOMPARE(stations->item(0, 1)->text(), QString("1.50"));
    QCOMPARE(stations->item(0, 3)->text(), QString("9.00"));
    QCOMPARE(stations->item(0, 5)->text(), QString("36.00"));
    QCOMPARE(chargers->rowCount(), 1);
    QCOMPARE(chargers->item(0, 4)->text(), QString("36.00"));
    const int previous = fixture->requests["forecasts.list"];
    widget<QPushButton>("runForecastButton")->click();
    QTRY_VERIFY(fixture->running);
    QVERIFY(!widget<QPushButton>("runForecastButton")->isEnabled());
    fixture->running = false;
    QTRY_VERIFY_WITH_TIMEOUT(
      widget<QPushButton>("runForecastButton")->isEnabled(), 5000);
    QTRY_VERIFY(fixture->requests["forecasts.list"] > previous);
    nav(6)->click();
    QTRY_COMPARE(widget<QTableWidget>("logsTable")->rowCount(), 1);
    QCOMPARE(widget<QTableWidget>("logsTable")->item(0, 1)->text(),
             QString("远程重启"));
    QCOMPARE(fixture->unauthorized, 0);
  }

  void delayedResponseAfterLogoutIsIgnored() {
    QVERIFY(login());
    fixture->holdUsers = true;
    nav(3)->click();
    QTRY_COMPARE(fixture->heldUsers.size(), 1);
    widget<QPushButton>("adminLogoutButton")->click();
    QTRY_VERIFY(widget<QPushButton>("adminLoginButton")->isVisible());
    fixture->releaseUsers();
    QTest::qWait(150);
    QCOMPARE(widget<QTableWidget>("userTable")->rowCount(), 0);
    QCOMPARE(widget<QLineEdit>("adminPassword")->text(), QString());
    QTRY_COMPARE(fixture->requests["auth.logout"], 1);
    QCOMPARE(fixture->unauthorized, 0);
  }
};

QTEST_MAIN(AdminUiTest)
#include "AdminUiTest.moc"
