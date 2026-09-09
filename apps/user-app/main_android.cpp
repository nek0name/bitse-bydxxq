#include "Appearance.h"
#include "Fonts.h"
#include "MobileController.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

int main(int argc, char *argv[]) {
  QGuiApplication application(argc, argv);
  QGuiApplication::setApplicationName("智充出行");
  QGuiApplication::setOrganizationName("ChargingPlatform");
  loadFonts();

  MobileController controller;
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("appearance", Appearance::instance());
  engine.rootContext()->setContextProperty("mobile", &controller);
  QObject::connect(
    &engine, &QQmlApplicationEngine::objectCreationFailed, &application,
    [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
  engine.load(QUrl("qrc:/qml/AndroidMain.qml"));
  if (engine.rootObjects().isEmpty()) return 1;
  controller.initialize();
  return application.exec();
}
