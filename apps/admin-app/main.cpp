#include "AdminMainWindow.h"
#include "AdminUi.h"
#include "Fonts.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication application(argc, argv);
  loadFonts();
  adminui::applyTheme();
  QApplication::setApplicationName("充电运营管理端");
  AdminMainWindow window;
  window.show();
  return application.exec();
}
