#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <functional>

class ApiClient;
class QTimer;
class QJsonValue;

class OrderListModel final : public QAbstractListModel {
public:
  using QAbstractListModel::QAbstractListModel;
  int rowCount(const QModelIndex &parent = {}) const override {
    return parent.isValid() ? 0 : m_items.size();
  }
  QVariant data(const QModelIndex &index, int role) const override {
    return index.isValid() && role == Qt::UserRole ? m_items.value(index.row())
                                                   : QVariant();
  }
  QHash<int, QByteArray> roleNames() const override {
    return {{Qt::UserRole, "order"}};
  }
  void replace(const QVariantList &items) {
    if (m_items == items) return;
    beginResetModel();
    m_items = items;
    endResetModel();
  }
  void append(const QVariantList &items) {
    if (items.isEmpty()) return;
    beginInsertRows({}, m_items.size(), m_items.size() + items.size() - 1);
    m_items.append(items);
    endInsertRows();
  }

private:
  QVariantList m_items;
};

class MobileController final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString page READ page NOTIFY pageChanged)
  Q_PROPERTY(QString tab READ tab NOTIFY tabChanged)
  Q_PROPERTY(bool signedIn READ signedIn NOTIFY userChanged)
  Q_PROPERTY(QVariantMap user READ user NOTIFY userChanged)
  Q_PROPERTY(QVariantList stations READ stations NOTIFY stationsChanged)
  Q_PROPERTY(QVariantMap station READ station NOTIFY stationChanged)
  Q_PROPERTY(QVariantList chargers READ chargers NOTIFY stationChanged)
  Q_PROPERTY(QAbstractItemModel *orders READ orders CONSTANT)
  Q_PROPERTY(bool loadingOrders READ loadingOrders NOTIFY ordersChanged)
  Q_PROPERTY(bool refreshingOrders READ refreshingOrders NOTIFY ordersChanged)
  Q_PROPERTY(bool hasMoreOrders READ hasMoreOrders NOTIFY ordersChanged)
  Q_PROPERTY(QString orderFilter READ orderFilter NOTIFY ordersChanged)
  Q_PROPERTY(QVariantMap activeOrder READ activeOrder NOTIFY activeOrderChanged)
  Q_PROPERTY(QVariantMap viewedOrder READ viewedOrder NOTIFY viewedOrderChanged)
  Q_PROPERTY(QVariantList presets READ presets NOTIFY locationChanged)
  Q_PROPERTY(QString locationName READ locationName NOTIFY locationChanged)
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY filtersChanged)
  Q_PROPERTY(QString sort READ sort WRITE setSort NOTIFY filtersChanged)
  Q_PROPERTY(
    bool fastOnly READ fastOnly WRITE setFastOnly NOTIFY filtersChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(
    bool loadingStations READ loadingStations NOTIFY loadingStationsChanged)
  Q_PROPERTY(QString error READ error NOTIFY errorChanged)
  Q_PROPERTY(QString avatarSource READ avatarSource NOTIFY userChanged)
  Q_PROPERTY(QString reservationRemaining READ reservationRemaining NOTIFY
               reservationRemainingChanged)

public:
  explicit MobileController(QObject *parent = nullptr);
  QString page() const { return m_page; }
  QString tab() const { return m_tab; }
  bool signedIn() const { return !m_user.isEmpty(); }
  QVariantMap user() const { return m_user; }
  QVariantList stations() const { return m_stations; }
  QVariantMap station() const { return m_station; }
  QVariantList chargers() const { return m_chargers; }
  QAbstractItemModel *orders() { return &m_orders; }
  bool loadingOrders() const { return m_loadingOrders; }
  bool refreshingOrders() const {
    return m_loadingOrders && !m_appendingOrders;
  }
  bool hasMoreOrders() const { return m_nextOrderCursor > 0; }
  QString orderFilter() const { return m_orderFilter; }
  QVariantMap activeOrder() const { return m_activeOrder; }
  QVariantMap viewedOrder() const { return m_viewedOrder; }
  QVariantList presets() const { return m_presets; }
  QString locationName() const { return m_locationName; }
  QString query() const { return m_query; }
  QString sort() const { return m_sort; }
  bool fastOnly() const { return m_fastOnly; }
  bool busy() const { return m_pending > 0; }
  bool loadingStations() const { return m_loadingStations; }
  QString error() const { return m_error; }
  QString avatarSource() const;
  QString reservationRemaining() const;

  void setQuery(const QString &value);
  void setSort(const QString &value);
  void setFastOnly(bool value);
  Q_INVOKABLE void initialize();
  Q_INVOKABLE void login(const QString &phone);
  Q_INVOKABLE void logout();
  Q_INVOKABLE void selectTab(const QString &tab);
  Q_INVOKABLE void navigate(const QString &page);
  Q_INVOKABLE void back();
  Q_INVOKABLE void clearError();
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void refreshStations();
  Q_INVOKABLE void loadMoreOrders();
  Q_INVOKABLE qreal orderScrollPosition() const {
    return m_orderScrollPosition;
  }
  Q_INVOKABLE void saveOrderScroll(qreal position) {
    m_orderScrollPosition = position;
  }
  Q_INVOKABLE void filterOrders(const QString &filter);
  Q_INVOKABLE void chooseLocation(int index);
  Q_INVOKABLE void geocode(const QString &address);
  Q_INVOKABLE void openStation(int stationId);
  Q_INVOKABLE void reserve(int chargerId);
  Q_INVOKABLE void openActiveOrder();
  Q_INVOKABLE void startCharging();
  Q_INVOKABLE void cancelReservation();
  Q_INVOKABLE void stopCharging();
  Q_INVOKABLE void settle();
  Q_INVOKABLE void openOrder(int orderId);
  Q_INVOKABLE void recharge(const QString &amount);
  Q_INVOKABLE void updateNickname(const QString &nickname);
  Q_INVOKABLE void chooseAvatar();
  Q_INVOKABLE void openNavigation(const QVariantMap &station);
  Q_INVOKABLE QString statusLabel(const QString &status) const;
  Q_INVOKABLE QString formatTime(const QString &value) const;

signals:
  void pageChanged();
  void tabChanged();
  void userChanged();
  void stationsChanged();
  void loadingStationsChanged();
  void stationChanged();
  void ordersChanged();
  void activeOrderChanged();
  void viewedOrderChanged();
  void locationChanged();
  void filtersChanged();
  void busyChanged();
  void errorChanged();
  void reservationRemainingChanged();
  void notification(const QString &message);
  void unfinishedOrder(const QString &message);
  void navigationRequested(const QVariantMap &destination,
                           const QString &originName, double latitude,
                           double longitude);

private:
  using Success = std::function<void(const QJsonValue &)>;
  void call(const QString &action, const QVariantMap &params, Success success,
            bool foreground = true);
  void setPage(const QString &page, bool push = false);
  void setError(const QString &message, const QString &action = {});
  void setUser(const QVariantMap &user);
  void setActiveOrder(const QVariantMap &order);
  void fetchActive(bool recover = false, std::function<void()> empty = {});
  void fetchOrders(bool append = false);
  void fetchStation(int stationId, bool foreground);
  void orderAction(const QString &action);
  void refreshProfile();
  QVariantMap locationParams() const;

  ApiClient *m_api;
  QTimer *m_pollTimer;
  QString m_page = "login";
  QString m_tab = "home";
  QStringList m_backStack;
  QVariantMap m_user;
  QVariantList m_stations;
  QVariantMap m_station;
  QVariantList m_chargers;
  OrderListModel m_orders;
  QString m_orderFilter = "all";
  bool m_loadingOrders = false;
  bool m_appendingOrders = false;
  bool m_ordersLoaded = false;
  qreal m_orderScrollPosition = 0;
  int m_nextOrderCursor = 0;
  int m_ordersRequest = 0;
  QVariantMap m_activeOrder;
  QVariantMap m_viewedOrder;
  QVariantList m_presets;
  QString m_locationName = "选择当前位置";
  double m_latitude = 0;
  double m_longitude = 0;
  bool m_hasLocation = false;
  QString m_query;
  QString m_sort = "distance";
  bool m_fastOnly = false;
  bool m_loadingStations = false;
  bool m_pollInFlight = false;
  QString m_error;
  QString m_errorAction;
  int m_pending = 0;
  int m_session = 0;
  int m_stationRequest = 0;
  int m_selectedStation = 0;
  int m_pollCount = 0;
  int m_orderRevision = 0;
  QString m_rechargeKey;
  qint64 m_rechargeAmount = 0;
  QString m_reservationKey;
  int m_reservationCharger = 0;
};
