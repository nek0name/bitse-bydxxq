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
#include <QDateTime>
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
        QPushButton[text="注册账号"] { color: #2f7fae; background: transparent; }
        QPushButton[text="注册账号"]:pressed { color: #256889; }
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
    auto *errorLabel = new QLabel;
    errorLabel->setStyleSheet("color: #c45151; font-size: 13px; padding-left: 4px;");
    errorLabel->setVisible(false);
    layout->insertWidget(layout->count() - 1, errorLabel);
    auto *loginButton = new QPushButton("登录");
    loginButton->setMinimumHeight(52);
    loginButton->setDefault(true);
    layout->addWidget(loginButton);
    auto *registerButton = new QPushButton("注册账号");
    registerButton->setFlat(true);
    layout->addWidget(registerButton);
    layout->addSpacing(8);
    QObject::connect(registerButton, &QPushButton::clicked, &dialog, [&] {
        const QString newPhone = phone->text().trimmed();
        if (newPhone.size() != 11) { errorLabel->setText("请输入 11 位手机号后再注册"); errorLabel->setVisible(true); return; }
        QSqlQuery exists(db);
        exists.prepare("SELECT id FROM users WHERE phone = :phone");
        exists.bindValue(":phone", newPhone);
        if (!exists.exec()) { errorLabel->setText("注册失败，请稍后重试"); errorLabel->setVisible(true); return; }
        if (exists.next()) { errorLabel->setText("该手机号已注册，请直接登录"); errorLabel->setVisible(true); return; }
        const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        QSqlQuery insert(db);
        insert.prepare("INSERT INTO users (phone, nickname, wallet_balance, status, created_at, updated_at) "
                       "VALUES (:phone, :nickname, 0, 'active', :created, :updated)");
        insert.bindValue(":phone", newPhone);
        insert.bindValue(":nickname", "用户" + newPhone.right(4));
        insert.bindValue(":created", now);
        insert.bindValue(":updated", now);
        if (!insert.exec()) { errorLabel->setText("注册失败，请稍后重试"); errorLabel->setVisible(true); return; }
        userId = insert.lastInsertId().toLongLong();
        dialog.accept();
    });
    QObject::connect(loginButton, &QPushButton::clicked, &dialog, [&] {
        auto showError = [&](const QString &message) { errorLabel->setText(message); errorLabel->setVisible(true); };
        if (phone->text().trimmed().size() != 11) { showError("请输入 11 位手机号"); return; }
        QSqlQuery query(db);
        query.prepare("SELECT id, status FROM users WHERE phone = :phone");
        query.bindValue(":phone", phone->text().trimmed());
        if (!query.exec() || !query.next()) { showError("该手机号未注册"); return; }
        const auto status = query.value(1).toString();
        if (status != "active") { showError(status == "frozen" ? "账户已冻结，暂时无法登录" : "账户已禁用，暂时无法登录"); return; }
        userId = query.value(0).toLongLong();
        dialog.accept();
    });
    return dialog.exec() == QDialog::Accepted;
}

void showLoginAlert(QWidget *parent, const QString &title, const QString &message) {
    QDialog alert(parent);
    alert.setWindowTitle(title);
    alert.setMinimumWidth(300);
    alert.setStyleSheet(R"QSS(
        QDialog { background: #ffffff; color: #17212b; }
        QLabel#alertTitle { font-size: 17px; font-weight: 600; }
        QLabel#alertMessage { color: #687684; font-size: 14px; }
        QPushButton { min-height: 44px; border: none; border-radius: 10px; background: #2f7fae; color: white; font-size: 15px; font-weight: 600; }
        QPushButton:pressed { background: #256889; }
    )QSS");
    auto *layout = new QVBoxLayout(&alert);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(10);
    auto *titleLabel = new QLabel(title);
    titleLabel->setObjectName("alertTitle");
    layout->addWidget(titleLabel);
    auto *messageLabel = new QLabel(message);
    messageLabel->setObjectName("alertMessage");
    messageLabel->setWordWrap(true);
    layout->addWidget(messageLabel);
    auto *ok = new QPushButton("知道了");
    layout->addSpacing(8);
    layout->addWidget(ok);
    QObject::connect(ok, &QPushButton::clicked, &alert, &QDialog::accept);
    alert.exec();
}
}

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    QApplication::setApplicationName("智充出行");
    while (true) {
        qint64 userId = 0;
        if (!login(nullptr, userId)) return 0;
        qputenv("CHARGING_USER_ID", QByteArray::number(userId));
        UserMainWindow window;
        window.setLogoutHandler([&application, &window] {
            window.close();
            application.exit(42);
        });
        window.show();
        if (application.exec() != 42) return 0;
        qunsetenv("CHARGING_USER_ID");
    }
}
