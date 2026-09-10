#include "MobileController.h"
#include "ApiClient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QFile>
#include <QUrlQuery>
#include <cmath>
#ifndef Q_OS_ANDROID
#include <QFileDialog>
#endif
#include <QGuiApplication>
#include <QHash>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QUuid>

MobileController::MobileController(QObject *parent)
    : QObject(parent), m_api(new ApiClient(this)),
      m_pollTimer(new QTimer(this)) {
  connect(m_api, &ApiClient::authenticationFailed, this,
          [this](const QString &) {
            expireSession();
          });
  auto *clock = new QTimer(this);
  clock->setInterval(1000);
  connect(clock, &QTimer::timeout, this, [this] {
    if (m_activeOrder.value("status") == "reserved")
      emit reservationRemainingChanged();
  });
  clock->start();
  m_pollTimer->setInterval(2000);
  connect(m_pollTimer, &QTimer::timeout, this, [this] {
    if (!signedIn() || busy() || m_pollInFlight
        || QGuiApplication::applicationState() != Qt::ApplicationActive)
      return;
    fetchActive();
    if (++m_pollCount % 6 == 0) {
      if (m_page == "home") refreshStations();
      if (m_page == "station" && m_selectedStation)
        fetchStation(m_selectedStation, false);
      refreshProfile();
    }
  });
}

void MobileController::call(const QString &action, const QVariantMap &params,
                            Success success, bool foreground) {
  const int session = m_session;
  if (foreground) {
    clearError();
    ++m_pending;
    emit busyChanged();
  }
  m_api->call(
    action, QJsonObject::fromVariantMap(params),
    [this, session, success, foreground, action](QJsonValue value) {
      if (foreground) {
        --m_pending;
        emit busyChanged();
      }
      if (session != m_session) return;
      if (m_errorAction == action) clearError();
      success(value);
    },
    [this, session, foreground, action](QString message) {
      if (foreground) {
        --m_pending;
        emit busyChanged();
      }
      if (session != m_session) return;
      if (action == "orders.active") m_pollInFlight = false;
      setError(message, action);
    });
}

void MobileController::initialize() {
  call(
    "location.presets", {},
    [this](const QJsonValue &value) {
      m_presets = value.toVariant().toList();
      emit locationChanged();
      if (!nativePlatform() && !m_presets.isEmpty())
        chooseLocation(0);
      else
        refreshStations();
    },
    false);
}

QString MobileController::sessionGroup() const {
  return QString::fromLatin1(
    QCryptographicHash::hash(m_api->baseUrl().toUtf8(),
                             QCryptographicHash::Sha256)
      .toHex());
}

void MobileController::saveSession() {
  if (!m_persistSession || !signedIn() || m_api->token().isEmpty()) return;
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     "ChargingPlatform", "MobileSession");
  settings.beginGroup(sessionGroup());
  settings.setValue("token", m_api->token());
  settings.setValue("user", QJsonDocument(QJsonObject::fromVariantMap(m_user))
                              .toJson(QJsonDocument::Compact));
  settings.sync();
  QFile::setPermissions(settings.fileName(),
                        QFile::ReadOwner | QFile::WriteOwner);
}

void MobileController::clearSavedSession() {
  if (!m_persistSession) return;
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     "ChargingPlatform", "MobileSession");
  settings.remove(sessionGroup());
  settings.sync();
}

void MobileController::restoreSession() {
  if (m_persistSession) return;
  m_persistSession = true;
  QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                     "ChargingPlatform", "MobileSession");
  settings.beginGroup(sessionGroup());
  const auto token = settings.value("token").toString();
  const auto user = QJsonDocument::fromJson(
                      settings.value("user").toByteArray())
                      .object()
                      .toVariantMap();
  if (token.isEmpty() || user.isEmpty()) return;
  m_api->setToken(token);
  setUser(user);
  setPage("home");
  m_pollTimer->start();
  // A network failure keeps the local session for retry. Only an explicit
  // authentication failure revokes it; the server remains authoritative.
  call("user.me", {}, [this](QJsonValue value) {
    setUser(value.toObject().toVariantMap());
    fetchActive(true);
  });
}

