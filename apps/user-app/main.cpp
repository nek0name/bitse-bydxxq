#include "UserMainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QLinearGradient>
#include <QPalette>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>

namespace {
QSqlDatabase loginDatabase() {
    QString path = qEnvironmentVariable("CHARGING_DB_PATH");
    if (path.isEmpty()) {
        QDir dir(QCoreApplication::applicationDirPath());
        const QStringList candidates = {dir.filePath("../../../../database/charging_platform.db"),
                                        dir.filePath("../../../database/charging_platform.db"),
                                        QDir::current().filePath("database/charging_platform.db")};
        for (const auto &candidate : candidates) if (QFileInfo::exists(candidate)) { path = QFileInfo(candidate).canonicalFilePath(); break; }
    }
    if (path.isEmpty()) return {};
    auto db = QSqlDatabase::addDatabase("QSQLITE", "login");
    db.setDatabaseName(path);
    if (!db.open()) return {};
    return db;
}

bool login(QWidget *parent, qint64 &userId) {
    const auto db = loginDatabase();
    if (!db.isOpen()) { QMessageBox::critical(parent, "登录失败", "无法连接数据库"); return false; }
    QDialog dialog(parent);
    dialog.setWindowTitle("智充出行");
    dialog.resize(420, 720);
    dialog.setMinimumSize(320, 540);
    QLinearGradient background(0, 0, 1, 1);
    background.setCoordinateMode(QGradient::ObjectBoundingMode);
    background.setColorAt(0.0, QColor("#fffefa"));
    background.setColorAt(0.34, QColor("#f5fbfb"));
    background.setColorAt(0.62, QColor("#fff4f7"));
    background.setColorAt(1.0, QColor("#fffdfd"));
    QPalette palette = dialog.palette();
    palette.setBrush(QPalette::Window, QBrush(background));
    dialog.setPalette(palette);
    dialog.setAutoFillBackground(true);
    dialog.setStyleSheet(R"QSS(
        QDialog { color: #17212b; }
        QLabel#loginSubtitle { color: #687684; font-size: 14px; }
        QLineEdit {
            background: rgba(255, 255, 255, 218);
            border: 1px solid #d7e0e7;
            border-radius: 12px;
            padding: 0 16px;
            color: #17212b;
            selection-background-color: #8bb8d8;
        }
        QLineEdit:focus { border: 2px solid #3e88b5; padding: 0 15px; }
        QPushButton {
            border: none;
            border-radius: 12px;
            padding: 0 18px;
            font-size: 15px;
        }
        QPushButton[text="登录"] {
            background: #2f7fae;
            color: white;
            font-weight: 600;
        }
        QPushButton[text="登录"]:pressed { background: #256889; }
        QPushButton[text="退出"] { color: #687684; background: transparent; }
        QPushButton[text="退出"]:pressed { color: #2f7fae; }
    )QSS");
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 54, 28, 28);
    layout->setSpacing(14);
    auto *brand = new QLabel("智充出行");
    QFont brandFont = brand->font(); brandFont.setPointSize(24); brandFont.setBold(true); brand->setFont(brandFont);
    brand->setAlignment(Qt::AlignCenter);
    layout->addWidget(brand);
    auto *welcome = new QLabel("登录后查找附近充电站");
    welcome->setObjectName("loginSubtitle");
    welcome->setAlignment(Qt::AlignCenter);
    layout->addWidget(welcome);
    layout->addSpacing(30);
    auto *phoneLabel = new QLabel("手机号");
    QFont labelFont = phoneLabel->font(); labelFont.setBold(true); phoneLabel->setFont(labelFont);
    layout->addWidget(phoneLabel);
    auto *phone = new QLineEdit;
    phone->setPlaceholderText("请输入手机号");
    phone->setMaxLength(11);
    phone->setMinimumHeight(48);
    phone->setFont(QFont(phone->font().family(), 14));
    QPalette phonePalette = phone->palette();
    phonePalette.setColor(QPalette::PlaceholderText, QColor("#b6c0c8"));
    phone->setPalette(phonePalette);
    phone->setInputMethodHints(Qt::ImhDigitsOnly);
    layout->addWidget(phone);
    // Apply after the style sheet polish pass; otherwise Qt may restore its default placeholder color.
    QTimer::singleShot(0, phone, [phone] {
        auto palette = phone->palette();
        palette.setColor(QPalette::PlaceholderText, QColor("#c3cbd1"));
        phone->setPalette(palette);
    });
    layout->addStretch(1);
    auto *loginButton = new QPushButton("登录");
    loginButton->setMinimumHeight(52);
    loginButton->setDefault(true);
    layout->addWidget(loginButton);
    auto *cancelButton = new QPushButton("退出");
    cancelButton->setFlat(true);
    layout->addWidget(cancelButton);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("演示账号：13800000001"));
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(loginButton, &QPushButton::clicked, &dialog, [&] {
        if (phone->text().trimmed().size() != 11) { QMessageBox::warning(&dialog, "提示", "请输入 11 位手机号"); return; }
        QSqlQuery query(db);
        query.prepare("SELECT id, status FROM users WHERE phone = :phone");
        query.bindValue(":phone", phone->text().trimmed());
        if (!query.exec() || !query.next()) { QMessageBox::warning(&dialog, "登录失败", "手机号未注册"); return; }
        const auto status = query.value(1).toString();
        if (status != "active") { QMessageBox::warning(&dialog, "登录失败", status == "frozen" ? "账户已冻结" : "账户已禁用"); return; }
        userId = query.value(0).toLongLong();
        dialog.accept();
    });
    return dialog.exec() == QDialog::Accepted;
}
}

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName("智充出行");
    qint64 userId = 0;
    if (!login(nullptr, userId)) return 0;
    qputenv("CHARGING_USER_ID", QByteArray::number(userId));
    UserMainWindow window;
    window.show();
    return application.exec();
}
