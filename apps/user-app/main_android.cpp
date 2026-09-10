#include "Appearance.h"
#include "AvatarImage.h"
#include "Fonts.h"
#include "MobileController.h"
#include "NativeMobile.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

int main(int argc, char *argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName("智充出行");
  QGuiApplication::setOrganizationName("ChargingPlatform");
  loadFonts();
  // All controls supply their own appearance. Native Material defaults add
  // paddings/insets that shrink custom labels and icon-only buttons.
  QQuickStyle::setStyle("Basic");

  qmlRegisterType<AvatarImage>("Charging.Native", 1, 0, "AvatarImage");
  MobileController controller;
  NativeMobile native;
  native.configureKeyboard();
  QObject::connect(&controller, &MobileController::notification, &native,
                   &NativeMobile::showToast);
  QObject::connect(&controller, &MobileController::systemLocationRequested,
                   &native, &NativeMobile::requestLocation);
  QObject::connect(&controller, &MobileController::locationPickerRequested,
                   &native, &NativeMobile::pickLocation);
  QObject::connect(&native, &NativeMobile::locationReady, &controller,
                   &MobileController::applySystemLocation);
  QObject::connect(&native, &NativeMobile::locationFailed, &controller,
                   &MobileController::locationFailed);
  bool requestedLocation = false;
  QObject::connect(&controller, &MobileController::userChanged, &controller,
                   [&] {
                     if (controller.signedIn() && !requestedLocation) {
                       requestedLocation = true;
                       controller.refreshLocation();
                     }
                   });
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("appearance",
                                           Appearance::instance());
  engine.rootContext()->setContextProperty("mobile", &controller);
  engine.rootContext()->setContextProperty("nativeMobile", &native);
  QObject::connect(
    &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
    [] {
      QCoreApplication::exit(1);
    },
    Qt::QueuedConnection);
  engine.load(QUrl("qrc:/qml/AndroidMain.qml"));
  if (engine.rootObjects().isEmpty()) return 1;
  auto updateBars = [&native] {
    auto *appearance = Appearance::instance();
    native.updateBars(appearance->colors().value("paper").value<QColor>(),
                      appearance->dark());
  };
  QObject::connect(Appearance::instance(), &Appearance::changed, &native,
                   updateBars);
  QTimer::singleShot(500, &native, updateBars);
  controller.restoreSession();
  controller.initialize();
  return application.exec();
}
