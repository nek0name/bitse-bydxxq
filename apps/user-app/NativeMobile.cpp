#include "NativeMobile.h"
#include "Appearance.h"
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPermissions>
#include <QPointer>
#include <QtCore/qcoreapplication_platform.h>

namespace {
QPointer<NativeMobile> bridge;
constexpr auto javaClass = "com/chargingplatform/user/NativeMobile";
void keyboardInsets(JNIEnv *, jclass, jdouble inset) {
  if (bridge)
    QMetaObject::invokeMethod(
      bridge,
      [inset] {
        if (bridge) bridge->setKeyboardInset(inset);
      },
      Qt::QueuedConnection);
}
void locationResult(JNIEnv *, jclass, jdouble latitude, jdouble longitude,
                    jstring name) {
  const auto label = QJniObject(name).toString();
  if (bridge)
    QMetaObject::invokeMethod(
      bridge,
      [latitude, longitude, label] {
        if (bridge) emit bridge->locationReady(latitude, longitude, label);
      },
      Qt::QueuedConnection);
}
void locationError(JNIEnv *, jclass, jstring message) {
  const auto text = QJniObject(message).toString();
  if (bridge)
    QMetaObject::invokeMethod(
      bridge,
      [text] {
        if (bridge) emit bridge->locationFailed(text);
      },
      Qt::QueuedConnection);
}
} // namespace

NativeMobile::NativeMobile(QObject *parent) : QObject(parent) {
  bridge = this;
  QJniEnvironment env;
  const JNINativeMethod methods[] = {
    {"keyboardInsets", "(D)V", reinterpret_cast<void *>(keyboardInsets)},
    {"locationResult", "(DDLjava/lang/String;)V",
     reinterpret_cast<void *>(locationResult)},
    {"locationError", "(Ljava/lang/String;)V",
     reinterpret_cast<void *>(locationError)}};
  env.registerNativeMethods(javaClass, methods, 3);
}

void NativeMobile::setKeyboardInset(qreal inset) {
  inset = qMax(qreal(0), inset);
  if (qFuzzyCompare(m_keyboardInset, inset)) return;
  m_keyboardInset = inset;
  emit keyboardInsetChanged();
}

void NativeMobile::configureKeyboard() {
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
    const auto context = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>(javaClass, "configureKeyboard",
                                       "(Landroid/content/Context;)V",
                                       context.object());
  });
}

void NativeMobile::showToast(const QString &message) {
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([message] {
    const auto context = QNativeInterface::QAndroidApplication::context();
    const auto text = QJniObject::fromString(message);
    QJniObject::callStaticMethod<void>(
      javaClass, "toast", "(Landroid/content/Context;Ljava/lang/String;)V",
      context.object(), text.object());
  });
}

void NativeMobile::requestLocation() {
  QLocationPermission permission;
  permission.setAccuracy(QLocationPermission::Precise);
  permission.setAvailability(QLocationPermission::WhenInUse);
  auto status = qApp->checkPermission(permission);
  if (status == Qt::PermissionStatus::Undetermined) {
    qApp->requestPermission(
      permission, this, [this](const QPermission &result) {
        QLocationPermission approximate;
        approximate.setAccuracy(QLocationPermission::Approximate);
        approximate.setAvailability(QLocationPermission::WhenInUse);
        if (result.status() != Qt::PermissionStatus::Granted
            && qApp->checkPermission(approximate)
                 != Qt::PermissionStatus::Granted) {
          emit locationFailed("未获得定位权限，可在地图上手动选择位置");
          return;
        }
        QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
          auto context = QNativeInterface::QAndroidApplication::context();
          QJniObject::callStaticMethod<void>(javaClass, "locate",
                                             "(Landroid/content/Context;)V",
                                             context.object());
        });
      });
    return;
  }
  QLocationPermission approximate;
  approximate.setAccuracy(QLocationPermission::Approximate);
  approximate.setAvailability(QLocationPermission::WhenInUse);
  if (status != Qt::PermissionStatus::Granted
      && qApp->checkPermission(approximate) != Qt::PermissionStatus::Granted) {
    emit locationFailed(
      "未获得定位权限，请在手机设置中允许，或在地图上手动选择位置");
    return;
  }
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
    auto context = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>(
      javaClass, "locate", "(Landroid/content/Context;)V", context.object());
  });
}

void NativeMobile::pickLocation(double latitude, double longitude, bool known) {
  auto *appearance = Appearance::instance();
  QJsonObject palette{{"dark", appearance->dark()}};
  const auto colors = appearance->colors();
  for (const auto &key : {"paper", "card", "ink", "muted", "primary",
                          "primaryText", "border", "disabled"})
    palette.insert(key, colors.value(key).value<QColor>().name());
  const auto theme = QString::fromUtf8(
    QJsonDocument(palette).toJson(QJsonDocument::Compact));
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
    [latitude, longitude, known, theme] {
      auto context = QNativeInterface::QAndroidApplication::context();
      QJniObject::callStaticMethod<void>(
        javaClass, "pickLocation",
        "(Landroid/content/Context;DDZLjava/lang/String;)V", context.object(),
        jdouble(latitude), jdouble(longitude), jboolean(known),
        QJniObject::fromString(theme).object());
    });
}

void NativeMobile::updateBars(const QColor &color, bool dark) {
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([color, dark] {
    auto context = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>(
      javaClass, "updateBars", "(Landroid/content/Context;IZ)V",
      context.object(), jint(color.rgba()), jboolean(dark));
  });
}
