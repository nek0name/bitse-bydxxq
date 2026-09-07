#include "Appearance.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QFile>
#include <QRegularExpression>
#include <cmath>

namespace {
using PortalSettings = QMap<QString, QVariantMap>;
constexpr auto appearanceGroup = "org.freedesktop.appearance";

QColor mix(const QColor &base, const QColor &target, double amount) {
  return QColor::fromRgbF(
    base.redF() * (1 - amount) + target.redF() * amount,
    base.greenF() * (1 - amount) + target.greenF() * amount,
    base.blueF() * (1 - amount) + target.blueF() * amount);
}

double luminance(const QColor &color) {
  const auto channel = [](double value) {
    return value <= 0.04045 ? value / 12.92
                            : std::pow((value + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF())
       + 0.0722 * channel(color.blueF());
}

QColor buttonColor(QColor color) {
  while ((1.05 / (luminance(color) + 0.05)) < 4.5)
    color = mix(color, Qt::black, 0.08);
  return color;
}
} // namespace

Appearance *Appearance::instance() {
  static auto *appearance = new Appearance(QCoreApplication::instance());
  return appearance;
}

Appearance::Appearance(QObject *parent)
    : QObject(parent), settings_(QSettings::IniFormat, QSettings::UserScope,
                                 "ChargingPlatform", "Appearance"),
      mode_(settings_.value("mode", "system").toString()),
      primaryColor_(settings_.value("primaryColor", "#6259CA").toString()),
      secondaryColor_(settings_.value("secondaryColor", "#8E86B8").toString()) {
  qDBusRegisterMetaType<PortalSettings>();
  qRegisterMetaType<QDBusVariant>();
  auto bus = QDBusConnection::sessionBus();
  bus.connect("org.freedesktop.portal.Desktop",
              "/org/freedesktop/portal/desktop",
              "org.freedesktop.portal.Settings", "SettingChanged", this,
              SLOT(systemSettingChanged(QString, QString, QDBusVariant)));
  auto request = QDBusMessage::createMethodCall(
    "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
    "org.freedesktop.portal.Settings", "ReadAll");
  request << QStringList{appearanceGroup};
  auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(request), this);
  connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
    const QDBusPendingReply<PortalSettings> reply = *watcher;
    watcher->deleteLater();
    if (!reply.isError() && !systemSettingReceived_)
      setSystemDark(
        reply.value().value(appearanceGroup).value("color-scheme").toUInt()
        == 1);
  });
}

QString Appearance::mode() const { return mode_; }
QColor Appearance::primaryColor() const { return primaryColor_; }
QColor Appearance::secondaryColor() const { return secondaryColor_; }
bool Appearance::dark() const {
  return mode_ == "dark" || (mode_ == "system" && systemDark_);
}

void Appearance::save(const QString &key, const QVariant &value) {
  settings_.setValue(key, value);
  settings_.sync();
  emit changed();
}

void Appearance::setMode(const QString &mode) {
  if (mode == mode_ || (mode != "system" && mode != "light" && mode != "dark"))
    return;
  mode_ = mode;
  save("mode", mode);
}

void Appearance::setPrimaryColor(const QColor &color) {
  if (!color.isValid() || color == primaryColor_) return;
  primaryColor_ = QColor(color.name());
  save("primaryColor", primaryColor_.name());
}

void Appearance::setSecondaryColor(const QColor &color) {
  if (!color.isValid() || color == secondaryColor_) return;
  secondaryColor_ = QColor(color.name());
  save("secondaryColor", secondaryColor_.name());
}

void Appearance::setSystemDark(bool dark) {
  if (systemDark_ == dark) return;
  systemDark_ = dark;
  if (mode_ == "system") emit changed();
}

void Appearance::systemSettingChanged(const QString &group, const QString &key,
                                      const QDBusVariant &value) {
  if (group == appearanceGroup && key == "color-scheme") {
    systemSettingReceived_ = true;
    setSystemDark(value.variant().toUInt() == 1);
  }
}

QStringList Appearance::swatches() const {
  return {"#6259CA", "#3867A6", "#287B73", "#A64F70", "#926B37", "#8E86B8"};
}

QVariantMap Appearance::colors() const {
  const bool isDark = dark();
  const QColor paper(isDark ? "#191A20" : "#F7F7F8");
  const QColor card(isDark ? "#23242D" : "#FFFFFF");
  const auto primary = buttonColor(primaryColor_);
  const auto primaryText = isDark ? mix(primaryColor_, Qt::white, 0.5)
                                  : primary;
  const auto secondary = isDark ? mix(secondaryColor_, Qt::white, 0.22)
                                : secondaryColor_;
  return {
    {"paper", paper},
    {"card", card},
    {"ink", QColor(isDark ? "#EEEEF4" : "#24252B")},
    {"muted", QColor(isDark ? "#B0B0BF" : "#71717A")},
    {"primary", primary},
    {"primaryText", primaryText},
    {"primaryPressed", mix(primary, Qt::black, 0.15)},
    {"primaryLight", mix(card, primaryColor_, isDark ? 0.20 : 0.09)},
    {"primarySoftPressed", mix(card, primaryColor_, isDark ? 0.32 : 0.18)},
    {"secondary", secondary},
    {"accent", mix(secondaryColor_, Qt::white, 0.52)},
    {"secondaryLight", mix(card, secondaryColor_, isDark ? 0.22 : 0.12)},
    {"border", QColor(isDark ? "#3A3B47" : "#E7E7EC")},
    {"surfaceDark", QColor("#30313D")},
    {"surfaceDarkRaised", QColor("#454653")},
    {"heroMuted", QColor("#CECCD9")},
    {"amber", QColor(isDark ? "#E4B56A" : "#98600A")},
    {"amberLight", QColor(isDark ? "#3D3224" : "#FFF5DF")},
    {"amberPressed", QColor(isDark ? "#55422B" : "#F8E6BD")},
    {"danger", QColor(isDark ? "#F094A5" : "#BA334A")},
    {"dangerLight", QColor(isDark ? "#452B35" : "#FFF0F3")},
    {"disabled", QColor(isDark ? "#34353F" : "#E7E7EC")},
    {"disabledText", QColor(isDark ? "#797987" : "#9898A3")},
    {"overlay", QColor(isDark ? "#99000000" : "#7524252B")},
    {"toast", QColor("#D930313D")},
    {"onPrimary", QColor("#FFFFFF")}};
}

QString Appearance::iconSource(const QString &resource,
                               const QColor &color) const {
  QString path = resource;
  if (path.startsWith("qrc:/")) path.remove(0, 3);
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return {};
  auto svg = QString::fromUtf8(file.readAll());
  static const QRegularExpression stroke(
    "stroke\\s*=\\s*([\"'])(?!none\\1)[^\"']*\\1");
  svg.replace(stroke, "stroke=\"" + color.name() + "\"");
  return "data:image/svg+xml;base64,"
       + QString::fromLatin1(svg.toUtf8().toBase64());
}
