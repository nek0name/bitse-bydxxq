#include "UserMainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QString resolveDatabasePath() {
    const QString configured = qEnvironmentVariable("CHARGING_DB_PATH");
    if (!configured.isEmpty()) return QDir::cleanPath(configured);

    QDir dir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {
      dir.filePath("../../../../database/charging_platform.db"),
      dir.filePath("../../../database/charging_platform.db"),
      QDir::current().filePath("database/charging_platform.db")};
    QString fallback;
    for (const auto &candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.exists()) return info.canonicalFilePath();
        if (fallback.isEmpty() && info.dir().exists())
            fallback = info.absoluteFilePath();
    }
    return fallback;
}

QString resolveSqlPath(const QString &databasePath, const QString &name) {
    const QFileInfo databaseInfo(databasePath);
    const QStringList candidates = {
      databaseInfo.dir().filePath(name),
      QDir(QCoreApplication::applicationDirPath())
        .filePath("../../../../database/" + name),
      QDir(QCoreApplication::applicationDirPath())
        .filePath("../../../database/" + name),
      QDir::current().filePath("database/" + name)};
    for (const auto &candidate : candidates) {
        if (QFileInfo::exists(candidate))
            return QFileInfo(candidate).absoluteFilePath();
    }
    return {};
}

QStringList splitSqlScript(const QString &script) {
    QStringList statements;
    QString statement;
    bool singleQuote = false;
    bool doubleQuote = false;
    bool lineComment = false;
    bool blockComment = false;
    for (int index = 0; index < script.size(); ++index) {
        const QChar ch = script.at(index);
        const QChar next = index + 1 < script.size() ? script.at(index + 1)
                                                     : QChar();
        if (lineComment) {
            if (ch == '\n') lineComment = false;
            continue;
        }
        if (blockComment) {
            if (ch == '*' && next == '/') {
                blockComment = false;
                ++index;
            }
            continue;
        }
        if (!singleQuote && !doubleQuote && ch == '-' && next == '-') {
            lineComment = true;
            ++index;
            continue;
        }
        if (!singleQuote && !doubleQuote && ch == '/' && next == '*') {
            blockComment = true;
            ++index;
            continue;
        }
        if (ch == '\'' && !doubleQuote) {
            statement += ch;
            if (singleQuote && next == '\'') {
                statement += next;
                ++index;
            } else {
                singleQuote = !singleQuote;
            }
            continue;
        }
        if (ch == '"' && !singleQuote) {
            statement += ch;
            if (doubleQuote && next == '"') {
                statement += next;
                ++index;
            } else {
                doubleQuote = !doubleQuote;
            }
            continue;
        }
        if (ch == ';' && !singleQuote && !doubleQuote) {
            if (!statement.trimmed().isEmpty())
                statements.append(statement.trimmed());
            statement.clear();
        } else {
            statement += ch;
        }
    }
    if (!statement.trimmed().isEmpty()) statements.append(statement.trimmed());
    return statements;
}

bool executeSqlScript(QSqlDatabase &db, const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    for (const auto &statement :
         splitSqlScript(QString::fromUtf8(file.readAll()))) {
        QSqlQuery query(db);
        if (!query.exec(statement)) {
            db.rollback();
            return false;
        }
    }
    return true;
}

bool initializeDatabase(QSqlDatabase &db, const QString &databasePath) {
    const QString schemaPath = resolveSqlPath(databasePath, "schema.sql");
    const QString seedPath = resolveSqlPath(databasePath, "seed.sql");
    if (schemaPath.isEmpty() || seedPath.isEmpty()) return false;
    return executeSqlScript(db, schemaPath) && executeSqlScript(db, seedPath);
}

bool hasSeedData(const QSqlDatabase &db) {
    QSqlQuery query(db);
    query.prepare("SELECT EXISTS (SELECT 1 FROM users) "
                  "AND EXISTS (SELECT 1 FROM stations) "
                  "AND EXISTS (SELECT 1 FROM chargers) "
                  "AND EXISTS (SELECT 1 FROM tariffs)");
    return query.exec() && query.next();
}

