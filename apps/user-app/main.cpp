#include "UserMainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName("充电用户端");
    UserMainWindow window;
    window.show();
    return application.exec();
}
