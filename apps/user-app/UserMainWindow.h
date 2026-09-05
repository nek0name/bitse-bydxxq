#pragma once

#include <QMainWindow>
#include <functional>

class QStackedWidget;

class UserMainWindow final : public QMainWindow {
  public:
    explicit UserMainWindow(QWidget *parent = nullptr);
    void setLogoutHandler(std::function<void()> handler) { logoutHandler_ = std::move(handler); }

  private:
    QStackedWidget *pages_ = nullptr;
    std::function<void()> logoutHandler_;
};