QSqlDatabase loginDatabase() {
    const QString path = resolveDatabasePath();
    if (path.isEmpty()) return {};
    const bool needsInitialization = !QFileInfo::exists(path);
    const QFileInfo pathInfo(path);
    if (!pathInfo.dir().exists()
        && !QDir().mkpath(pathInfo.dir().absolutePath()))
        return {};
    auto db = QSqlDatabase::addDatabase("QSQLITE", "login");
    db.setDatabaseName(path);
    if (!db.open()) return {};
    QSqlQuery pragma(db);
    pragma.exec("PRAGMA foreign_keys = ON");
    pragma.exec("PRAGMA busy_timeout = 5000");
    if ((needsInitialization || !hasSeedData(db))
        && !initializeDatabase(db, path)) {
        db.close();
        return {};
    }
    return db;
}

bool login(QWidget *parent, qint64 &userId) {
    const auto db = loginDatabase();
    if (!db.isOpen()) {
        QMessageBox::critical(parent, "登录失败", "无法连接数据库");
        return false;
    }
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
    )QSS");
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 54, 28, 28);
    layout->setSpacing(14);
    auto *brand = new QLabel("智充出行");
    QFont brandFont = brand->font();
    brandFont.setPointSize(24);
    brandFont.setBold(true);
    brand->setFont(brandFont);
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
    // Apply after the style sheet polish pass; otherwise Qt may restore its
    // default placeholder color.
    QTimer::singleShot(0, phone, [phone] {
        auto palette = phone->palette();
        palette.setColor(QPalette::PlaceholderText, QColor("#c3cbd1"));
        phone->setPalette(palette);
    });
    auto *errorLabel = new QLabel;
    errorLabel->setStyleSheet(
      "color: #c45151; font-size: 13px; padding-left: 4px;");
    errorLabel->setVisible(false);
    layout->addWidget(errorLabel);
    layout->addStretch(1);
    auto *loginButton = new QPushButton("登录");
    loginButton->setMinimumHeight(52);
    loginButton->setDefault(true);
    layout->addWidget(loginButton);
    layout->addSpacing(8);
    QObject::connect(loginButton, &QPushButton::clicked, &dialog, [&] {
        auto showError = [&](const QString &message) {
            errorLabel->setText(message);
            errorLabel->setVisible(true);
        };
        const QString loginPhone = phone->text().trimmed();
        if (loginPhone.size() != 11
            || !QRegularExpression("^1\\d{10}$").match(loginPhone).hasMatch()) {
            showError("请输入有效的 11 位手机号");
            return;
        }
        QSqlQuery query(db);
        query.prepare("SELECT id, status FROM users WHERE phone = :phone");
        query.bindValue(":phone", loginPhone);
        if (!query.exec()) {
            showError("登录失败，请稍后重试");
            return;
        }
        if (!query.next()) {
            const QString now = QDateTime::currentDateTimeUtc().toString(
              Qt::ISODate);
            QSqlQuery insert(db);
            insert.prepare(
              "INSERT INTO users (phone, nickname, wallet_balance, status, "
              "created_at, updated_at) VALUES (:phone, :nickname, 0, 'active', "
              ":created, :updated)");
            insert.bindValue(":phone", loginPhone);
            insert.bindValue(":nickname", "用户" + loginPhone.right(4));
            insert.bindValue(":created", now);
            insert.bindValue(":updated", now);
            if (!insert.exec()) {
                showError("注册失败，请稍后重试");
                return;
            }
            userId = insert.lastInsertId().toLongLong();
            dialog.accept();
            return;
        }
        const auto status = query.value(1).toString();
        if (status != "active") {
            showError(status == "frozen" ? "账户已冻结，暂时无法登录"
                                         : "账户已禁用，暂时无法登录");
            return;
        }
        userId = query.value(0).toLongLong();
        QSqlQuery lastLogin(db);
        lastLogin.prepare(
          "UPDATE users SET last_login_at = :last_login, updated_at = :updated "
          "WHERE id = :id");
        const QString now = QDateTime::currentDateTimeUtc().toString(
          Qt::ISODate);
        lastLogin.bindValue(":last_login", now);
        lastLogin.bindValue(":updated", now);
        lastLogin.bindValue(":id", userId);
        lastLogin.exec();
        dialog.accept();
    });
    return dialog.exec() == QDialog::Accepted;
}

void showLoginAlert(QWidget *parent, const QString &title,
                    const QString &message) {
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
} // namespace

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
