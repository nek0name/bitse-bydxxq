#pragma once

#include <QMainWindow>

class QStackedWidget;

class UserMainWindow final : public QMainWindow {
  public:
    explicit UserMainWindow(QWidget *parent = nullptr);

  private:
    QStackedWidget *pages_ = nullptr;
};
