#include "NotificationManager.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QApplication>
#include <QStyle>
#include <QDebug>

NotificationManager& NotificationManager::instance() {
    static NotificationManager inst;
    return inst;
}

NotificationManager::NotificationManager(QObject* parent) : QObject(parent) {}

void NotificationManager::initialize(QObject* parent) {
    if (m_tray) return;
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

        if (ratio >= 1.0) {
            notify("예산 초과",
                   QString("%1 예산을 초과했습니다! 지출: %2 / 한도: %3")
                       .arg(cat).arg(spent, 0, 'f', 0).arg(limit, 0, 'f', 0));
        } else if (ratio >= 0.8) {
            notify("예산 경고",
                   QString("%1 예산이 %2%%에 도달했습니다 (%3 / %4)")
                       .arg(cat).arg(int(ratio * 100)).arg(spent, 0, 'f', 0).arg(limit, 0, 'f', 0));
        }
    }
}

void NotificationManager::notify(const QString& title, const QString& message) {
    if (m_tray && QSystemTrayIcon::isSystemTrayAvailable())
        m_tray->showMessage(title, message, QSystemTrayIcon::Warning, 5000);
}
