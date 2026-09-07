#pragma once

#include "AdminMainWindow.h"

#include <QHash>
#include <QJsonObject>
#include <functional>

class ApiClient;
class QChartView;
class QCheckBox;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
namespace adminui {
class DataTable;
}
class QTimer;
class QVBoxLayout;

class AdminMainWindow::Impl {
public:
  explicit Impl(AdminMainWindow *window);
  void call(QObject *owner, const QString &action, const QJsonObject &params,
            const std::function<void(QJsonValue)> &success,
            const std::function<void(QString)> &failure = {});
  void read(const QString &action, const QJsonObject &params,
            const std::function<void(QJsonValue)> &success);
  void buildLogin();
  void buildWorkspace();
  QVBoxLayout *page(const QString &title, QHBoxLayout *header = nullptr);
  QLabel *metric(const QString &title, QHBoxLayout *row);
  void buildOverview();
  void refreshOverview();
  void buildStations();
  void refreshStations();
  void stationEditor(const QJsonObject &existing);
  void chargerActions(adminui::DataTable *target, QHBoxLayout *toolbar,
                      QObject *owner, const std::function<void()> &refresh);
  void stationDetail(const QJsonObject &station);
  void buildChargers();
  void refreshChargers();
  void buildUsers();
  void refreshUsers();
  void userOrders(const QJsonObject &user);
  void buildOrders();
  void refreshOrders();
  void buildForecasts();
  void refreshForecasts();
  void refreshForecastStatus();
  void buildLogs();
  void refreshLogs();
  void refreshCurrent();
  void logoutNow();

  AdminMainWindow *w;
  ApiClient *api;
  QStackedWidget *central = nullptr;
  QStackedWidget *pages = nullptr;
  QLineEdit *serverUrl = nullptr;
  QLineEdit *username = nullptr;
  QLineEdit *password = nullptr;
  QPushButton *loginButton = nullptr;
  QLabel *loginError = nullptr;
  QLabel *identity = nullptr;
  QLabel *updatedAt = nullptr;
  QCheckBox *autoRefresh = nullptr;
  QTimer *poll = nullptr;
  QTimer *forecastPoll = nullptr;
  int session = 0;
  bool loggedIn = false;
  QComboBox *trendDays = nullptr;
  QLabel *todayRevenue = nullptr;
  QLabel *monthRevenue = nullptr;
  QLabel *totalRevenue = nullptr;
  QLabel *todayOrders = nullptr;
  QChartView *revenueChart = nullptr;
  adminui::DataTable *statusTable = nullptr;
  adminui::DataTable *trendTable = nullptr;
  adminui::DataTable *stationTable = nullptr;
  adminui::DataTable *chargerTable = nullptr;
  adminui::DataTable *userTable = nullptr;
  adminui::DataTable *ordersTable = nullptr;
  adminui::DataTable *logsTable = nullptr;
  QComboBox *forecastStation = nullptr;
  adminui::DataTable *stationForecasts = nullptr;
  adminui::DataTable *chargerForecasts = nullptr;
  QLabel *forecastMeta = nullptr;
  QLabel *forecastState = nullptr;
  QLabel *forecastWarnings = nullptr;
  QPushButton *runForecast = nullptr;
  bool forecastRunning = false;
  bool forecastRequestPending = false;
  QHash<QString, int> revisions;
};