void MobileController::login(const QString &phone) {
  performLogin(phone, false);
}

void MobileController::confirmRegistration(const QString &phone) {
  performLogin(phone, true);
}

void MobileController::performLogin(const QString &phone,
                                    bool allowRegistration) {
  if (busy()) return;
  const QString number = phone.trimmed();
  if (!QRegularExpression("^1[0-9]{10}$").match(number).hasMatch()) {
    setError("请输入有效的 11 位手机号");
    return;
  }
  call("user.login",
       {{"phone", number}, {"allowRegistration", allowRegistration}},
       [this, number](const QJsonValue &value) {
         const auto result = value.toObject();
         if (result.value("registrationRequired").toBool()) {
           emit registrationRequested(number);
           return;
         }
         setUser(result.value("user").toObject().toVariantMap());
         m_tab = "home";
         emit tabChanged();
         m_backStack.clear();
         setPage("home");
         if (m_presets.isEmpty())
           initialize();
         else
           refreshStations();
         fetchActive(true);
         m_pollTimer->start();
       });
}

void MobileController::logout() {
  if (busy()) return;
  clearSavedSession();
  // Send revocation with the current token, then clear the local session even
  // if connectivity has been lost. An old reply cannot clear a newer login.
  m_api->call(
    "auth.logout", {}, [](QJsonValue) {}, [](QString) {});
  m_api->setToken({});
  ++m_session;
  ++m_orderRevision;
  m_pollTimer->stop();
  m_pollInFlight = false;
  m_loadingStations = false;
  emit loadingStationsChanged();
  m_user.clear();
  m_orders.replace({});
  ++m_ordersRequest;
  m_loadingOrders = false;
  m_nextOrderCursor = 0;
  m_orderFilter = "all";
  m_ordersLoaded = false;
  m_orderScrollPosition = 0;
  m_activeOrder.clear();
  m_viewedOrder.clear();
  m_rechargeKey.clear();
  m_reservationKey.clear();
  m_backStack.clear();
  emit userChanged();
  emit ordersChanged();
  emit activeOrderChanged();
  emit reservationRemainingChanged();
  emit viewedOrderChanged();
  setPage("login");
}

void MobileController::setPage(const QString &page, bool push) {
  if (m_page == page) return;
  m_transitionDirection = m_returning ? -1 : (push ? 1 : 0);
  if (push) m_backStack.append(m_page);
  m_page = page;
  clearError();
  emit pageChanged();
}

void MobileController::selectTab(const QString &tab) {
  if (!signedIn() || busy() || m_page == tab) return;
  if (m_tab != tab) {
    m_tab = tab;
    emit tabChanged();
  }
  m_backStack.clear();
  setPage(tab);
  if (tab == "home") refreshStations();
  if (tab == "orders" && !m_ordersLoaded && !m_loadingOrders) fetchOrders();
  if (tab == "profile") refreshProfile();
}

void MobileController::navigate(const QString &page) {
  if (busy()) return;
  setPage(page, true);
}

void MobileController::back() {
  if (busy()) return;
  m_returning = true;
  if (!m_backStack.isEmpty())
    setPage(m_backStack.takeLast());
  else
    setPage(m_tab);
  m_returning = false;
}

bool MobileController::handleSystemBack() {
  if (busy()) return true;
  if (!signedIn()) return false;
  if (!m_backStack.isEmpty() || m_page != m_tab) {
    back();
    return true;
  }
  if (m_tab != "home") {
    selectTab("home");
    return true;
  }
  return false;
}

void MobileController::setError(const QString &message, const QString &action) {
  m_errorAction = action;
  if (m_error == message) return;
  m_error = message;
  emit errorChanged();
}

