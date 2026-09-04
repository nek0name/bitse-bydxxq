#pragma once

#include <QMainWindow>
class QButtonGroup;

class AdminMainWindow final : public QMainWindow {
  public:
    explicit AdminMainWindow(QWidget *parent = nullptr);

  private:
    QButtonGroup *navigationGroup_ = nullptr;
};
