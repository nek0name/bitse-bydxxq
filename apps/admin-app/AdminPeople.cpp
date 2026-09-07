#include "AdminUi.h"
#include "AdminWindowState.h"

#include <QDialog>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <memory>

using namespace adminui;

void AdminMainWindow::Impl::buildUsers() {
  auto *row = new QHBoxLayout;
  auto *layout = page("用户管理", row);
  auto *freeze = button("冻结账号", row);
  freeze->setObjectName("freezeUserButton");
  auto *unfreeze = button("解冻账号", row);
  unfreeze->setObjectName("unfreezeUserButton");
  auto *orders = button("查看用户订单", row);
  userTable = new DataTable(
    {"用户 ID", "手机号", "昵称", "钱包余额 (元)", "注册时间", "状态"},
    "userTable");
  userTable->setColumnWidth(4, 190);
  layout->addWidget(userTable->panel(row), 1);
  const auto pendingSession = std::make_shared<int>(-1);
  auto update = [this, freeze, unfreeze, orders, pendingSession] {
    const auto item = selected(userTable);
    freeze->setEnabled(*pendingSession != session && !item.isEmpty()
                       && item["status"].toString() == "active");
    unfreeze->setEnabled(*pendingSession != session && !item.isEmpty()
                         && item["status"].toString() == "frozen");
    orders->setEnabled(!item.isEmpty());
  };
  QObject::connect(userTable, &QTableWidget::itemSelectionChanged, w, update);
  update();
  for (auto entry : {qMakePair(freeze, QString("frozen")),
                     qMakePair(unfreeze, QString("active"))}) {
    QObject::connect(
      entry.first, &QPushButton::clicked, w,
      [this, entry, pendingSession, update] {
        const auto user = selected(userTable);
        if (user.isEmpty()) return;
        *pendingSession = session;
        update();
        call(
          w, "admin.user.status",
          {{"userId", user["id"]}, {"status", entry.second}},
          [this, pendingSession, update](QJsonValue) {
            *pendingSession = -1;
            update();
            w->statusBar()->showMessage("用户账号状态已更新", 6000);
            refreshUsers();
          },
          [this, pendingSession, update](const QString &message) {
            *pendingSession = -1;
            update();
            w->statusBar()->showMessage(message, 12000);
          });
      });
  }
  auto showOrders = [this] {
    const auto user = selected(userTable);
    if (!user.isEmpty()) userOrders(user);
  };
  QObject::connect(orders, &QPushButton::clicked, w, showOrders);
  QObject::connect(userTable, &QTableWidget::cellDoubleClicked, w, showOrders);
}

void AdminMainWindow::Impl::refreshUsers() {
  read("admin.users", {}, [this](QJsonValue data) {
    userTable->fill(data.toArray(), [](const QJsonObject &o) -> QVariantList {
      return {o["id"].toInt(),          o["phone"].toString(),
              o["nickname"].toString(), moneyCell(o["balanceCents"]),
              timeText(o["createdAt"]), state(o["status"].toString())};
    });
  });
}

void AdminMainWindow::Impl::userOrders(const QJsonObject &user) {
  auto *dialog = new QDialog(w);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowTitle(user["phone"].toString() + " · 用户订单");
  dialog->resize(1150, 560);
  auto *layout = new QVBoxLayout(dialog);
  auto *row = new QHBoxLayout;
  row->addWidget(heading(user["nickname"].toString()));
  row->addSpacing(16);
  auto *refreshButton = button("刷新订单", row);
  auto *details = new DataTable(orderHeaders(), "userOrdersTable");
  details->sortItems(0, Qt::DescendingOrder);
  layout->addWidget(details->panel(row), 1);
  const auto revision = std::make_shared<int>(0);
  auto refresh = [this, dialog, user, details, revision] {
    const int request = ++*revision;
    call(
      dialog, "admin.orders", {{"userId", user["id"]}},
      [details, revision, request](QJsonValue data) {
        if (request != *revision) return;
        details->fill(data.toArray(), orderColumns);
      },
      [this, revision, request](const QString &message) {
        if (request == *revision) w->statusBar()->showMessage(message, 12000);
      });
  };
  QObject::connect(refreshButton, &QPushButton::clicked, dialog, refresh);
  refresh();
  dialog->open();
}

void AdminMainWindow::Impl::buildOrders() {
  auto *row = new QHBoxLayout;
  auto *layout = page("全平台订单记录", row);
  ordersTable = new DataTable(orderHeaders(), "ordersTable");
  ordersTable->sortItems(0, Qt::DescendingOrder);
  ordersTable->setColumnWidth(1, 220);
  for (int column : {6, 7, 8}) ordersTable->setColumnWidth(column, 190);
  layout->addWidget(ordersTable->panel(row), 1);
}

void AdminMainWindow::Impl::refreshOrders() {
  read("admin.orders", {}, [this](QJsonValue data) {
    ordersTable->fill(data.toArray(), orderColumns);
  });
}

void AdminMainWindow::Impl::buildLogs() {
  auto *row = new QHBoxLayout;
  auto *layout = page("运维操作日志", row);
  logsTable = new DataTable({"日志 ID", "操作", "操作对象", "详情", "时间"},
                            "logsTable");
  logsTable->sortItems(0, Qt::DescendingOrder);
  logsTable->setColumnWidth(1, 180);
  logsTable->setColumnWidth(3, 380);
  logsTable->setColumnWidth(4, 190);
  layout->addWidget(logsTable->panel(row), 1);
}

void AdminMainWindow::Impl::refreshLogs() {
  read("admin.logs", {}, [this](QJsonValue data) {
    logsTable->fill(
      data.toArray(), [](const QJsonObject &item) -> QVariantList {
        static const QMap<QString, QString> actions{
          {"admin.station.save", "保存电站"},
          {"admin.charger.restart", "远程重启"},
          {"admin.charger.status", "更改电桩状态"},
          {"admin.user.status", "更改用户状态"},
          {"forecasts.run", "运行负荷预测"},
          {"admin.login", "管理员登录"}};
        const auto action = item["action"].toString();
        return {item["id"].toInt(), actions.value(action, action),
                item["target"].toVariant().toString(),
                item["detail"].toString(), timeText(item["createdAt"])};
      });
  });
}