void MobileController::clearError() {
  if (m_error.isEmpty()) return;
  m_error.clear();
  m_errorAction.clear();
  emit errorChanged();
}

void MobileController::expireSession() {
  clearSavedSession();
  if (!signedIn()) return;
  m_api->setToken({});
  ++m_session;
  ++m_orderRevision;
  m_pollTimer->stop();
  m_pollInFlight = false;
  m_loadingStations = false;
  emit loadingStationsChanged();
  m_user.clear();
  m_orders.replace({});
  ++m_ordersRequest;
  m_loadingOrders = false;
  m_nextOrderCursor = 0;
  m_orderFilter = "all";
  m_ordersLoaded = false;
  m_orderScrollPosition = 0;
  m_activeOrder.clear();
  m_viewedOrder.clear();
  m_rechargeKey.clear();
  m_reservationKey.clear();
  m_backStack.clear();
  emit userChanged();
  emit ordersChanged();
  emit activeOrderChanged();
  emit reservationRemainingChanged();
  emit viewedOrderChanged();
  setPage("login");
  setError("登录已失效，请重新登录");
}

void MobileController::setUser(const QVariantMap &user) {
  if (m_user == user) return;
  m_user = user;
  saveSession();
  emit userChanged();
}

void MobileController::setActiveOrder(const QVariantMap &order) {
  if (m_activeOrder == order) return;
  const QString previousStatus = m_activeOrder.value("status").toString();
  if (previousStatus != order.value("status").toString()
      || m_activeOrder.value("id") != order.value("id"))
    m_ordersLoaded = false;
  m_activeOrder = order;
  emit activeOrderChanged();
  emit reservationRemainingChanged();
  if (!order.isEmpty() && m_viewedOrder.value("id") == order.value("id")) {
    m_viewedOrder = order;
    emit viewedOrderChanged();
  }
  if (previousStatus == "charging"
      && order.value("status") == "pending_payment") {
    if (m_page == "charge")
      setPage("settlement");
    else if (m_page != "settlement")
      emit notification("充电已结束，请结算");
  }
}

void MobileController::setQuery(const QString &value) {
  if (m_query == value) return;
  m_query = value;
  emit filtersChanged();
}

void MobileController::setSort(const QString &value) {
  if (m_sort == value) return;
  m_sort = value;
  emit filtersChanged();
  refreshStations();
}

void MobileController::setFastOnly(bool value) {
  if (m_fastOnly == value) return;
  m_fastOnly = value;
  emit filtersChanged();
  refreshStations();
}

QVariantMap MobileController::locationParams() const {
  if (!m_hasLocation) return {};
  return {{"latitude", m_latitude}, {"longitude", m_longitude}};
}

void MobileController::refreshLocation() {
  if (m_locating) return;
  if (!nativePlatform()) {
    navigate("location");
    return;
  }
  m_locating = true;
  emit locationChanged();
  emit systemLocationRequested();
}

void MobileController::openLocationPicker() {
  if (!nativePlatform()) {
    navigate("location");
    return;
  }
  m_locating = false;
  emit locationChanged();
  emit locationPickerRequested(m_latitude, m_longitude, m_hasLocation);
}

void MobileController::applySystemLocation(double latitude, double longitude,
                                           const QString &name) {
  m_locating = false;
  if (!std::isfinite(latitude) || !std::isfinite(longitude)
      || std::abs(latitude) > 90 || std::abs(longitude) > 180) {
    locationFailed("定位返回了无效坐标，请重试");
    return;
  }
  m_latitude = latitude;
  m_longitude = longitude;
  m_hasLocation = true;
  m_locationName = name;
  clearError();
  emit locationChanged();
  refreshStations();
}

void MobileController::locationFailed(const QString &message) {
  m_locating = false;
  emit locationChanged();
  emit notification(message);
}

