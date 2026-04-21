#include "NotificationManager.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QApplication>
#include <QStyle>
#include <QDebug>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QPropertyAnimation>

NotificationManager& NotificationManager::instance() {
    static NotificationManager inst;
    return inst;
}

NotificationManager::NotificationManager(QObject* parent) : QObject(parent) {}

void NotificationManager::initialize(QWidget* parent) {
    if (m_tray) return;
    m_mainWindow = parent;
    m_tray = new QSystemTrayIcon(
        QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation),
        parent ? static_cast<QObject*>(parent) : this);
    m_tray->setToolTip("계정 관리자");
    m_tray->show();
}

void NotificationManager::checkBudgets(int userId) {
    if (!m_tray) return;

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(R"(
        SELECT b.category, b.monthly_limit,
               COALESCE(SUM(t.amount), 0) as spent
        FROM budgets b
        LEFT JOIN accounts a ON a.user_id = b.user_id
        LEFT JOIN transactions t ON t.account_id = a.id
            AND t.type = '출금'
            AND t.category = b.category
            AND strftime('%Y-%m', t.created_at) = strftime('%Y-%m', 'now', 'localtime')
        WHERE b.user_id = :uid
        GROUP BY b.id, b.category, b.monthly_limit
    )");
    q.bindValue(":uid", userId);

    if (!q.exec()) {
        qWarning() << "checkBudgets failed:" << q.lastError().text();
        return;
    }

    while (q.next()) {
        QString cat   = q.value(0).toString();
        double  limit = q.value(1).toDouble();
        double  spent = q.value(2).toDouble();

        if (limit <= 0.0) continue;
        double ratio = spent / limit;

        if (spent > limit) {
            notify("예산 초과",
                   QString("%1 예산을 초과했습니다! (지출: %2 / 한도: %3)")
                       .arg(cat).arg(spent, 0, 'f', 0).arg(limit, 0, 'f', 0),
                   QColor("#e24b4a"));
        } else if (spent == limit) {
            notify("예산 도달",
                   QString("%1 예산에 도달했습니다! (지출: %2 / 한도: %3)")
                       .arg(cat).arg(spent, 0, 'f', 0).arg(limit, 0, 'f', 0),
                   QColor("#FFB800"));
        } else if (ratio >= 0.8) {
            notify("예산 경고",
                   QString("%1 예산이 %2%에 도달했습니다 (%3 / %4)")
                       .arg(cat).arg(int(ratio * 100)).arg(spent, 0, 'f', 0).arg(limit, 0, 'f', 0),
                   QColor("#F59E0B"));
        }
    }
}

void NotificationManager::notify(const QString& title, const QString& message,
                                  const QColor& accentColor) {
    if (!m_mainWindow) {
        if (m_tray && QSystemTrayIcon::isSystemTrayAvailable())
            m_tray->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
        return;
    }

    auto* toast = new QFrame(nullptr);
    toast->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    toast->setAttribute(Qt::WA_TranslucentBackground);
    toast->setAttribute(Qt::WA_StyledBackground, true);
    toast->setAttribute(Qt::WA_DeleteOnClose);
    toast->setAttribute(Qt::WA_ShowWithoutActivating);
    toast->setFixedWidth(320);
    toast->setStyleSheet(QString(
        "QFrame {"
        "  background-color: rgba(15, 23, 42, 235);"
        "  border: none;"
        "  border-left: 5px solid %1;"
        "  border-radius: 12px;"
        "}"
        "QLabel { background: transparent; }").arg(accentColor.name()));

    auto* hl = new QHBoxLayout(toast);
    hl->setContentsMargins(14, 14, 14, 14);
    hl->setSpacing(12);

    // 상태별 아이콘 배지 (색상 원형 + "!")
    QString iconChar = title.contains("초과") ? "X" : "!";
    auto* iconLbl = new QLabel(iconChar, toast);
    iconLbl->setAlignment(Qt::AlignCenter);
    iconLbl->setFixedSize(28, 28);
    iconLbl->setStyleSheet(QString(
        "color:#FFFFFF; font-size:13px; font-weight:900;"
        "background:%1; border-radius:14px;").arg(accentColor.name()));

    auto* contentLay = new QVBoxLayout;
    contentLay->setSpacing(4);

    auto* titleLbl = new QLabel(title, toast);
    titleLbl->setStyleSheet("color:#F1F5F9; font-size:12px; font-weight:bold;");

    auto* msgLbl = new QLabel(message, toast);
    msgLbl->setStyleSheet("color:#94A3B8; font-size:11px;");
    msgLbl->setWordWrap(true);
    msgLbl->setFixedWidth(248);

    contentLay->addWidget(titleLbl);
    contentLay->addWidget(msgLbl);

    hl->addWidget(iconLbl, 0, Qt::AlignTop);
    hl->addLayout(contentLay);

    toast->adjustSize();

    positionToast(toast);
    m_activeToasts.append(toast);
    toast->show();

    // 3초 표시 후 500ms 페이드아웃
    auto* timer = new QTimer(toast);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [this, toast]() {
        auto* anim = new QPropertyAnimation(toast, "windowOpacity", toast);
        anim->setDuration(500);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        connect(anim, &QPropertyAnimation::finished, this, [this, toast]() {
            m_activeToasts.removeOne(toast);
            toast->deleteLater();
            repositionToasts();
        });
        anim->start();
    });
    timer->start(3000);
}

void NotificationManager::positionToast(QWidget* toast) {
    const int margin   = 16;
    const int stackGap = 8;

    QRect winRect = m_mainWindow->geometry();

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect avail = screen ? screen->availableGeometry() : winRect;

    // 기존 토스트 높이만큼 위로 쌓기
    int yOffset = 0;
    for (QWidget* t : m_activeToasts)
        yOffset += t->height() + stackGap;

    int x = winRect.right() - toast->width() - margin;
    int y = winRect.bottom() - toast->height() - margin - yOffset;

    x = qBound(avail.left(), x, avail.right()  - toast->width());
    y = qBound(avail.top(),  y, avail.bottom() - toast->height());

    toast->move(x, y);
}

void NotificationManager::repositionToasts() {
    const int margin   = 16;
    const int stackGap = 8;

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
    QRect winRect = m_mainWindow ? m_mainWindow->geometry() : avail;

    int yOffset = 0;
    for (QWidget* t : m_activeToasts) {
        if (!t) continue;
        int x = winRect.right() - t->width() - margin;
        int y = winRect.bottom() - t->height() - margin - yOffset;
        x = qBound(avail.left(), x, avail.right()  - t->width());
        y = qBound(avail.top(),  y, avail.bottom() - t->height());
        t->move(x, y);
        yOffset += t->height() + stackGap;
    }
}
