#include "Fonts.h"

#include <QFontDatabase>
#include <QGuiApplication>

void loadFonts() {
  Q_INIT_RESOURCE(fonts);
  for (const auto *weight : {"Regular", "Medium", "Bold"})
    QFontDatabase::addApplicationFont(
      QString(":/fonts/HarmonyOS_Sans_SC_%1.ttf").arg(weight));
  QGuiApplication::setFont(QFont("HarmonyOS Sans SC", 10));
}