void MobileController::uploadAvatar(const QString &base64) {
  if (busy()) return;
  const auto bytes = QByteArray::fromBase64(base64.toLatin1());
  if (bytes.isEmpty() || bytes.size() > 2 * 1024 * 1024
      || QImage::fromData(bytes).isNull()) {
    setError("头像图片无效或超过 2 MB，请重新选择");
    return;
  }
  call("user.update", {{"avatarBase64", base64}},
       [this](const QJsonValue &value) {
         setUser(value.toObject().toVariantMap());
         emit notification("头像已更新");
       });
}

void MobileController::chooseLocation(int index) {
  if (index < 0 || index >= m_presets.size()) return;
  const auto preset = m_presets.at(index).toMap();
  m_latitude = preset.value("latitude").toDouble();
  m_longitude = preset.value("longitude").toDouble();
  m_locationName = preset.value("name").toString();
  m_hasLocation = true;
  emit locationChanged();
  refreshStations();
}

void MobileController::geocode(const QString &address) {
  if (busy()) return;
  if (address.trimmed().size() < 2) {
    setError("请输入完整地址，例如上海市人民广场");
    return;
  }
  call("location.geocode", {{"address", address.trimmed()}},
       [this](const QJsonValue &value) {
         const auto result = value.toObject();
         m_latitude = result.value("latitude").toDouble();
         m_longitude = result.value("longitude").toDouble();
         m_locationName = result.value("name").toString();
         m_hasLocation = true;
         emit locationChanged();
         back();
         refreshStations();
       });
}

void MobileController::refreshStations() {
  auto params = locationParams();
  params.insert("query", m_query.trimmed());
  params.insert("sort", m_sort);
  params.insert("fastOnly", m_fastOnly);
  const int request = ++m_stationRequest;
  const int session = m_session;
  m_loadingStations = true;
  emit loadingStationsChanged();
  m_api->call(
    "stations.list", QJsonObject::fromVariantMap(params),
    [this, request, session](const QJsonValue &value) {
      if (request != m_stationRequest || session != m_session) return;
      if (m_errorAction == "stations.list") clearError();
      const auto stations = value.toVariant().toList();
      if (m_stations != stations) {
        m_stations = stations;
        emit stationsChanged();
      }
      m_loadingStations = false;
      emit loadingStationsChanged();
    },
    [this, request, session](QString message) {
      if (request != m_stationRequest || session != m_session) return;
      m_loadingStations = false;
      setError(message, "stations.list");
      emit loadingStationsChanged();
    });
}

void MobileController::fetchStation(int stationId, bool foreground) {
  auto params = locationParams();
  params.insert("stationId", stationId);
  call(
    "stations.detail", params,
    [this, stationId, foreground](const QJsonValue &value) {
      if (m_selectedStation != stationId) return;
      const auto result = value.toObject();
      const auto station = result.value("station").toObject().toVariantMap();
      const auto chargers = result.value("chargers").toVariant().toList();
      if (station != m_station || chargers != m_chargers) {
        m_station = station;
        m_chargers = chargers;
        emit stationChanged();
      }
      if (foreground && m_page == "home") setPage("station", true);
    },
    foreground);
}

void MobileController::openStation(int stationId) {
  if (busy()) return;
  m_selectedStation = stationId;
  m_station.clear();
  m_chargers.clear();
  emit stationChanged();
  fetchStation(stationId, true);
}

void MobileController::fetchActive(bool recover, std::function<void()> empty) {
  if (m_pollInFlight && !recover) return;
  if (recover) ++m_orderRevision;
  const int revision = m_orderRevision;
  m_pollInFlight = true;
  call(
    "orders.active", {},
    [this, recover, empty, revision](const QJsonValue &value) {
      m_pollInFlight = false;
      if (revision != m_orderRevision) return;
      const auto order = value.toObject().toVariantMap();
      const int oldId = m_activeOrder.value("id").toInt();
      const bool displayingActive = m_page == "charge"
                                 || m_page == "settlement";
      setActiveOrder(order);
      if (order.isEmpty()) {
        if (oldId && displayingActive) {
          call(
            "orders.get", {{"orderId", oldId}},
            [this](const QJsonValue &finished) {
              m_viewedOrder = finished.toObject().toVariantMap();
              emit viewedOrderChanged();
              setPage("receipt");
            },
            false);
        }
        if (empty) empty();
        return;
      }
      if (recover) {
        openActiveOrder();
        const auto status = order.value("status").toString();
        if (status == "charging" || status == "pending_payment")
          emit unfinishedOrder("有未完成的充电订单，请先结算");
      }
    },
    recover);
}

void MobileController::reserve(int chargerId) {
  if (busy()) return;
  fetchActive(true, [this, chargerId] {
    if (m_user.value("balanceCents").toLongLong() <= 0) {
      emit notification("余额不足，请先充值");
      setPage("recharge", true);
      return;
    }
    if (m_reservationKey.isEmpty() || m_reservationCharger != chargerId) {
      m_reservationCharger = chargerId;
      m_reservationKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    call("orders.reserve",
         {{"chargerId", chargerId}, {"idempotencyKey", m_reservationKey}},
         [this](const QJsonValue &value) {
           setActiveOrder(value.toObject().toVariantMap());
           m_reservationKey.clear();
           setPage("charge", true);
           refreshStations();
         });
  });
}

void MobileController::openActiveOrder() {
  if (m_activeOrder.isEmpty()) return;
  const QString status = m_activeOrder.value("status").toString();
  setPage(status == "reserved" ? "charge" : "settlement", true);
}

void MobileController::orderAction(const QString &action) {
  if (busy() || m_activeOrder.isEmpty()) return;
  const int orderId = m_activeOrder.value("id").toInt();
  ++m_orderRevision;
  call(action, {{"orderId", orderId}}, [this](const QJsonValue &value) {
    const auto order = value.toObject().toVariantMap();
    const QString status = order.value("status").toString();
    if (status == "paid" || status == "cancelled") {
      m_viewedOrder = order;
      emit viewedOrderChanged();
      setActiveOrder({});
      setPage("receipt");
      refreshProfile();
      fetchOrders();
      refreshStations();
    } else {
      setActiveOrder(order);
      setPage(status == "pending_payment" ? "settlement" : "charge");
    }
  });
}

void MobileController::startCharging() { orderAction("orders.start"); }
void MobileController::cancelReservation() { orderAction("orders.cancel"); }
void MobileController::stopCharging() { orderAction("orders.stop"); }
void MobileController::settle() { orderAction("orders.settle"); }

void MobileController::fetchOrders(bool append) {
  if (!signedIn() || (m_loadingOrders && append)) return;
  const int request = ++m_ordersRequest;
  const int session = m_session;
  m_loadingOrders = true;
  m_appendingOrders = append;
  emit ordersChanged();
  m_api->call(
    "orders.list",
    {{"limit", 30},
     {"cursor", append ? m_nextOrderCursor : 0},
     {"filter", m_orderFilter}},
    [this, request, session, append](QJsonValue value) {
      if (request != m_ordersRequest || session != m_session) return;
      if (m_errorAction == "orders.list") clearError();
      const auto result = value.toObject();
      const auto items = result.value("items").toVariant().toList();
      if (append)
        m_orders.append(items);
      else
        m_orders.replace(items);
      m_nextOrderCursor = result.value("nextCursor").toInt();
      m_loadingOrders = false;
      m_ordersLoaded = true;
      emit ordersChanged();
    },
    [this, request, session](QString message) {
      if (request != m_ordersRequest || session != m_session) return;
      m_loadingOrders = false;
      setError(message, "orders.list");
      emit ordersChanged();
    });
}

void MobileController::loadMoreOrders() {
  if (hasMoreOrders() && !m_loadingOrders) fetchOrders(true);
}

void MobileController::filterOrders(const QString &filter) {
  if (m_orderFilter == filter) return;
  m_orderFilter = filter;
  m_ordersLoaded = false;
  m_orderScrollPosition = 0;
  clearError();
  m_orders.replace({});
  m_nextOrderCursor = 0;
  fetchOrders();
}

void MobileController::openOrder(int orderId) {
  if (busy()) return;
  call("orders.get", {{"orderId", orderId}}, [this](const QJsonValue &value) {
    const auto order = value.toObject().toVariantMap();
    const auto status = order.value("status").toString();
    if (status == "reserved" || status == "charging"
        || status == "pending_payment") {
      setActiveOrder(order);
      openActiveOrder();
    } else {
      m_viewedOrder = order;
      emit viewedOrderChanged();
      setPage("receipt", true);
    }
  });
}

void MobileController::refreshProfile() {
  if (!signedIn()) return;
  call(
    "user.me", {},
    [this](const QJsonValue &value) {
      setUser(value.toObject().toVariantMap());
    },
    false);
}

void MobileController::refresh() {
  if (m_page == "orders") {
    if (!m_loadingOrders && !busy()) {
      clearError();
      fetchOrders();
    }
    return;
  }
  if (busy() || m_loadingStations) return;
  clearError();
  if (m_presets.isEmpty()) initialize();
  if (m_page == "home" || m_page == "login") refreshStations();
  if (m_page == "station") fetchStation(m_selectedStation, false);
  if (signedIn()) {
    fetchActive();
    refreshProfile();
  }
}

void MobileController::recharge(const QString &amount) {
  if (busy()) return;
  const auto match = QRegularExpression("^([0-9]{1,5})(?:\\.([0-9]{1,2}))?$")
                       .match(amount.trimmed());
  if (!match.hasMatch()) {
    setError("请输入有效金额，最多保留两位小数");
    return;
  }
  const qint64 cents = match.captured(1).toLongLong() * 100
                     + match.captured(2).leftJustified(2, '0').toInt();
  if (cents < 1 || cents > 1000000) {
    setError("单次充值金额为 0.01 至 10,000 元");
    return;
  }
  if (m_rechargeKey.isEmpty() || m_rechargeAmount != cents) {
    m_rechargeAmount = cents;
    m_rechargeKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
  }
  call("wallet.recharge",
       {{"amountCents", cents}, {"idempotencyKey", m_rechargeKey}},
       [this, cents](const QJsonValue &value) {
         setUser(value.toObject().toVariantMap());
         m_rechargeKey.clear();
         back();
         emit notification(QString("已到账 ¥%1").arg(cents / 100.0, 0, 'f', 2));
       });
}

void MobileController::updateNickname(const QString &nickname) {
  if (busy()) return;
  const auto name = nickname.trimmed();
  if (name.isEmpty() || name.size() > 24) {
    setError("昵称需要 1 至 24 个字符");
    return;
  }
  call("user.update", {{"nickname", name}}, [this](const QJsonValue &value) {
    setUser(value.toObject().toVariantMap());
    back();
  });
}

void MobileController::chooseAvatar() {
#ifdef Q_OS_ANDROID
  navigate("editProfile");
  return;
#else
  if (busy()) return;
  const QString path = QFileDialog::getOpenFileName(
    nullptr, "选择头像", {}, "图片 (*.png *.jpg *.jpeg)");
  if (path.isEmpty()) return;
  QFile image(path);
  if (!image.open(QIODevice::ReadOnly) || image.size() > 2 * 1024 * 1024) {
    setError("请选择大小不超过 2 MB 的 PNG 或 JPEG 图片");
    return;
  }
  QImageReader reader(path);
  const auto size = reader.size();
  if (!reader.canRead() || !size.isValid() || size.width() > 4096
      || size.height() > 4096) {
    setError("图片无法读取，请选择尺寸不超过 4096 × 4096 的图片");
    return;
  }
  call("user.update",
       {{"avatarBase64", QString::fromLatin1(image.readAll().toBase64())}},
       [this](const QJsonValue &value) {
         setUser(value.toObject().toVariantMap());
       });
#endif
}

QString MobileController::avatarSource() const {
  const QString avatar = m_user.value("avatarUrl").toString();
  if (avatar.isEmpty()) return {};
  return QUrl(m_api->baseUrl() + '/').resolved(QUrl(avatar)).toString();
}

void MobileController::openNavigation(const QVariantMap &station) {
#ifdef Q_OS_ANDROID
  bool latitudeOk = false, longitudeOk = false;
  const double latitude = station.value("latitude").toDouble(&latitudeOk);
  const double longitude = station.value("longitude").toDouble(&longitudeOk);
  if (!latitudeOk || !longitudeOk || !std::isfinite(latitude)
      || !std::isfinite(longitude) || std::abs(latitude) > 90
      || std::abs(longitude) > 180) {
    setError("站点坐标不可用，无法导航");
    return;
  }
  const QString lat = QString::number(latitude, 'f', 6);
  const QString lon = QString::number(longitude, 'f', 6);
  const QString name = station.value("name").toString();
  QUrl nativeUrl("amapuri://route/plan/");
  QUrlQuery nativeQuery;
  nativeQuery.addQueryItem("sourceApplication", "chargingplatform");
  nativeQuery.addQueryItem("dname", name);
  nativeQuery.addQueryItem("dlat", lat);
  nativeQuery.addQueryItem("dlon", lon);
  // Station coordinates use GCJ-02, as do AMap's encrypted coordinates.
  nativeQuery.addQueryItem("dev", "0");
  nativeQuery.addQueryItem("t", "0");
  nativeUrl.setQuery(nativeQuery);
  if (QDesktopServices::openUrl(nativeUrl)) return;
  QUrl webUrl("https://uri.amap.com/navigation");
  QUrlQuery webQuery;
  webQuery.addQueryItem("to", lon + ',' + lat + ',' + name);
  webQuery.addQueryItem("mode", "car");
  webQuery.addQueryItem("policy", "0");
  webQuery.addQueryItem("src", "chargingplatform");
  webQuery.addQueryItem("callnative", "0");
  webUrl.setQuery(webQuery);
  if (!QDesktopServices::openUrl(webUrl))
    setError("无法打开地图，请安装高德地图或浏览器后重试");
#else
  if (!m_hasLocation) {
    setPage("location", true);
    return;
  }
  if (station.isEmpty()) return;
  emit navigationRequested(station, m_locationName, m_latitude, m_longitude);
#endif
}

QString MobileController::statusLabel(const QString &status) const {
  static const QHash<QString, QString> labels = {{"idle", "空闲"},
                                                 {"reserved", "已预约"},
                                                 {"charging", "充电中"},
                                                 {"fault", "故障"},
                                                 {"offline", "离线"},
                                                 {"restarting", "重启中"},
                                                 {"pending_payment", "待结算"},
                                                 {"paid", "已完成"},
                                                 {"cancelled", "已取消"}};
  return labels.value(status, status);
}

QString MobileController::formatTime(const QString &value) const {
  const auto time = QDateTime::fromString(value, Qt::ISODate);
  return time.isValid() ? time.toLocalTime().toString("yyyy-MM-dd HH:mm")
                        : QString("—");
}

QString MobileController::reservationRemaining() const {
  const auto expires = QDateTime::fromString(
    m_activeOrder.value("expiresAt").toString(), Qt::ISODate);
  const qint64 seconds = qMax<qint64>(
    0, QDateTime::currentDateTimeUtc().secsTo(expires));
  return QString("%1:%2")
    .arg(seconds / 60, 2, 10, QChar('0'))
    .arg(seconds % 60, 2, 10, QChar('0'));
}
